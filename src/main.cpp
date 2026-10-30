#include "app.h"

#include "imgui.h"

int main() {
  App app = {};
  if (!app.init("ryzc", 1280, 800)) {
    return 1;
  }

  bool show_demo_window = true;

  while (app.begin_frame()) {
    ImGui::ShowDemoWindow(&show_demo_window);
    app.end_frame();
  }

  app.shutdown();
  return 0;
}
