#include "CanvasView.hpp"
#include "core/VhalCatalog.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cctype>

namespace can_vhal {

namespace ed = ax::NodeEditor;

namespace {

std::string toLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

void drawPinSocket(bool isConnected, ImU32 ringColor, ImU32 dotColor, float radius = 8.0f) {
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float diameter = radius * 2.0f;
    ImVec2 center(cursor.x + radius + 1.0f, cursor.y + ImGui::GetTextLineHeight() * 0.5f);

    if (isConnected) {
        drawList->AddCircle(center, radius + 2.5f, ringColor & 0x66FFFFFF, 16, 2.0f);
    }
    drawList->AddCircle(center, radius, ringColor, 16, 2.0f);

    if (isConnected) {
        drawList->AddCircleFilled(center, radius - 2.5f, dotColor);
    } else {
        drawList->AddCircleFilled(center, radius - 4.5f, IM_COL32(230, 230, 230, 220));
    }

    ImGui::Dummy(ImVec2(diameter + 3.0f, diameter));
}

} // namespace

CanvasView::CanvasView() {
    init();
}

CanvasView::~CanvasView() {
    if (mEditorContext) {
        ed::DestroyEditor(mEditorContext);
        mEditorContext = nullptr;
    }
}

void CanvasView::init() {
    if (!mEditorContext) {
        ed::Config config;
        config.SettingsFile = "can_vhal_canvas.json";
        mEditorContext = ed::CreateEditor(&config);
    }
}

void CanvasView::clearCanvas() {
    mNodes.clear();
    mLinks.clear();
    mPinToNode.clear();
    mNextId = 1;
    mLastSelectedCanNodeId = 0;
    mLastSelectedVhalNodeId = 0;
    mSpawnCount = 0;
    if (mOnLinksChanged) mOnLinksChanged();
}

uint64_t CanvasView::addCanSignalNode(const CanMessage& msg, const CanSignal& sig, float posX, float posY) {
    float targetX = posX;
    float targetY = posY;
    if (targetX == 0.0f && targetY == 0.0f) {
        float cascade = static_cast<float>((mSpawnCount++ % 5) * 25);
        targetX = mCurrentViewCenter.x - 220.0f + cascade;
        targetY = mCurrentViewCenter.y - 60.0f + cascade;
    }

    // Check if node already exists for this signal
    for (auto& node : mNodes) {
        if (node.type == CanvasNodeType::CAN_SIGNAL &&
            node.canMsgId == msg.id &&
            node.signal.name == sig.name) {
            // Reposition to current focused view so user immediately sees it
            node.posX = targetX;
            node.posY = targetY;
            if (mEditorContext) {
                ed::SetCurrentEditor(mEditorContext);
                ed::SetNodePosition(ed::NodeId(node.id), ImVec2(targetX, targetY));
                ed::ClearSelection();
                ed::SelectNode(ed::NodeId(node.id), false);
            }
            mLastSelectedCanNodeId = node.id;
            return node.id;
        }
    }

    CanvasNode node;
    node.id = mNextId++;
    node.type = CanvasNodeType::CAN_SIGNAL;
    node.canMsgId = msg.id;
    node.canMsgName = msg.name;
    node.signal = sig;
    node.outputPinId = mNextId++;
    node.posX = targetX;
    node.posY = targetY;

    mPinToNode[node.outputPinId] = node.id;
    mNodes.push_back(node);

    if (mEditorContext) {
        ed::SetCurrentEditor(mEditorContext);
        ed::SetNodePosition(ed::NodeId(node.id), ImVec2(node.posX, node.posY));
        ed::ClearSelection();
        ed::SelectNode(ed::NodeId(node.id), false);
    }
    mLastSelectedCanNodeId = node.id;

    return node.id;
}

uint64_t CanvasView::addVhalPropertyNode(const VhalProperty& prop, float posX, float posY) {
    float targetX = posX;
    float targetY = posY;
    if (targetX == 0.0f && targetY == 0.0f) {
        float cascade = static_cast<float>((mSpawnCount++ % 5) * 25);
        targetX = mCurrentViewCenter.x + 50.0f + cascade;
        targetY = mCurrentViewCenter.y - 60.0f + cascade;
    }

    // Check if node already exists for this property
    for (auto& node : mNodes) {
        if (node.type == CanvasNodeType::VHAL_PROPERTY && node.property.id == prop.id) {
            // Reposition to current focused view so user immediately sees it
            node.posX = targetX;
            node.posY = targetY;
            if (mEditorContext) {
                ed::SetCurrentEditor(mEditorContext);
                ed::SetNodePosition(ed::NodeId(node.id), ImVec2(targetX, targetY));
                ed::ClearSelection();
                ed::SelectNode(ed::NodeId(node.id), false);
            }
            mLastSelectedVhalNodeId = node.id;
            return node.id;
        }
    }

    CanvasNode node;
    node.id = mNextId++;
    node.type = CanvasNodeType::VHAL_PROPERTY;
    node.property = prop;
    node.inputPinId = mNextId++;
    node.posX = targetX;
    node.posY = targetY;

    mPinToNode[node.inputPinId] = node.id;
    mNodes.push_back(node);

    if (mEditorContext) {
        ed::SetCurrentEditor(mEditorContext);
        ed::SetNodePosition(ed::NodeId(node.id), ImVec2(node.posX, node.posY));
        ed::ClearSelection();
        ed::SelectNode(ed::NodeId(node.id), false);
    }
    mLastSelectedVhalNodeId = node.id;

    return node.id;
}

const CanvasNode* CanvasView::findNode(uint64_t nodeId) const {
    for (const auto& n : mNodes) {
        if (n.id == nodeId) return &n;
    }
    return nullptr;
}

const CanvasNode* CanvasView::findNodeByPin(uint64_t pinId) const {
    auto it = mPinToNode.find(pinId);
    if (it != mPinToNode.end()) {
        return findNode(it->second);
    }
    return nullptr;
}

void CanvasView::removeNode(uint64_t nodeId) {
    if (mLastSelectedCanNodeId == nodeId) mLastSelectedCanNodeId = 0;
    if (mLastSelectedVhalNodeId == nodeId) mLastSelectedVhalNodeId = 0;

    auto it = std::remove_if(mNodes.begin(), mNodes.end(), [nodeId](const CanvasNode& n) {
        return n.id == nodeId;
    });
    if (it != mNodes.end()) {
        uint64_t outPin = it->outputPinId;
        uint64_t inPin = it->inputPinId;
        mPinToNode.erase(outPin);
        mPinToNode.erase(inPin);
        mNodes.erase(it, mNodes.end());

        // Also remove attached links
        mLinks.erase(std::remove_if(mLinks.begin(), mLinks.end(), [outPin, inPin](const NodeLink& l) {
            return l.fromPinId == outPin || l.toPinId == inPin;
        }), mLinks.end());

        if (mOnLinksChanged) mOnLinksChanged();
    }
}

void CanvasView::removeLink(uint64_t linkId) {
    mLinks.erase(std::remove_if(mLinks.begin(), mLinks.end(), [linkId](const NodeLink& l) {
        return l.id == linkId;
    }), mLinks.end());
    if (mOnLinksChanged) mOnLinksChanged();
}

void CanvasView::addLink(const NodeLink& link) {
    // Avoid duplicates
    for (const auto& l : mLinks) {
        if (l.fromPinId == link.fromPinId && l.toPinId == link.toPinId) {
            return;
        }
    }
    mLinks.push_back(link);
    if (mOnLinksChanged) mOnLinksChanged();
}

bool CanvasView::createLinkBetweenPins(uint64_t fromPinId, uint64_t toPinId) {
    const CanvasNode* srcNode = findNodeByPin(fromPinId);
    const CanvasNode* dstNode = findNodeByPin(toPinId);

    if (!srcNode || !dstNode) return false;

    // Validate: src must be CAN_SIGNAL (outputPin), dst must be VHAL_PROPERTY (inputPin)
    if (srcNode->type == CanvasNodeType::VHAL_PROPERTY && dstNode->type == CanvasNodeType::CAN_SIGNAL) {
        std::swap(srcNode, dstNode);
        std::swap(fromPinId, toPinId);
    }

    if (srcNode->type != CanvasNodeType::CAN_SIGNAL || dstNode->type != CanvasNodeType::VHAL_PROPERTY) {
        return false;
    }

    // Check if link already exists
    for (const auto& l : mLinks) {
        if (l.fromPinId == fromPinId && l.toPinId == toPinId) {
            return false;
        }
    }

    NodeLink link;
    link.id = mNextId++;
    link.canMsgId = srcNode->canMsgId;
    link.canMsgName = srcNode->canMsgName;
    link.signalName = srcNode->signal.name;
    link.vhalPropertyId = dstNode->property.id;
    link.vhalPropertyName = dstNode->property.name;
    link.fromPinId = fromPinId;
    link.toPinId = toPinId;

    // Smart default conversion detection
    if (srcNode->signal.unit == "km/h" && dstNode->property.unit == "m/s") {
        link.transform.conversion = UnitConversion::KMH_TO_MPS;
    } else if (srcNode->signal.unit == "deg C" || srcNode->signal.unit == "celsius") {
        if (dstNode->property.unit == "kelvin") {
            link.transform.conversion = UnitConversion::CELSIUS_TO_KELVIN;
        }
    }

    mLinks.push_back(link);
    if (mOnLinksChanged) mOnLinksChanged();
    return true;
}

bool CanvasView::connectSelectedNodes(std::string* outStatusMessage) {
    if (!mEditorContext) {
        if (outStatusMessage) *outStatusMessage = "Editor not initialized.";
        return false;
    }
    ed::SetCurrentEditor(mEditorContext);

    const CanvasNode* canNode = nullptr;
    const CanvasNode* vhalNode = nullptr;

    // 1. Check directly selected nodes in the editor
    int count = ed::GetSelectedObjectCount();
    if (count > 0) {
        std::vector<ed::NodeId> selectedNodes(count);
        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), count);
        selectedNodes.resize(nodeCount);

        for (auto nid : selectedNodes) {
            const CanvasNode* n = findNode(nid.Get());
            if (!n) continue;
            if (n->type == CanvasNodeType::CAN_SIGNAL && !canNode) {
                canNode = n;
                mLastSelectedCanNodeId = n->id;
            } else if (n->type == CanvasNodeType::VHAL_PROPERTY && !vhalNode) {
                vhalNode = n;
                mLastSelectedVhalNodeId = n->id;
            }
        }
    }

    // 2. If only CAN node was selected, locate the target VHAL node
    if (canNode && !vhalNode) {
        if (mLastSelectedVhalNodeId != 0) {
            vhalNode = findNode(mLastSelectedVhalNodeId);
        }
        if (!vhalNode) {
            // Find any VHAL property node on the canvas
            for (const auto& n : mNodes) {
                if (n.type == CanvasNodeType::VHAL_PROPERTY) {
                    vhalNode = &n;
                    break;
                }
            }
        }
    }

    // 3. If only VHAL node was selected, locate the source CAN node
    if (vhalNode && !canNode) {
        if (mLastSelectedCanNodeId != 0) {
            canNode = findNode(mLastSelectedCanNodeId);
        }
        if (!canNode) {
            // Find any CAN signal node on the canvas
            for (const auto& n : mNodes) {
                if (n.type == CanvasNodeType::CAN_SIGNAL) {
                    canNode = &n;
                    break;
                }
            }
        }
    }

    // 4. If neither node is directly selected right now, fall back to last selected pair or single nodes on canvas
    if (!canNode && !vhalNode) {
        if (mLastSelectedCanNodeId != 0) canNode = findNode(mLastSelectedCanNodeId);
        if (mLastSelectedVhalNodeId != 0) vhalNode = findNode(mLastSelectedVhalNodeId);

        if (!canNode) {
            for (const auto& n : mNodes) {
                if (n.type == CanvasNodeType::CAN_SIGNAL) { canNode = &n; break; }
            }
        }
        if (!vhalNode) {
            for (const auto& n : mNodes) {
                if (n.type == CanvasNodeType::VHAL_PROPERTY) { vhalNode = &n; break; }
            }
        }
    }

    if (!canNode && !vhalNode) {
        if (outStatusMessage) *outStatusMessage = "No nodes on canvas to connect.";
        return false;
    }
    if (!canNode) {
        if (outStatusMessage) *outStatusMessage = "Select or add a CAN signal node first.";
        return false;
    }
    if (!vhalNode) {
        if (outStatusMessage) *outStatusMessage = "Select or add an AOSP property node first.";
        return false;
    }

    // Check if link already exists
    for (const auto& l : mLinks) {
        if (l.fromPinId == canNode->outputPinId && l.toPinId == vhalNode->inputPinId) {
            if (outStatusMessage) {
                *outStatusMessage = "Already connected: [" + canNode->signal.name + "] -> [" + vhalNode->property.name + "]";
            }
            return true;
        }
    }

    if (createLinkBetweenPins(canNode->outputPinId, vhalNode->inputPinId)) {
        if (outStatusMessage) {
            *outStatusMessage = "Connected: CAN [" + canNode->signal.name + "] -> AOSP [" + vhalNode->property.name + "]!";
        }
        return true;
    }

    if (outStatusMessage) *outStatusMessage = "Could not create link between pins.";
    return false;
}

size_t CanvasView::autoConnectMatchingNames() {
    size_t count = 0;
    for (const auto& canNode : mNodes) {
        if (canNode.type != CanvasNodeType::CAN_SIGNAL) continue;
        std::string sigLower = toLower(canNode.signal.name);

        for (const auto& vhalNode : mNodes) {
            if (vhalNode.type != CanvasNodeType::VHAL_PROPERTY) continue;
            std::string propLower = toLower(vhalNode.property.name);

            bool match = false;
            if (sigLower.find("speed") != std::string::npos && propLower.find("speed") != std::string::npos) match = true;
            else if (sigLower.find("rpm") != std::string::npos && propLower.find("rpm") != std::string::npos) match = true;
            else if (sigLower.find("coolant") != std::string::npos && propLower.find("coolant") != std::string::npos) match = true;
            else if (sigLower.find("temp") != std::string::npos && propLower.find("temp") != std::string::npos) match = true;
            else if (sigLower.find("gear") != std::string::npos && propLower.find("gear") != std::string::npos) match = true;
            else if (sigLower.find("fuel") != std::string::npos && propLower.find("fuel") != std::string::npos) match = true;
            else if (sigLower.find("brake") != std::string::npos && propLower.find("brake") != std::string::npos) match = true;
            else if (sigLower.find("battery") != std::string::npos && propLower.find("battery") != std::string::npos) match = true;
            else if (sigLower.find("pressure") != std::string::npos && propLower.find("pressure") != std::string::npos) match = true;
            else if (sigLower.find("steering") != std::string::npos && propLower.find("steering") != std::string::npos) match = true;
            else if (sigLower.find("odometer") != std::string::npos && propLower.find("odometer") != std::string::npos) match = true;

            if (match) {
                if (createLinkBetweenPins(canNode.outputPinId, vhalNode.inputPinId)) {
                    count++;
                }
            }
        }
    }
    return count;
}

void CanvasView::autoLayoutVertical() {
    ed::SetCurrentEditor(mEditorContext);

    // Group links: layout connected pairs first
    std::unordered_map<uint64_t, bool> nodePlaced;
    float currentY = 100.0f;
    const float deltaY = 165.0f;

    for (const auto& link : mLinks) {
        const CanvasNode* src = findNodeByPin(link.fromPinId);
        const CanvasNode* dst = findNodeByPin(link.toPinId);

        if (src && dst) {
            ed::SetNodePosition(ed::NodeId(src->id), ImVec2(100.0f, currentY));
            ed::SetNodePosition(ed::NodeId(dst->id), ImVec2(650.0f, currentY));
            nodePlaced[src->id] = true;
            nodePlaced[dst->id] = true;
            currentY += deltaY;
        }
    }

    // Place remaining unconnected CAN nodes
    float unconnectedCanY = currentY;
    for (const auto& node : mNodes) {
        if (node.type == CanvasNodeType::CAN_SIGNAL && !nodePlaced[node.id]) {
            ed::SetNodePosition(ed::NodeId(node.id), ImVec2(100.0f, unconnectedCanY));
            unconnectedCanY += deltaY;
        }
    }

    // Place remaining unconnected VHAL nodes
    float unconnectedVhalY = currentY;
    for (const auto& node : mNodes) {
        if (node.type == CanvasNodeType::VHAL_PROPERTY && !nodePlaced[node.id]) {
            ed::SetNodePosition(ed::NodeId(node.id), ImVec2(650.0f, unconnectedVhalY));
            unconnectedVhalY += deltaY;
        }
    }

    navigateToContent();
}

void CanvasView::navigateToContent() {
    if (mEditorContext) {
        ed::SetCurrentEditor(mEditorContext);
        ed::NavigateToContent(0.5f);
    }
}

void CanvasView::populateBenchmark(size_t pairCount) {
    clearCanvas();
    const auto& catalog = VhalCatalog::instance().getAllProperties();
    if (catalog.empty()) return;

    for (size_t i = 0; i < pairCount; ++i) {
        CanMessage msg;
        msg.id = static_cast<uint32_t>(0x100 + i);
        msg.name = "Msg_0x" + [&]() {
            std::ostringstream s; s << std::hex << msg.id; return s.str();
        }();
        msg.dlc = 8;

        CanSignal sig;
        sig.name = "Sig_" + std::to_string(i);
        sig.startBit = static_cast<uint32_t>((i % 4) * 16);
        sig.bitLength = 16;
        sig.endianness = (i % 2 == 0) ? Endianness::LittleEndian : Endianness::BigEndian;
        sig.factor = 0.01;
        sig.offset = 0.0;
        sig.unit = (i % 2 == 0) ? "km/h" : "rpm";

        msg.signals.push_back(sig);

        const auto& prop = catalog[i % catalog.size()];

        uint64_t srcNodeId = addCanSignalNode(msg, sig);
        uint64_t dstNodeId = addVhalPropertyNode(prop);

        const CanvasNode* src = findNode(srcNodeId);
        const CanvasNode* dst = findNode(dstNodeId);
        if (src && dst) {
            createLinkBetweenPins(src->outputPinId, dst->inputPinId);
        }
    }

    autoLayoutVertical();
}

void CanvasView::loadFromReverseResult(const ReverseEngineeringResult& result) {
    clearCanvas();

    for (const auto& link : result.extractedLinks) {
        CanMessage msg;
        msg.id = link.canMsgId;
        msg.name = link.canMsgName;
        msg.dlc = 8;

        CanSignal sig;
        sig.name = link.signalName;
        sig.startBit = 0;
        sig.bitLength = 16;
        sig.unit = (link.transform.conversion == UnitConversion::KMH_TO_MPS) ? "km/h" : "";

        // Check if message was also extracted
        for (const auto& em : result.extractedMessages) {
            if (em.id == link.canMsgId) {
                msg = em;
                for (const auto& es : em.signals) {
                    if (es.name == link.signalName) {
                        sig = es;
                        break;
                    }
                }
                break;
            }
        }

        auto propOpt = VhalCatalog::instance().findByName(link.vhalPropertyName);
        VhalProperty prop;
        if (propOpt) {
            prop = *propOpt;
        } else {
            prop.id = link.vhalPropertyId;
            prop.name = link.vhalPropertyName;
            prop.unit = "value";
        }

        uint64_t srcNodeId = addCanSignalNode(msg, sig);
        uint64_t dstNodeId = addVhalPropertyNode(prop);

        const CanvasNode* src = findNode(srcNodeId);
        const CanvasNode* dst = findNode(dstNodeId);
        if (src && dst) {
            NodeLink activeLink = link;
            activeLink.fromPinId = src->outputPinId;
            activeLink.toPinId = dst->inputPinId;
            addLink(activeLink);
        }
    }

    autoLayoutVertical();
}

void CanvasView::render() {
    ed::SetCurrentEditor(mEditorContext);
    ed::Begin("VhalNodeEditorCanvas");

    // Track active canvas viewport center in canvas coordinates
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    mCanvasScreenCenter = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);
    mCurrentViewCenter = ed::ScreenToCanvas(mCanvasScreenCenter);

    // Track user selection dynamically
    int selCount = ed::GetSelectedObjectCount();
    if (selCount > 0) {
        std::vector<ed::NodeId> selNodes(selCount);
        int nodeCount = ed::GetSelectedNodes(selNodes.data(), selCount);
        for (int i = 0; i < nodeCount; ++i) {
            const CanvasNode* n = findNode(selNodes[i].Get());
            if (n) {
                if (n->type == CanvasNodeType::CAN_SIGNAL) {
                    mLastSelectedCanNodeId = n->id;
                } else if (n->type == CanvasNodeType::VHAL_PROPERTY) {
                    mLastSelectedVhalNodeId = n->id;
                }
            }
        }
    }

    // 1. Render all nodes
    for (const auto& node : mNodes) {
        renderNode(node);
    }

    // 2. Render all links
    renderLinks();

    // 3. Handle interactions (creation / deletion / context menu)
    handleCreationAndDeletion();

    ed::End();
    ed::SetCurrentEditor(nullptr);

    // 4. Render modal popup if active
    renderTransformModal();
}

void CanvasView::renderNode(const CanvasNode& node) {
    ed::BeginNode(ed::NodeId(node.id));

    if (node.type == CanvasNodeType::CAN_SIGNAL) {
        // --- CAN SIGNAL NODE (Blueprint slate-blue header) ---
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 1.00f, 1.0f));
        ImGui::Text("CAN %s", node.canMsgName.c_str());
        ImGui::PopStyleColor();
        ImGui::TextDisabled("ID: 0x%X", node.canMsgId);

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", node.signal.name.c_str());
        ImGui::TextDisabled("Bit %u..%u [%s, %s]",
            node.signal.startBit,
            node.signal.startBit + node.signal.bitLength - 1,
            node.signal.endianness == Endianness::LittleEndian ? "Intel" : "Motorola",
            node.signal.isSigned() ? "Signed" : "Unsigned");

        if (!node.signal.unit.empty()) {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "Unit: %s (x%g)",
                node.signal.unit.c_str(), node.signal.factor);
        }

        // Check if output pin is currently connected
        bool isConnected = false;
        std::string connectedTargetName;
        for (const auto& l : mLinks) {
            if (l.fromPinId == node.outputPinId) {
                isConnected = true;
                connectedTargetName = l.vhalPropertyName;
                break;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Output Pin with glowing socket connector
        ed::BeginPin(ed::PinId(node.outputPinId), ed::PinKind::Output);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 1.0f, 1.0f));
        ImGui::Text("OUTPUT CONNECTOR ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        drawPinSocket(isConnected, IM_COL32(80, 200, 255, 255), IM_COL32(50, 230, 255, 255));
        ed::EndPin();

        if (isConnected) {
            ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.65f, 1.0f), "Linked to: %s", connectedTargetName.c_str());
        } else {
            ImGui::TextDisabled("Drag (●) socket or click below");
        }

        // Direct 1-Click Connect Button
        ImGui::PushID(static_cast<int>(node.id));
        if (ImGui::SmallButton("🔗 Connect to...")) {
            ImGui::OpenPopup("ConnectCanToAospPopup");
        }
        if (ImGui::BeginPopup("ConnectCanToAospPopup")) {
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Connect '%s' to AOSP:", node.signal.name.c_str());
            ImGui::Separator();
            bool hasVhalNode = false;
            for (const auto& vNode : mNodes) {
                if (vNode.type == CanvasNodeType::VHAL_PROPERTY) {
                    hasVhalNode = true;
                    if (ImGui::Selectable(vNode.property.name.c_str())) {
                        createLinkBetweenPins(node.outputPinId, vNode.inputPinId);
                    }
                }
            }
            if (!hasVhalNode) {
                ImGui::TextDisabled("No AOSP Property nodes on canvas.\nAdd properties from Right Panel first.");
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

    } else {
        // --- VHAL PROPERTY NODE (Blueprint emerald-green header) ---
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.95f, 0.65f, 1.0f));
        ImGui::Text("AOSP: %s", node.property.name.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        // Check if input pin is currently connected
        bool isConnected = false;
        std::string connectedSourceName;
        for (const auto& l : mLinks) {
            if (l.toPinId == node.inputPinId) {
                isConnected = true;
                connectedSourceName = l.signalName;
                break;
            }
        }

        // Input Pin with glowing socket connector
        ed::BeginPin(ed::PinId(node.inputPinId), ed::PinKind::Input);
        drawPinSocket(isConnected, IM_COL32(90, 240, 150, 255), IM_COL32(60, 255, 130, 255));
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.95f, 0.65f, 1.0f));
        ImGui::Text(" INPUT CONNECTOR");
        ImGui::PopStyleColor();
        ed::EndPin();

        if (isConnected) {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Linked to: %s", connectedSourceName.c_str());
        } else {
            ImGui::TextDisabled("Drop (●) socket or click below");
        }

        // Direct 1-Click Connect Button
        ImGui::PushID(static_cast<int>(node.id));
        if (ImGui::SmallButton("🔗 Connect to...")) {
            ImGui::OpenPopup("ConnectAospToCanPopup");
        }
        if (ImGui::BeginPopup("ConnectAospToCanPopup")) {
            ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.65f, 1.0f), "Connect '%s' from CAN Signal:", node.property.name.c_str());
            ImGui::Separator();
            bool hasCanNode = false;
            for (const auto& cNode : mNodes) {
                if (cNode.type == CanvasNodeType::CAN_SIGNAL) {
                    hasCanNode = true;
                    std::string label = cNode.canMsgName + " -> " + cNode.signal.name;
                    if (ImGui::Selectable(label.c_str())) {
                        createLinkBetweenPins(cNode.outputPinId, node.inputPinId);
                    }
                }
            }
            if (!hasCanNode) {
                ImGui::TextDisabled("No CAN Signal nodes on canvas.\nAdd signals from Left Panel first.");
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("AIDL: %s", node.property.idHexStr().c_str());
        ImGui::TextColored(ImVec4(0.92f, 0.82f, 0.45f, 1.0f), "Type: %s",
            vhalTypeToString(node.property.type));
        if (!node.property.unit.empty()) {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "Unit: %s",
                node.property.unit.c_str());
        }
        ImGui::TextDisabled("Category: %s", node.property.category.c_str());
    }

    ed::EndNode();
}

void CanvasView::renderLinks() {
    for (const auto& link : mLinks) {
        // Color based on transform status
        ImColor linkColor(100, 200, 255, 255);
        if (link.transform.conversion != UnitConversion::NONE) {
            linkColor = ImColor(255, 215, 0, 255); // Gold for transformed links
        }

        ed::Link(ed::LinkId(link.id),
                 ed::PinId(link.fromPinId),
                 ed::PinId(link.toPinId),
                 linkColor, 3.0f);

        // Tooltip on hover
        if (ed::GetHoveredLink() == ed::LinkId(link.id)) {
            ImGui::BeginTooltip();
            ImGui::Text("Link: CAN [%s] -> AOSP [%s]", link.signalName.c_str(), link.vhalPropertyName.c_str());
            ImGui::Text("Transform: %s", unitConversionToString(link.transform.conversion));
            if (!link.transform.customExpression.empty()) {
                ImGui::Text("Custom Expr: %s", link.transform.customExpression.c_str());
            }
            if (link.transform.hasClamp) {
                ImGui::Text("Clamp: [%g .. %g]", link.transform.clampMin, link.transform.clampMax);
            }
            ImGui::TextDisabled("(Double-click link to configure transformation)");
            ImGui::EndTooltip();
        }
    }
}

void CanvasView::handleCreationAndDeletion() {
    // 1. Link Creation with vivid visual feedback
    if (ed::BeginCreate(ImColor(80, 240, 150), 3.5f)) {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            auto srcNode = findNodeByPin(startPinId.Get());
            auto dstNode = findNodeByPin(endPinId.Get());

            if (srcNode && dstNode) {
                if (srcNode->id == dstNode->id) {
                    ed::RejectNewItem(ImColor(255, 60, 60), 2.5f);
                } else if (srcNode->type == dstNode->type) {
                    ed::RejectNewItem(ImColor(255, 60, 60), 2.5f);
                } else {
                    // Valid connection! Glowing green curve
                    if (ed::AcceptNewItem(ImColor(60, 255, 120), 4.5f)) {
                        createLinkBetweenPins(startPinId.Get(), endPinId.Get());
                    }
                }
            }
        }
    }
    ed::EndCreate();

    // 2. Link & Node Deletion
    if (ed::BeginDelete()) {
        ed::LinkId delLinkId;
        while (ed::QueryDeletedLink(&delLinkId)) {
            if (ed::AcceptDeletedItem()) {
                removeLink(delLinkId.Get());
            }
        }

        ed::NodeId delNodeId;
        while (ed::QueryDeletedNode(&delNodeId)) {
            if (ed::AcceptDeletedItem()) {
                removeNode(delNodeId.Get());
            }
        }
    }
    ed::EndDelete();

    // 3. Double-click link to open transform modal
    ed::LinkId dblClickedLinkId = ed::GetDoubleClickedLink();
    if (dblClickedLinkId) {
        for (auto& l : mLinks) {
            if (l.id == dblClickedLinkId.Get()) {
                mEditingLink = &l;
                mTempTransform = l.transform;
                mShowTransformModal = true;
                break;
            }
        }
    }
}

void CanvasView::renderTransformModal() {
    if (!mShowTransformModal || !mEditingLink) return;

    ImGui::OpenPopup("Configure Link Transformation");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 420));

    if (ImGui::BeginPopupModal("Configure Link Transformation", &mShowTransformModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Mapping: CAN [%s] -> AOSP [%s]",
            mEditingLink->signalName.c_str(),
            mEditingLink->vhalPropertyName.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        // 1. Conversion Preset Combo
        const char* conversions[] = {
            "None (Direct Passthrough)",
            "km/h -> m/s (/ 3.6)",
            "m/s -> km/h (* 3.6)",
            "Celsius -> Kelvin (+ 273.15)",
            "Kelvin -> Celsius (- 273.15)",
            "RPM -> rad/s (* 0.10472)",
            "Custom Math Expression"
        };
        int currentConvIdx = static_cast<int>(mTempTransform.conversion);
        if (ImGui::Combo("Conversion", &currentConvIdx, conversions, IM_COUNTOF(conversions))) {
            mTempTransform.conversion = static_cast<UnitConversion>(currentConvIdx);
        }

        // Custom expression input if selected
        if (mTempTransform.conversion == UnitConversion::CUSTOM_EXPR) {
            char exprBuf[256];
            std::snprintf(exprBuf, sizeof(exprBuf), "%s", mTempTransform.customExpression.c_str());
            if (ImGui::InputText("Expression", exprBuf, sizeof(exprBuf))) {
                mTempTransform.customExpression = exprBuf;
            }
            ImGui::TextDisabled("Example: (rawVal * 0.1f) + 5.0f");
        }

        // 2. Custom Scale & Offset
        float scale = static_cast<float>(mTempTransform.customScale);
        float offset = static_cast<float>(mTempTransform.customOffset);
        if (ImGui::DragFloat("Custom Scale", &scale, 0.01f, 0.0001f, 1000.0f, "%.4f")) {
            mTempTransform.customScale = scale;
        }
        if (ImGui::DragFloat("Custom Offset", &offset, 0.1f, -10000.0f, 10000.0f, "%.4f")) {
            mTempTransform.customOffset = offset;
        }

        // 3. Clamping
        ImGui::Checkbox("Enable Value Clamping", &mTempTransform.hasClamp);
        if (mTempTransform.hasClamp) {
            float cMin = static_cast<float>(mTempTransform.clampMin);
            float cMax = static_cast<float>(mTempTransform.clampMax);
            if (ImGui::DragFloat("Clamp Min", &cMin, 0.5f, -10000.0f, cMax)) {
                mTempTransform.clampMin = cMin;
            }
            if (ImGui::DragFloat("Clamp Max", &cMax, 0.5f, cMin, 10000.0f)) {
                mTempTransform.clampMax = cMax;
            }
        }

        // 4. Area ID
        int area = mTempTransform.areaId;
        if (ImGui::InputInt("Area ID (0 for Global)", &area)) {
            mTempTransform.areaId = area;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            mEditingLink->transform = mTempTransform;
            mShowTransformModal = false;
            if (mOnLinksChanged) mOnLinksChanged();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            mShowTransformModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace can_vhal
