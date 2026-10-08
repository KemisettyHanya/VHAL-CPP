#include "PropertyPalette.hpp"
#include "core/VhalCatalog.hpp"
#include <imgui.h>

namespace can_vhal {

void PropertyPalette::render(OnAddPropertyCallback onAddProperty) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.95f, 0.65f, 1.00f));
    ImGui::Text("TARGET AOSP VEHICLE PROPERTIES");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Filter input
    char filterBuf[128];
    std::snprintf(filterBuf, sizeof(filterBuf), "%s", mFilterText.c_str());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 36.0f);
    if (ImGui::InputTextWithHint("##PropFilter", "Search AOSP properties (e.g. SPEED, GEAR)...", filterBuf, sizeof(filterBuf))) {
        mFilterText = filterBuf;
    }
    ImGui::SameLine();
    if (ImGui::Button("X##ClearPropFilter", ImVec2(28.0f, 0.0f))) {
        mFilterText.clear();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear search filter");
    }

    // Category Selector
    auto categories = VhalCatalog::instance().getCategories();
    categories.insert(categories.begin(), "All");

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##PropCategoryCombo", ("Category: " + mSelectedCategory).c_str())) {
        for (const auto& cat : categories) {
            bool isSelected = (mSelectedCategory == cat);
            if (ImGui::Selectable(cat.c_str(), isSelected)) {
                mSelectedCategory = cat;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::BeginChild("PropListRegion", ImVec2(0, -28.0f), true);

    auto matchedProps = VhalCatalog::instance().search(mFilterText, mSelectedCategory);

    if (matchedProps.empty()) {
        ImGui::TextDisabled("No matching AOSP properties found.");
    }

    for (const auto& prop : matchedProps) {
        ImGui::PushID(static_cast<int>(prop.id));

        // + Add button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.62f, 0.44f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.75f, 0.52f, 1.0f));
        if (ImGui::SmallButton("+ Add")) {
            if (onAddProperty) onAddProperty(prop);
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Add this property node to canvas");
        }

        ImGui::SameLine();
        // Property name
        ImGui::TextColored(ImVec4(0.95f, 0.96f, 0.98f, 1.0f), "%s", prop.name.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (onAddProperty) onAddProperty(prop);
        }

        // Type badge in gold/amber
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.92f, 0.82f, 0.45f, 1.0f), "[%s]", vhalTypeToString(prop.type));

        // Unit badge if present
        if (!prop.unit.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "[%s]", prop.unit.c_str());
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.65f, 1.0f), "Property: %s", prop.name.c_str());
            ImGui::Separator();
            ImGui::Text("AIDL ID: %s", prop.idHexStr().c_str());
            ImGui::Text("Category: %s", prop.category.c_str());
            ImGui::Text("Type: %s | Unit: %s", vhalTypeToString(prop.type), prop.unit.c_str());
            ImGui::Text("Change Mode: %s", changeModeToString(prop.changeMode));
            ImGui::TextDisabled("Desc: %s", prop.description.c_str());
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Tip: Click '+ Add' or double-click to place on canvas.");
            ImGui::EndTooltip();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    ImGui::TextDisabled("%zu Properties Available", matchedProps.size());
}

} // namespace can_vhal
