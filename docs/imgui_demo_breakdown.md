# Here is break down of imgui demo, for learning purpose 

## 1. Title 
- First create window from Platform 
- Start with ImGui::Begin("Title", p_open, window_flags), if return false, end with ImGui::End()
- Confused with width for frame widget and width for label
- Simple text with ImGui:Text("hello (%s) (%d)", "placeholder", 5)

## 2. Menubar
- Start with ImGui::BeginMenuBar(), it is bool so wrap in if statement and end with EndMenuBar()
- Each menu start with template 
    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Open", "shortcut", p_selected, enable); 
        // selected: mean can be click or not, enable: Have a tick after label
        ImGui::EndMenu();
    }
- BeginMenu can be as MenuItem by put it nested in other BeginMenu
- Checkbox:
    ImGui::BeginDisabled(bool)
    ImGui::Checkbox("Label", bool)
    ImGui::EndDisabled(bool)
- ImGui::TextDisabled("Text"): Make text in gray color
- ImGui::GetIO(): Not sure about this maybe get client i/o ?L:w
- ImGui::SetItemTooltip("Show tooltip when hover")

- ImGui::Separator(): horizontal line split
- ImGui::SeparatorText("text"): horizontal line split cross text 

- ImGui::BeginChild("name", size, border_flag, window_flags): Create snippet with border
- ImGui::SliderFloat("Value", value, min, max, format, flag)
- ImGui::InputFloat("Value", value, step, step_fast, format, flag)
- ImGui::Combo("Combo", current_item, array + count | string_split_by_zero, popup): Dropdown list

- Appending menu by call BeginMenu("SameName") again

## 3. Layout and Scrolling
- Child have override or treat as framed item (what is framed item)
- ImGui::SameLine(offset, spacing): Append to right to preceding item
- ImGui::Button() and SmallButton()

