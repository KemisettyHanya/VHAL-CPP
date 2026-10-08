#include "SignalPalette.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace can_vhal {

namespace {

std::string toLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

} // namespace

void SignalPalette::render(
    const std::vector<CanMessage>& messages,
    OnAddSignalCallback onAddSignal,
    OnAddMessageCallback onAddMessage) {

    // Header banner with stats
    size_t totalSignals = 0;
    for (const auto& m : messages) totalSignals += m.signals.size();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.82f, 1.00f, 1.00f));
    ImGui::Text("SOURCE CAN DBC SIGNALS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu msgs, %zu sigs)", messages.size(), totalSignals);

    ImGui::Separator();
    ImGui::Spacing();

    // Search filter row
    char filterBuf[128];
    std::snprintf(filterBuf, sizeof(filterBuf), "%s", mFilterText.c_str());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 36.0f);
    if (ImGui::InputTextWithHint("##CanFilter", "Search signal or ID (e.g. Speed, 0x100)...", filterBuf, sizeof(filterBuf))) {
        mFilterText = filterBuf;
    }
    ImGui::SameLine();
    if (ImGui::Button("X##ClearCanFilter", ImVec2(28.0f, 0.0f))) {
        mFilterText.clear();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear search filter");
    }

    // Expand / Collapse / Add All action buttons
    if (ImGui::Button("Expand All")) {
        mExpandCollapseState = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Collapse All")) {
        mExpandCollapseState = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Add All Msgs")) {
        for (const auto& msg : messages) {
            if (onAddMessage) onAddMessage(msg);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add all signals across all messages to canvas");
    }

    std::string filterLower = toLower(mFilterText);

    ImGui::Spacing();
    ImGui::BeginChild("CanSignalTreeRegion", ImVec2(0, -28.0f), true);

    if (messages.empty()) {
        ImGui::TextWrapped("No DBC loaded yet.\nSelect 'File -> Open DBC...' or 'Load Full Production DBC' in top menu.");
    }

    for (const auto& msg : messages) {
        // Filter match logic
        std::string msgNameLower = toLower(msg.name);
        std::string msgIdStr = toLower(msg.idHexStr());
        bool msgMatches = filterLower.empty() ||
                          (msgNameLower.find(filterLower) != std::string::npos) ||
                          (msgIdStr.find(filterLower) != std::string::npos);

        // Check if any child signals match
        bool anySignalMatches = false;
        for (const auto& sig : msg.signals) {
            std::string sigLower = toLower(sig.name);
            if (sigLower.find(filterLower) != std::string::npos) {
                anySignalMatches = true;
                break;
            }
        }

        if (!msgMatches && !anySignalMatches) {
            continue;
        }

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!filterLower.empty()) {
            nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        if (mExpandCollapseState == 1) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        } else if (mExpandCollapseState == -1) {
            ImGui::SetNextItemOpen(false, ImGuiCond_Always);
        }

        char headerLabel[256];
        std::snprintf(headerLabel, sizeof(headerLabel), "%s  %s  (DLC:%u, %zu sigs)###msg_%u",
            msg.idHexStr().c_str(), msg.name.c_str(), msg.dlc, msg.signals.size(), msg.id);

        bool nodeOpen = ImGui::TreeNodeEx(headerLabel, nodeFlags);

        // Quick "+ All" button on right side of tree node header
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 56.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.35f, 0.50f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.48f, 0.70f, 1.0f));
        std::string btnId = "+ All##btn_all_" + std::to_string(msg.id);
        if (ImGui::SmallButton(btnId.c_str())) {
            if (onAddMessage) onAddMessage(msg);
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Add all %zu signals of message %s to canvas", msg.signals.size(), msg.name.c_str());
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Add All Signals to Canvas")) {
                if (onAddMessage) onAddMessage(msg);
            }
            ImGui::EndPopup();
        }

        if (nodeOpen) {
            if (!msg.comment.empty()) {
                ImGui::TextDisabled("  Note: %s", msg.comment.c_str());
            }

            for (const auto& sig : msg.signals) {
                std::string sigLower = toLower(sig.name);
                if (!filterLower.empty() && !msgMatches && sigLower.find(filterLower) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(sig.name.c_str());

                // Prominent + Add button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.32f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.62f, 0.44f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.75f, 0.52f, 1.0f));
                if (ImGui::SmallButton("+ Add")) {
                    if (onAddSignal) onAddSignal(msg, sig);
                }
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Add this signal node to canvas");
                }

                ImGui::SameLine();
                // Signal Name
                ImGui::TextColored(ImVec4(0.96f, 0.97f, 0.98f, 1.0f), "%s", sig.name.c_str());
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (onAddSignal) onAddSignal(msg, sig);
                }

                // Unit badge in cyan or raw
                ImGui::SameLine();
                if (!sig.unit.empty()) {
                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "[%s]", sig.unit.c_str());
                } else {
                    ImGui::TextDisabled("[raw]");
                }

                // Bit range & factor
                ImGui::SameLine();
                ImGui::TextDisabled("(b%u:%u, %gx)", sig.startBit, sig.bitLength, sig.factor);

                // Rich Tooltip on hover
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Signal: %s", sig.name.c_str());
                    ImGui::Separator();
                    ImGui::Text("Message: %s (%s)", msg.name.c_str(), msg.idHexStr().c_str());
                    ImGui::Text("Bit Range: %u .. %u (Length: %u)",
                        sig.startBit, sig.startBit + sig.bitLength - 1, sig.bitLength);
                    ImGui::Text("Layout: %s | Type: %s",
                        sig.endianness == Endianness::LittleEndian ? "Intel (Little Endian)" : "Motorola (Big Endian)",
                        sig.isSigned() ? "Signed" : "Unsigned");
                    ImGui::Text("Scale: %g | Offset: %g", sig.factor, sig.offset);
                    ImGui::Text("Physical Range: [%g .. %g] %s", sig.minVal, sig.maxVal, sig.unit.c_str());
                    if (!sig.comment.empty()) {
                        ImGui::TextDisabled("Comment: %s", sig.comment.c_str());
                    }
                    if (!sig.valueDescriptions.empty()) {
                        ImGui::Spacing();
                        ImGui::Text("Enums:");
                        for (const auto& vd : sig.valueDescriptions) {
                            ImGui::TextDisabled("  %lld -> %s", static_cast<long long>(vd.value), vd.description.c_str());
                        }
                    }
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Tip: Click '+ Add' or double-click to place on canvas.");
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }

    // Reset expand/collapse trigger after rendering
    mExpandCollapseState = 0;

    ImGui::EndChild();

    // Footer stats
    ImGui::TextDisabled("%zu Messages | %zu Total Signals", messages.size(), totalSignals);
}

} // namespace can_vhal
