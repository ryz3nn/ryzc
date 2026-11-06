Building a lightweight Wayland client library like GLFW or SDL's `src/video/wayland` backend requires shifting away from traditional windowing architectures (like Win32 or X11). In Wayland:
1. **The client does not know global coordinates** (no `set_window_pos(x, y)`).
2. **Every frame is transactional/atomic** (no tearing, changes are staged in pending state and applied on `wl_surface_commit`).
3. **No pixels may be presented until the compositor authorizes it** (you must wait for the initial `xdg_surface.configure` event before committing your first buffer).
4. **Buffer allocation and lifecycle are client-driven** (the compositor reads your memory; you cannot reuse or modify a buffer until the compositor emits `wl_buffer.release`).

Here is an architectural roadmap divided into 8 distinct phases to build a dependable, production-ready C99 Wayland client library.

---

### Phase 0: Toolchain & Protocol Code Generation

Before writing application logic, set up automated code generation for the Wayland protocol XML specifications.

#### 1. Dependencies to link
- `libwayland-client` (core IPC & wire protocol)
- `libwayland-cursor` (loading system cursor themes)
- `libxkbcommon` (keyboard translation & state machine)
- `libEGL` & `libGL` / `libGLESv2` (for OpenGL rendering)
- `wayland-protocols` (system package containing standard extensions)

#### 2. Protocol XMLs needed
- **Core**: `wayland.xml` (shipped with `libwayland`, provides `wl_display`, `wl_registry`, `wl_compositor`, `wl_shm`, `wl_seat`, `wl_output`).
- **Window Management**: `xdg-shell.xml` (stable extension for `xdg_wm_base`, `xdg_surface`, `xdg_toplevel`, `xdg_popup`).
- **Window Decorations**: `xdg-decoration-unstable-v1.xml` (negotiating server-side vs client-side window borders).
- **Advanced Input** *(later phase)*: `pointer-constraints-unstable-v1.xml` (pointer lock for first-person cameras) and `relative-pointer-unstable-v1.xml`.
- **HiDPI Scaling** *(modern Wayland)*: `fractional-scale-v1.xml` and `viewporter.xml`.

#### 3. Build step (`wayland-scanner`)
In CMake / Ninja:
```bash
# Generate C headers
wayland-scanner client-header < xdg-shell.xml > xdg-shell-client-protocol.h
# Generate C marshalling tables
wayland-scanner private-code  < xdg-shell.xml > xdg-shell-protocol.c
```

---

### Phase 1: Core Connection, Registry & Event Loop Architecture

GLFW and SDL cannot just block inside `wl_display_dispatch()`. You need non-blocking event pumping that plays nicely with game loops and timers.

```
+-----------------------------------------------------------+
|                        User Loop                          |
|  while (running) { poll_events(); update(); render(); }   |
+-----------------------------+-----------------------------+
                              |
                     poll_events()
                              v
+-----------------------------------------------------------+
|                     Wayland Event Pump                    |
|  1. wl_display_dispatch_pending()                         |
|  2. wl_display_flush()                                    |
|  3. poll(wl_display_get_fd(), POLLIN, timeout=0)          |
|  4. wl_display_prepare_read() -> read_events() -> dispatch|
+-----------------------------------------------------------+
```

#### Key Architecture Steps:
1. **Connection**:
   - `wl_display_connect(NULL)` inspects `$WAYLAND_SOCKET`, then `$WAYLAND_DISPLAY`, then defaults to `wayland-0`.
2. **Registry Listener**:
   - Call `wl_display_get_registry(display)` and attach `wl_registry_listener`.
   - In `registry_handle_global()`, match interface names and bind to:
     - `wl_compositor` (creates surfaces & regions)
     - `wl_shm` (software memory buffers)
     - `xdg_wm_base` (window shell)
     - `wl_seat` (keyboard, mouse, touch input)
     - `wl_output` (monitors, resolution, scale)
     - `zxdg_decoration_manager_v1` (window decorations)
   - Perform initial `wl_display_roundtrip()` to ensure all advertised globals are bound before proceeding.
3. **Non-blocking Event Loop Pattern**:
   - Implement thread-safe read semantics using `wl_display_prepare_read()`, `wl_display_cancel_read()`, `wl_display_read_events()`, and `wl_display_dispatch_pending()`.

---

### Phase 2: Displays & Output Management (`wl_output`)

Like `glfwGetMonitors()`, your library must track connected displays and their properties.

#### Key Architecture Steps:
1. **Output Registry**:
   - Maintain a dynamic list of outputs (`struct LibOutput`). When `wl_output` is seen in the registry, allocate a tracking object.
   - When `global_remove` fires with an output's ID, cleanly destroy that output.
2. **Output Listener**:
   - `geometry`: stores monitor name, physical dimensions, subpixel layout, transform.
   - `mode`: lists available resolutions and refresh rates (marks the current mode with `WL_OUTPUT_MODE_CURRENT`).
   - `scale`: integer DPI scaling factor (1x, 2x, etc.).
3. **Surface-to-Output Tracking**:
   - Listen to `wl_surface.enter` and `wl_surface.leave`.
   - Maintain a set of outputs each window is currently spanning.
   - Whenever the window enters a monitor with a higher scale factor, update the window's buffer scale via `wl_surface_set_buffer_scale()` and re-render at the target resolution.

---

### Phase 3: Window Lifecycle & The XDG Shell State Machine

Wayland window creation follows a strict multi-stage handshake. You cannot simply allocate pixels and display them immediately.

```
wl_compositor_create_surface()
           │
           ▼
xdg_wm_base_get_xdg_surface()
           │
           ▼
xdg_surface_get_toplevel()
           │
wl_surface_commit()  <--- Required to initiate negotiation
           │
           ▼ (Compositor calculates layout)
xdg_toplevel.configure(width, height, states)
           │
xdg_surface.configure(serial)
           │
           ▼
Client resizes internal buffer -> ack_configure(serial)
           │
wl_surface_attach(buffer) + wl_surface_commit()
           │
           ▼ (Window is finally mapped onto screen!)
```

#### Key Architecture Steps:
1. **Window Object (`LibWindow`)**:
   - Contains `wl_surface`, `xdg_surface`, `xdg_toplevel`, window states (maximized, fullscreen, active, resizing), and current/configured dimensions.
2. **Ping / Pong**:
   - Listen to `xdg_wm_base.ping` and immediately call `xdg_wm_base_pong(..., serial)`. If you don't respond, the compositor assumes your app has frozen and prompts the user to force-kill it.
3. **Configure & State Transitions**:
   - `xdg_toplevel.configure`: Compositor provides suggested `width` and `height` (0 means "client chooses"), plus state flags (`MAXIMIZED`, `FULLSCREEN`, `ACTIVATED`, `RESIZING`).
   - Store these in `window->pending_width`, `window->pending_states`.
   - `xdg_surface.configure`: Signals the end of configuration for this frame.
   - Call `xdg_surface_ack_configure(xdg_surface, serial)`.
   - Trigger the resize callback in your library and render the matching buffer size.
4. **Close Request**:
   - Listen to `xdg_toplevel.close`. This is the user clicking the "X" button or pressing Alt+F4. Set `window->should_close = true`.

---

### Phase 4: Graphics & Frame Presentation

A GUI library needs to support both **Software Rendering (CPU)** and **Hardware Accelerated Rendering (GPU)**.

#### Path A: Software Rendering (`wl_shm`)
1. Create an anonymous shared memory file (`memfd_create()` on modern Linux, or `shm_open()` with fallback).
2. `ftruncate()` the fd to `width * height * 4 * num_buffers`.
3. `mmap()` the file into client address space (`MAP_SHARED`).
4. Pass the fd to `wl_shm_create_pool()`, then carve out individual `wl_buffer` instances.
5. **Double/Triple Buffering Rule**:
   - Attach `wl_buffer_listener` to listen for the `.release` event.
   - Never write to a buffer currently owned by the compositor. Maintain an available queue of buffers.

#### Path B: OpenGL / EGL (`libwayland-egl`)
Like GLFW/SDL:
1. Initialize EGL display with `eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_KHR, display, ...)`.
2. Choose EGL config matching your color/depth requirements.
3. Create native window bridge: `struct wl_egl_window *egl_window = wl_egl_window_create(surface, width, height)`.
4. Create surface: `eglCreatePlatformWindowSurfaceEXT(..., egl_window, ...)`.
5. When resizing, call `wl_egl_window_resize(egl_window, new_width, new_height, 0, 0)`.
6. `eglSwapBuffers()` automatically commits the underlying `wl_surface`.

#### Frame Throttling (`wl_surface.frame`)
- Do not blindly render in an unthrottled loop.
- Before committing, request a frame callback: `cb = wl_surface_frame(surface)`.
- Listen for `wl_callback_listener.done`. Only draw the subsequent frame when the compositor reports ready. (For EGL, this synchronizes with `eglSwapInterval`).

---

### Phase 5: Input Subsystem (`wl_seat`)

Wayland combines all input devices under dynamic "seats".

#### 1. Seat Capabilities
- Listen for `wl_seat.capabilities`.
- Dynamically bind/release:
  - Pointer (`WL_SEAT_CAPABILITY_POINTER`)
  - Keyboard (`WL_SEAT_CAPABILITY_KEYBOARD`)
  - Touch (`WL_SEAT_CAPABILITY_TOUCH`)

#### 2. Mouse / Pointer (`wl_pointer`)
- **The Frame Pattern**: Pointer events (`enter`, `leave`, `motion`, `button`, `axis`) arrive piecemeal. Do not dispatch to user callbacks immediately. Accumulate changes in a pending event struct and dispatch when `wl_pointer.frame` arrives.
- **Buttons**: Scancodes match Linux `input-event-codes.h` (`BTN_LEFT = 0x110`, `BTN_RIGHT = 0x111`, etc.). Map these to your library's `MOUSE_BUTTON_LEFT/RIGHT`.
- **Cursor Management**:
  - Wayland windows do not have default cursors! When `wl_pointer.enter` arrives, the cursor disappears unless you provide one.
  - Use `wl_cursor_theme_load()` and `wl_cursor_theme_get_cursor(theme, "left_ptr")`.
  - Obtain the `wl_buffer` from the cursor image, attach it to a dedicated cursor `wl_surface`, and call `wl_pointer_set_cursor(pointer, enter_serial, cursor_surface, hotspot_x, hotspot_y)`.

#### 3. Keyboard (`wl_keyboard` + `xkbcommon`)
- **Keymap Event**: The server passes a file descriptor containing the XKB keymap string. `mmap()` the fd and compile via `xkb_keymap_new_from_string()`, then initialize `xkb_state_new(keymap)`.
- **Key Event**: Wayland delivers raw evdev scancodes. **Crucial rule**: Add `8` to the scancode before passing to XKB functions (`xkb_state_key_get_one_sym(state, code + 8)`).
- **Modifiers**: Listen to `wl_keyboard.modifiers` and pass masks (`depressed`, `latched`, `locked`, `group`) directly into `xkb_state_update_mask()`.
- **Key Repeat**: Compositor emits `repeat_info(rate, delay)`. The client must implement its own timer (e.g. via `timerfd_create()` or internal clock checks) to emit repeated key events.

---

### Phase 6: Window Decorations & Window Controls

On X11 and Windows, the OS typically draws titlebars and borders. On Wayland:
- GNOME / Mutter enforces **Client-Side Decorations (CSD)**.
- KDE Plasma / wlroots supports **Server-Side Decorations (SSD)**.

#### 1. Decoration Negotiation (`xdg-decoration`)
- Create `zxdg_toplevel_decoration_v1` from your `xdg_toplevel`.
- Request `ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE`.
- Handle `configure`:
  - If server grants SSD (`SERVER_SIDE`), the compositor draws the border, minimize, maximize, and close buttons.
  - If server insists on CSD (`CLIENT_SIDE`), your GUI library must render its own titlebar and control buttons.

#### 2. Interactive Move & Resize
- Wayland forbids applications from changing their own screen coordinates.
- When the user clicks the title bar:
  - Call `xdg_toplevel_move(toplevel, seat, button_press_serial)`. The compositor takes over and begins dragging the window.
- When the user clicks the window edges/corners:
  - Call `xdg_toplevel_resize(toplevel, seat, button_press_serial, edge)`. The compositor handles edge dragging.

---

### Phase 7: Advanced Features for Full SDL/GLFW Parity

| Feature | Wayland Protocol / Extension | How It Works |
| :--- | :--- | :--- |
| **Clipboard / Selection** | `wl_data_device_manager` | Reads/writes via UNIX pipes (`pipe2()`) with MIME types like `text/plain;charset=utf-8`. |
| **Relative Mouse / Mouse Lock** | `zwp_relative_pointer_manager_v1` & `zwp_pointer_constraints_v1` | Locks pointer in place (or hides it) and delivers raw delta `(dx, dy)` for 3D camera controls. |
| **Fractional Scaling** | `wp_fractional_scale_manager_v1` & `wp_viewport` | Avoids blurry 125%/150% scaling by telling the client the exact physical ratio (in 1/120ths). |
| **Context Menus / Popups** | `xdg_positioner` & `xdg_popup` | Positions child dropdown menus relative to an anchor rectangle on the parent window. |

---

### Recommended Implementation Milestones

When starting implementation, write small test harnesses verifying each step before building the abstraction:

```
[Milestone 1: The Raw Wire Test]
└── Connect to wl_display, bind wl_shm, wl_compositor, xdg_wm_base.
    Ack configure, allocate 1 shm buffer, and paint a static color window.

[Milestone 2: The Graphics & Resizing Loop]
└── Add eglCreatePlatformWindowSurfaceEXT or double-buffered shm.
    Handle xdg_toplevel configure sizes and wl_surface.frame throttling.

[Milestone 3: Input & Pointer]
└── Integrate wl_seat. Load standard cursor via wayland-cursor.
    Handle mouse clicks and print motion coordinates.

[Milestone 4: Text Input with XKB]
└── Map keyboard keymap fds, initialize xkb_state, translate scancodes (+8)
    to UTF-8 strings and keysyms.

[Milestone 5: Library API Packaging]
└── Wrap into clean GLFW-like public API (e.g. `lib_init()`, `lib_create_window()`,
    `lib_poll_events()`, `lib_swap_buffers()`).
```
