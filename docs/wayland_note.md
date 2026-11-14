## Disclaimer: This is my note, and it might not be correct at all.

## X11 Architecture
1. User click/type, event send to kernel (evdev)
2. X server determine which window receive event
3. Client (Window) receive input and update state
4. Application send rendering request to server for update UI
5. X notify compositor that window has been changed (damage event)
6. Compositor rendering desktop, send to X server -> KMS -> page flip to new buffer

## Wayland Architecture
1. Kernel receive input, and send to compositor
2. Compositor determine window and send event to it
3. Application receive this event, update UI and rendering itself
4. Application notify compositor the region that was update
5. Compositor collect damage event and recomposite screen then send to KMS

### Wayland rendering
- Application share buffer with compositor in video memory
- Application render to this buffer and compositor use this buffer for recomposite screen.
- 2 ways update buffer:
    - Buffer swapping: Render into buffer then notify compositor use it 
    - Direct: Render into buffer that be used by compositor.

### Components need:
- DRM: GPU resource manager, orchestrate command queue to GPU
- KMS: Display and modesetting manage (resolution, refresh rate, v-sync, ...)
- Wayland EGL -> EGL -> GLES

## Wayland Protocol
### 1. Basic principle
- Client sent request to server, then server send event to client
- All action be done through object with (ID, interface: method, event, opcode)
- It's asynchorous: Server send event, and client listen and change if need (cache, buffer, ...) without query server all time.

### 2. Code Generation
- wayland.xml generate C API for Client and Compositor

### 3. Wire format
- Wayland use UNIX Domain Socket for communicate, default is wayland-0 or can be set via $WAYLAND_DISPLAY
- Message format:<br>
Header | Payload
Object ID (32 bits) - Message Size (16 bits) - Opcode (16 bits) | Arguments 
- File descriptor: share buffer between Client and Compositor, so it only need to read from buffer then render without copy

### 4. Version
- Each interface has own version, and Client choose version to bind

### 5. Connect time
1. Client receive wl_display with id 1
2. Client get registry for available interface and version.
3. Choose and bind needed interface to global object

### 6. Creating object
- ID: 0 - NULL
- ID: 1 - wl_display
- ID: from 2 - Client object
- ID: from 0xff000000 - Server object

### 7. Display and Registry
- wl_display: manage connection, handle request/event, create registry
- registry: where Client can find interface that Compositor supported

### 8. Surface
- wl_surface: is rectangular grid that client create for display content (window)
- Client create buffer then attach buffer into surface (render -> buffer -> surface)
- Server read buffer then render screen, after that it return buffer back to client

### 9. Input
- wl_seat: gather input device in here like: mouse, keyboard
- Compositor determine surface receive focus: like mouse on window A -> A receive pointer focus,...
- Serial number for check input valid or not
- Application must handle cursor image itself

### 10. Output
- wl_output can be monitor or logical Output
- It can be considered as xrandr on X11

### 11. Data Sharing
- 2 common ways: Copy-Paste, and Drag-and-Drop
- Flow:<br>
1. Sending client create source and show type of data
2. Receiver receive data_offer and choose MIME, then send request with fd
3. Sending writing data into fd

### Step to create window
- get display -> add needed registry -> create compositor -> create surface and shm (CPU) or EGL -> attach buffer to surface -> commit surface and render
- wl_surface is only pixel content, to make it into window need xdg_shell (xdg_toplevel, xdg_popup):
    - create surface -> create xdg_surface -> Create toplevel (normal windows) -> Wait server send configure event -> Client attach to buffer and acknowledge this event for notify render done -> Commit to server for starting to composite
- Render mechanism: Client ask server whether ready to render, server response with done(). Now client destroy this callback and request new callback (for next frame), and prepare buffer then attach, commit to surface



