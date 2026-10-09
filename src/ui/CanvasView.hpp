#pragma once

#include "core/AutomotiveTypes.hpp"
#include "core/CppReverseParser.hpp"
#include <imgui.h>
#include <imgui_node_editor.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

namespace can_vhal {

class CanvasView {
public:
    using OnLinksChangedCallback = std::function<void()>;

    CanvasView();
    ~CanvasView();

    void init();
    void render();

    // Node & Link Operations
    uint64_t addCanSignalNode(const CanMessage& msg, const CanSignal& sig, float posX = 0.0f, float posY = 0.0f);
    uint64_t addVhalPropertyNode(const VhalProperty& prop, float posX = 0.0f, float posY = 0.0f);
    void removeNode(uint64_t nodeId);
    void removeLink(uint64_t linkId);

    void addLink(const NodeLink& link);
    bool createLinkBetweenPins(uint64_t fromPinId, uint64_t toPinId);

    // Fast Connection Tools
    bool connectSelectedNodes(std::string* outStatusMessage = nullptr);
    size_t autoConnectMatchingNames();

    void autoLayoutVertical();
    void clearCanvas();
    void populateBenchmark(size_t pairCount = 150);
    void loadFromReverseResult(const ReverseEngineeringResult& result);

    const std::vector<CanvasNode>& getNodes() const { return mNodes; }
    const std::vector<NodeLink>& getLinks() const { return mLinks; }
    std::vector<NodeLink>& getLinks() { return mLinks; }

    const CanvasNode* findNode(uint64_t nodeId) const;
    const CanvasNode* findNodeByPin(uint64_t pinId) const;

    void navigateToContent();

    void setOnLinksChanged(OnLinksChangedCallback cb) { mOnLinksChanged = std::move(cb); }

private:
    void renderNode(const CanvasNode& node);
    void renderLinks();
    void handleCreationAndDeletion();
    void renderTransformModal();

    ax::NodeEditor::EditorContext* mEditorContext = nullptr;
    std::vector<CanvasNode> mNodes;
    std::vector<NodeLink> mLinks;
    uint64_t mNextId = 1;

    // Selection & View Tracking
    uint64_t mLastSelectedCanNodeId = 0;
    uint64_t mLastSelectedVhalNodeId = 0;
    ImVec2 mCanvasScreenCenter = ImVec2(0.0f, 0.0f);
    ImVec2 mCurrentViewCenter = ImVec2(350.0f, 250.0f);
    uint32_t mSpawnCount = 0;

    // Pin to Node ID lookup
    std::unordered_map<uint64_t, uint64_t> mPinToNode;

    // Modal state for link transformation editing
    bool mShowTransformModal = false;
    NodeLink* mEditingLink = nullptr;
    TransformConfig mTempTransform;

    OnLinksChangedCallback mOnLinksChanged;
};

} // namespace can_vhal
