#pragma once

#include "core/AutomotiveTypes.hpp"
#include "core/DbcParser.hpp"
#include "core/CppReverseParser.hpp"
#include "core/CppSynthesizer.hpp"
#include "SignalPalette.hpp"
#include "PropertyPalette.hpp"
#include "CanvasView.hpp"

#include <string>
#include <vector>
#include <memory>

struct GLFWwindow;
struct ImFont;

namespace can_vhal {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool init(int width = 1600, int height = 960, const char* title = "CAN-to-AOSP VHAL Mapping Studio");
    void run(int maxFrames = 0);
    void shutdown();

    // Direct programmatic APIs for testing/benchmarking
    bool loadDbcFile(const std::string& filePath);
    bool importCppFile(const std::string& filePath);
    std::string synthesizeCode();
    void runBenchmark(size_t pairCount = 150);

    // Font & UI scaling
    void setUiScale(float scale);
    float getUiScale() const { return mUiScale; }

private:
    void initFonts();
    void renderUI();
    void renderMenuBar();
    void renderLeftPanel();
    void renderCenterPanel();
    void renderRightPanel();
    void renderBottomPanel();
    void renderFileDialogModal();
    void renderAboutModal();

    GLFWwindow* mWindow = nullptr;
    int mWidth = 1600;
    int mHeight = 960;

    // Font state
    float mUiScale = 1.15f;
    ImFont* mMainFont = nullptr;
    ImFont* mCodeFont = nullptr;

    // Core Engines
    DbcParser mDbcParser;
    CppReverseParser mReverseParser;
    CppSynthesizer mSynthesizer;

    // UI Sub-components
    SignalPalette mSignalPalette;
    PropertyPalette mPropertyPalette;
    CanvasView mCanvasView;

    // Generated Code State
    std::string mGeneratedCode;
    std::string mStatusMessage = "Ready";
    float mStatusMessageTimer = 0.0f;

    // Dialog state
    bool mShowFileDialog = false;
    enum class FileDialogMode { OPEN_DBC, IMPORT_CPP, SAVE_CPP } mDialogMode = FileDialogMode::OPEN_DBC;
    char mFilePathBuffer[512] = "";
    bool mShowAboutModal = false;
};

} // namespace can_vhal
