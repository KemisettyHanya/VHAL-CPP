#include "AppWindow.hpp"
#include "core/PathUtils.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace can_vhal {

namespace {

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void setupModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    // Generous padding for crisp readability on high-resolution displays
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(10.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 18.0f;
    style.GrabMinSize = 14.0f;

    // Automotive Dark Blueprint Palette
    colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.60f, 0.66f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.11f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.17f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.29f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.34f, 0.44f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.26f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.34f, 0.42f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.50f, 0.62f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.30f, 0.85f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.30f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.24f, 0.33f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.36f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.32f, 0.44f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.19f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.36f, 0.49f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.32f, 0.44f, 0.60f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.22f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.30f, 0.45f, 0.65f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.35f, 0.55f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.25f, 0.35f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.35f, 0.48f, 0.62f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.60f, 0.78f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.19f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.25f, 0.35f, 0.48f, 1.00f);
    colors[ImGuiCol_TabSelected]           = ImVec4(0.22f, 0.30f, 0.42f, 1.00f);
}

} // namespace

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    shutdown();
}

void AppWindow::initFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const float baseFontSize = 18.0f;
    const float codeFontSize = 16.0f;

    // List of candidate system UI font paths (Windows & Linux)
    static const char* const candidateFontPaths[] = {
        // Windows standard UI fonts
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
        // Linux standard UI fonts
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        nullptr
    };

    // List of candidate monospace code font paths
    static const char* const candidateCodeFontPaths[] = {
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\cour.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        nullptr
    };

    ImFontConfig fontCfg;
    fontCfg.OversampleH = 2;
    fontCfg.OversampleV = 1;
    fontCfg.PixelSnapH = false;
    fontCfg.SizePixels = baseFontSize;

    mMainFont = nullptr;
    for (int i = 0; candidateFontPaths[i] != nullptr; ++i) {
        std::ifstream f(candidateFontPaths[i]);
        if (f.good()) {
            mMainFont = io.Fonts->AddFontFromFileTTF(candidateFontPaths[i], baseFontSize, &fontCfg);
            if (mMainFont) {
                std::cout << "[UI] Loaded system UI font: " << candidateFontPaths[i] << " (" << baseFontSize << "px)" << std::endl;
                break;
            }
        }
    }

    if (!mMainFont) {
        fontCfg.SizePixels = baseFontSize;
        mMainFont = io.Fonts->AddFontDefaultVector(&fontCfg);
        std::cout << "[UI] Loaded embedded scalable UI font (" << baseFontSize << "px)" << std::endl;
    }

    // Monospace code font
    ImFontConfig codeCfg;
    codeCfg.OversampleH = 2;
    codeCfg.OversampleV = 1;
    codeCfg.SizePixels = codeFontSize;

    mCodeFont = nullptr;
    for (int i = 0; candidateCodeFontPaths[i] != nullptr; ++i) {
        std::ifstream f(candidateCodeFontPaths[i]);
        if (f.good()) {
            mCodeFont = io.Fonts->AddFontFromFileTTF(candidateCodeFontPaths[i], codeFontSize, &codeCfg);
            if (mCodeFont) {
                std::cout << "[UI] Loaded code font: " << candidateCodeFontPaths[i] << " (" << codeFontSize << "px)" << std::endl;
                break;
            }
        }
    }

    if (!mCodeFont) {
        codeCfg.SizePixels = codeFontSize;
        mCodeFont = io.Fonts->AddFontDefaultVector(&codeCfg);
    }

    io.FontDefault = mMainFont;

    // Detect system DPI scaling
    float xscale = 1.0f, yscale = 1.0f;
    if (mWindow) {
        glfwGetWindowContentScale(mWindow, &xscale, &yscale);
    }
    if (xscale > 1.05f) {
        mUiScale = xscale;
    } else {
        mUiScale = 1.15f; // Default comfortable scaling
    }
    io.FontGlobalScale = mUiScale;
}

void AppWindow::setUiScale(float scale) {
    mUiScale = std::clamp(scale, 0.80f, 2.50f);
    ImGui::GetIO().FontGlobalScale = mUiScale;
    mStatusMessage = "UI Font Scale set to " + std::to_string(static_cast<int>(mUiScale * 100.0f)) + "%";
    mStatusMessageTimer = 3.0f;
}

bool AppWindow::init(int width, int height, const char* title) {
    mWidth = width;
    mHeight = height;

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(mWidth, mHeight, title, nullptr, nullptr);
    if (!mWindow) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(1); // Enable V-Sync for rock-solid 60 FPS

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    setupModernDarkTheme();
    initFonts();

    ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize node editor
    mCanvasView.init();
    mCanvasView.setOnLinksChanged([this]() {
        synthesizeCode();
    });

    // Auto-load full production vehicle DBC by default
    std::string defaultDbcPath = resolvePath("sample_data/full_vehicle_production.dbc");
    if (!loadDbcFile(defaultDbcPath)) {
        // Fallback to complete powertrain if full not available
        loadDbcFile(resolvePath("sample_data/complete_vehicle_powertrain.dbc"));
    }

    return true;
}

void AppWindow::shutdown() {
    if (mWindow) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
        glfwTerminate();
    }
}

bool AppWindow::loadDbcFile(const std::string& filePath) {
    std::string err;
    if (mDbcParser.parseFile(filePath, err)) {
        mStatusMessage = "Loaded DBC: " + filePath + " (" +
            std::to_string(mDbcParser.getMessages().size()) + " msgs, " +
            std::to_string(mDbcParser.totalSignalsCount()) + " sigs)";
        mStatusMessageTimer = 5.0f;
        return true;
    } else {
        mStatusMessage = "Error loading DBC: " + err;
        mStatusMessageTimer = 5.0f;
        return false;
    }
}

bool AppWindow::importCppFile(const std::string& filePath) {
    auto res = mReverseParser.parseFile(filePath, mDbcParser.getMessages());
    if (res.success) {
        mCanvasView.loadFromReverseResult(res);
        mGeneratedCode = synthesizeCode();
        mStatusMessage = "Reconstituted " + std::to_string(res.extractedLinks.size()) +
            " links from legacy C++ (" + filePath + ")";
        mStatusMessageTimer = 5.0f;
        return true;
    } else {
        mStatusMessage = "Reverse parsing failed: " + res.errorMessage;
        mStatusMessageTimer = 5.0f;
        return false;
    }
}

std::string AppWindow::synthesizeCode() {
    mGeneratedCode = mSynthesizer.synthesize(
        mCanvasView.getLinks(),
        mDbcParser.getMessages());
    return mGeneratedCode;
}

void AppWindow::runBenchmark(size_t pairCount) {
    mCanvasView.populateBenchmark(pairCount);
    mGeneratedCode = synthesizeCode();
    mStatusMessage = "Benchmark loaded: " + std::to_string(pairCount * 2) +
        " nodes + " + std::to_string(pairCount) + " bezier links.";
    mStatusMessageTimer = 5.0f;
}

void AppWindow::run(int maxFrames) {
    int frameCount = 0;
    while (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(mWindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.11f, 0.13f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(mWindow);

        if (maxFrames > 0 && ++frameCount >= maxFrames) {
            std::cout << "[GUI] Successfully rendered " << frameCount << " frames. Exiting test loop." << std::endl;
            break;
        }
    }
}

void AppWindow::renderUI() {
    // Process Font Scale Keyboard Shortcuts: Ctrl + '+', Ctrl + '-', Ctrl + '0'
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Equal)) {
        setUiScale(mUiScale + 0.15f);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Minus)) {
        setUiScale(mUiScale - 0.15f);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_0)) {
        setUiScale(1.15f);
    }

    // Direct Action Shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        synthesizeCode();
        mStatusMessage = "Synthesized C++ code (F5)!";
        mStatusMessageTimer = 3.0f;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        mDialogMode = FileDialogMode::IMPORT_CPP;
        std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vehicle_vhal.cpp").c_str());
        mShowFileDialog = true;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        mDialogMode = FileDialogMode::SAVE_CPP;
        std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
        mShowFileDialog = true;
    }

    renderMenuBar();

    // Full screen background host window for the 3 panels + bottom panel
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus |
                                 ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::Begin("MainStudioHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    float totalWidth = viewport->WorkSize.x - 8.0f;
    float totalHeight = viewport->WorkSize.y - 8.0f;

    float bottomPanelHeight = 250.0f;
    float topPanelsHeight = totalHeight - bottomPanelHeight - 8.0f;

    // Generous panel widths for readable typography
    float leftPanelWidth = 430.0f;
    float rightPanelWidth = 410.0f;
    float centerPanelWidth = totalWidth - leftPanelWidth - rightPanelWidth - 16.0f;

    // --- TOP ROW: 3 PANELS ---
    // 1. LEFT PANEL: Source CAN Signals
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 4.0f, viewport->WorkPos.y + 4.0f));
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, topPanelsHeight));
    ImGui::Begin("LeftPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    renderLeftPanel();
    ImGui::End();

    // 2. CENTER PANEL: Node Editor Canvas
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + leftPanelWidth + 12.0f, viewport->WorkPos.y + 4.0f));
    ImGui::SetNextWindowSize(ImVec2(centerPanelWidth, topPanelsHeight));
    ImGui::Begin("CenterPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    renderCenterPanel();
    ImGui::End();

    // 3. RIGHT PANEL: Target AOSP Properties
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + leftPanelWidth + centerPanelWidth + 20.0f, viewport->WorkPos.y + 4.0f));
    ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, topPanelsHeight));
    ImGui::Begin("RightPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    renderRightPanel();
    ImGui::End();

    // --- BOTTOM ROW: GENERATED CODE PREVIEW & TOOLBAR ---
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 4.0f, viewport->WorkPos.y + topPanelsHeight + 12.0f));
    ImGui::SetNextWindowSize(ImVec2(totalWidth, bottomPanelHeight));
    ImGui::Begin("BottomPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    renderBottomPanel();
    ImGui::End();

    ImGui::End(); // MainStudioHost

    // Modals
    renderFileDialogModal();
    renderAboutModal();
}

void AppWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open DBC File...")) {
                mDialogMode = FileDialogMode::OPEN_DBC;
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/full_vehicle_production.dbc").c_str());
                mShowFileDialog = true;
            }
            if (ImGui::MenuItem("Load Full Production DBC (70+ signals) [Default]")) {
                loadDbcFile(resolvePath("sample_data/full_vehicle_production.dbc"));
            }
            if (ImGui::MenuItem("Load Powertrain DBC (16 signals)")) {
                loadDbcFile(resolvePath("sample_data/complete_vehicle_powertrain.dbc"));
            }
            if (ImGui::MenuItem("Load Speed Sample DBC (4 signals)")) {
                loadDbcFile(resolvePath("sample_data/sample_vehicle_speed.dbc"));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open Generated / Legacy C++ File (Load Connections)...", "Ctrl+O")) {
                mDialogMode = FileDialogMode::IMPORT_CPP;
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vehicle_vhal.cpp").c_str());
                mShowFileDialog = true;
            }
            if (ImGui::MenuItem("Load Full Vehicle Legacy C++ (Powertrain)")) {
                importCppFile(resolvePath("sample_data/legacy_vehicle_vhal.cpp"));
            }
            if (ImGui::MenuItem("Load Speed Legacy C++ (Simple)")) {
                importCppFile(resolvePath("sample_data/legacy_vhal_speed.cpp"));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Generated C++ Code to File...", "Ctrl+S")) {
                mDialogMode = FileDialogMode::SAVE_CPP;
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
                mShowFileDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                if (mWindow) glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Generate")) {
            if (ImGui::MenuItem("⚡ Generate C++ Code (Now)", "F5")) {
                synthesizeCode();
                mStatusMessage = "Synthesized C++ code!";
                mStatusMessageTimer = 3.0f;
            }
            if (ImGui::MenuItem("💾 Save Generated C++ to File...", "Ctrl+S")) {
                mDialogMode = FileDialogMode::SAVE_CPP;
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
                mShowFileDialog = true;
            }
            if (ImGui::MenuItem("📋 Copy Generated Code to Clipboard")) {
                if (!mGeneratedCode.empty()) {
                    ImGui::SetClipboardText(mGeneratedCode.c_str());
                    mStatusMessage = "Code copied to clipboard!";
                    mStatusMessageTimer = 3.0f;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::TextDisabled("UI & Font Scaling");
            ImGui::Separator();
            if (ImGui::MenuItem("100% (Compact)", nullptr, std::abs(mUiScale - 1.0f) < 0.05f)) {
                setUiScale(1.0f);
            }
            if (ImGui::MenuItem("115% (Comfortable)", nullptr, std::abs(mUiScale - 1.15f) < 0.05f)) {
                setUiScale(1.15f);
            }
            if (ImGui::MenuItem("125% (Large)", nullptr, std::abs(mUiScale - 1.25f) < 0.05f)) {
                setUiScale(1.25f);
            }
            if (ImGui::MenuItem("140% (Extra Large)", nullptr, std::abs(mUiScale - 1.40f) < 0.05f)) {
                setUiScale(1.40f);
            }
            if (ImGui::MenuItem("160% (HiDPI / 2K)", nullptr, std::abs(mUiScale - 1.60f) < 0.05f)) {
                setUiScale(1.60f);
            }
            if (ImGui::MenuItem("200% (Ultra / 4K)", nullptr, std::abs(mUiScale - 2.00f) < 0.05f)) {
                setUiScale(2.00f);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Zoom In (Ctrl + =)", "Ctrl++")) {
                setUiScale(mUiScale + 0.15f);
            }
            if (ImGui::MenuItem("Zoom Out (Ctrl + -)", "Ctrl+-")) {
                setUiScale(mUiScale - 0.15f);
            }
            if (ImGui::MenuItem("Reset Zoom", "Ctrl+0")) {
                setUiScale(1.15f);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Auto-Layout Vertical Stack")) {
                mCanvasView.autoLayoutVertical();
            }
            if (ImGui::MenuItem("Generate 300-Node Benchmark (60 FPS Test)")) {
                runBenchmark(150);
            }
            if (ImGui::MenuItem("Synthesize C++ AIDL Code")) {
                synthesizeCode();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Canvas")) {
                mCanvasView.clearCanvas();
                mGeneratedCode.clear();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About CAN-to-AOSP VHAL Studio")) {
                mShowAboutModal = true;
            }
            ImGui::EndMenu();
        }

        // Live stats in menu bar
        ImGui::SameLine(ImGui::GetWindowWidth() - 360.0f);
        ImGui::TextDisabled("Scale: %d%% | FPS: %.1f | Nodes: %zu | Links: %zu",
            static_cast<int>(mUiScale * 100.0f),
            ImGui::GetIO().Framerate,
            mCanvasView.getNodes().size(),
            mCanvasView.getLinks().size());

        ImGui::EndMainMenuBar();
    }
}

void AppWindow::renderLeftPanel() {
    mSignalPalette.render(
        mDbcParser.getMessages(),
        [this](const CanMessage& msg, const CanSignal& sig) {
            mCanvasView.addCanSignalNode(msg, sig);
        },
        [this](const CanMessage& msg) {
            for (const auto& sig : msg.signals) {
                mCanvasView.addCanSignalNode(msg, sig);
            }
            mCanvasView.autoLayoutVertical();
        }
    );
}

void AppWindow::renderCenterPanel() {
    // 1. Prominent Primary Generation Button (Green)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.58f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.72f, 0.42f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.85f, 0.50f, 1.0f));
    if (ImGui::Button("⚡ GENERATE C++ CODE", ImVec2(190.0f, 32.0f))) {
        synthesizeCode();
        mStatusMessage = "Synthesized " + std::to_string(mCanvasView.getLinks().size()) + " mappings into C++ code!";
        mStatusMessageTimer = 4.0f;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    // 2. Open / Load C++ File to Reconstruct Connections (Blue)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.38f, 0.60f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.50f, 0.75f, 1.0f));
    if (ImGui::Button("📁 Load C++ File / Reconstruct", ImVec2(240.0f, 32.0f))) {
        mDialogMode = FileDialogMode::IMPORT_CPP;
        std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vehicle_vhal.cpp").c_str());
        mShowFileDialog = true;
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open any generated or legacy C++ file to reconstruct all visual nodes and connections!");
    }

    ImGui::SameLine();
    // 3. Connect Selected Nodes
    if (ImGui::Button("🔗 Connect Selected", ImVec2(145.0f, 32.0f))) {
        if (mCanvasView.connectSelectedNodes()) {
            synthesizeCode();
            mStatusMessage = "Connected selected nodes!";
            mStatusMessageTimer = 3.0f;
        } else {
            mStatusMessage = "Tip: Select 1 CAN node and 1 AOSP node to connect";
            mStatusMessageTimer = 3.0f;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Connect the selected CAN signal node to the selected AOSP property node");
    }

    ImGui::SameLine();
    // 4. Auto-Connect Matching Names
    if (ImGui::Button("⚡ Auto-Connect Matching", ImVec2(185.0f, 32.0f))) {
        size_t count = mCanvasView.autoConnectMatchingNames();
        synthesizeCode();
        mStatusMessage = "Auto-connected " + std::to_string(count) + " matching signal/property pairs!";
        mStatusMessageTimer = 3.0f;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Auto-match and connect signals with properties based on matching names and domains");
    }

    ImGui::SameLine();
    // 5. Layout & Navigation buttons
    if (ImGui::Button("Auto-Layout", ImVec2(100.0f, 32.0f))) {
        mCanvasView.autoLayoutVertical();
    }

    ImGui::SameLine();
    if (ImGui::Button("Zoom Fit", ImVec2(80.0f, 32.0f))) {
        mCanvasView.navigateToContent();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(60.0f, 32.0f))) {
        mCanvasView.clearCanvas();
        mGeneratedCode.clear();
    }

    ImGui::Separator();
    mCanvasView.render();
}

void AppWindow::renderRightPanel() {
    mPropertyPalette.render(
        [this](const VhalProperty& prop) {
            mCanvasView.addVhalPropertyNode(prop);
        }
    );
}

void AppWindow::renderBottomPanel() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 1.00f, 1.00f));
    ImGui::Text("LIVE GENERATED AOSP C++ CODE PREVIEW (onCanFrameReceived)");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // Prominent Generate Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.58f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.72f, 0.42f, 1.0f));
    if (ImGui::Button("⚡ GENERATE CODE NOW", ImVec2(180.0f, 0.0f))) {
        synthesizeCode();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    if (ImGui::Button("📁 Load C++ File...")) {
        mDialogMode = FileDialogMode::IMPORT_CPP;
        std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vehicle_vhal.cpp").c_str());
        mShowFileDialog = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("💾 Save to File...")) {
        mDialogMode = FileDialogMode::SAVE_CPP;
        std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
        mShowFileDialog = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("📋 Copy to Clipboard")) {
        if (!mGeneratedCode.empty()) {
            ImGui::SetClipboardText(mGeneratedCode.c_str());
            mStatusMessage = "Code copied to clipboard!";
            mStatusMessageTimer = 3.0f;
        }
    }

    // Status indicator
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 320.0f);
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", mStatusMessage.c_str());

    ImGui::Separator();

    if (mGeneratedCode.empty()) {
        mGeneratedCode = "// Connect CAN Signal pins to AOSP Property pins, then click '⚡ GENERATE C++ CODE'.";
    }

    if (mCodeFont) ImGui::PushFont(mCodeFont);
    ImVec2 codeAreaSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 4.0f);
    ImGuiInputTextFlags codeFlags = ImGuiInputTextFlags_ReadOnly;
    ImGui::InputTextMultiline("##GeneratedCodeView", const_cast<char*>(mGeneratedCode.data()),
        mGeneratedCode.size() + 1, codeAreaSize, codeFlags);
    if (mCodeFont) ImGui::PopFont();
}

void AppWindow::renderFileDialogModal() {
    if (!mShowFileDialog) return;

    const char* title = "Open File";
    if (mDialogMode == FileDialogMode::OPEN_DBC) title = "Open CAN DBC File";
    else if (mDialogMode == FileDialogMode::IMPORT_CPP) title = "Open Generated or Legacy C++ VHAL File";
    else if (mDialogMode == FileDialogMode::SAVE_CPP) title = "Save Generated C++ Code";

    ImGui::OpenPopup(title);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640, 250));

    if (ImGui::BeginPopupModal(title, &mShowFileDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter file path or pick a quick preset below:");
        ImGui::Spacing();

        ImGui::InputText("Path", mFilePathBuffer, sizeof(mFilePathBuffer));

        ImGui::Spacing();
        ImGui::TextDisabled("Quick Presets:");
        if (mDialogMode == FileDialogMode::OPEN_DBC) {
            if (ImGui::Button("Full Production DBC (70+ signals)")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/full_vehicle_production.dbc").c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Powertrain DBC (16 signals)")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/complete_vehicle_powertrain.dbc").c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Speed DBC (4 signals)")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/sample_vehicle_speed.dbc").c_str());
            }
        } else if (mDialogMode == FileDialogMode::IMPORT_CPP) {
            if (ImGui::Button("generated_vhal_mapping.cpp")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
            }
            ImGui::SameLine();
            if (ImGui::Button("Full Vehicle Legacy C++")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vehicle_vhal.cpp").c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Speed Legacy C++")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "%s", resolvePath("sample_data/legacy_vhal_speed.cpp").c_str());
            }
        } else if (mDialogMode == FileDialogMode::SAVE_CPP) {
            if (ImGui::Button("generated_vhal_mapping.cpp")) {
                std::snprintf(mFilePathBuffer, sizeof(mFilePathBuffer), "generated_vhal_mapping.cpp");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            std::string path(mFilePathBuffer);
            if (mDialogMode == FileDialogMode::OPEN_DBC) {
                loadDbcFile(path);
            } else if (mDialogMode == FileDialogMode::IMPORT_CPP) {
                importCppFile(path);
            } else if (mDialogMode == FileDialogMode::SAVE_CPP) {
                std::ofstream out(path);
                if (out.is_open()) {
                    out << mGeneratedCode;
                    mStatusMessage = "Exported to: " + path;
                } else {
                    mStatusMessage = "Failed to write: " + path;
                }
            }
            mShowFileDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            mShowFileDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void AppWindow::renderAboutModal() {
    if (!mShowAboutModal) return;

    ImGui::OpenPopup("About CAN-to-AOSP VHAL Studio");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 320));

    if (ImGui::BeginPopupModal("About CAN-to-AOSP VHAL Studio", &mShowAboutModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Native C++ CAN-to-AOSP VHAL Mapping Studio");
        ImGui::Text("Version 1.1.0 (High-DPI Font & Full Vehicle Production Edition)");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BulletText("Standalone native C++20 desktop application.");
        ImGui::BulletText("Vector DBC Parser & AST Synthesizer for Android 13/14+ AIDL IVehicle.");
        ImGui::BulletText("C++ Reverse-engineering AST parser for legacy onCanFrameReceived.");
        ImGui::BulletText("Interactive node-graph visual workspace with cubic Bezier routing.");
        ImGui::BulletText("Dynamic UI font scaling (100%% - 200%%) & HiDPI display support.");
        ImGui::BulletText("Full cross-platform support: Linux (GCC/Clang) and Windows (MSVC/MinGW).");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            mShowAboutModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace can_vhal

