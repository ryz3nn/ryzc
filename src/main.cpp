#include "app.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

int main() {
  App app("ryzc", 1280, 800);

  bool show_demo_window = true;
  return app.run([&] { ImGui::ShowDemoWindow(&show_demo_window); });
}
