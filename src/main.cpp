#include "ui/AppWindow.hpp"
#include "core/DbcParser.hpp"
#include "core/CppReverseParser.hpp"
#include "core/CppSynthesizer.hpp"
#include "core/VhalCatalog.hpp"
#include "core/PathUtils.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

int main(int argc, char** argv) {
    std::cout << "========================================================" << std::endl;
    std::cout << " Native C++ CAN-to-AOSP VHAL Mapping Studio (Dear ImGui) " << std::endl;
    std::cout << "========================================================" << std::endl;

    // Check for CLI arguments
    int maxFrames = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--benchmark") {
            std::cout << "[CLI] Running 300+ Node Benchmark headlessly..." << std::endl;
            can_vhal::AppWindow app;
            auto start = std::chrono::high_resolution_clock::now();
            app.runBenchmark(150);
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "[CLI] Benchmark completed in " << ms << " ms." << std::endl;
            return 0;
        } else if (arg == "--test" || arg == "--verify") {
            std::cout << "[CLI] Running Full Verification..." << std::endl;
            can_vhal::DbcParser parser;
            std::string err;
            if (!parser.parseFile(can_vhal::resolvePath("sample_data/full_vehicle_production.dbc"), err)) {
                std::cerr << "DBC parsing failed: " << err << std::endl;
                return 1;
            }
            std::cout << "[CLI] Parsed Production DBC successfully: " << parser.getMessages().size()
                      << " messages, " << parser.totalSignalsCount() << " signals." << std::endl;

            can_vhal::CppReverseParser rev;
            auto revRes = rev.parseFile(can_vhal::resolvePath("sample_data/legacy_vehicle_vhal.cpp"), parser.getMessages());
            if (!revRes.success) {
                std::cerr << "Reverse parsing failed: " << revRes.errorMessage << std::endl;
                return 1;
            }
            std::cout << "[CLI] Reverse engineering succeeded: " << revRes.extractedLinks.size()
                      << " links extracted." << std::endl;

            can_vhal::CppSynthesizer synth;
            std::string synthCode = synth.synthesize(revRes.extractedLinks, parser.getMessages());
            std::cout << "[CLI] Synthesized " << synthCode.size() << " bytes of AOSP C++ code." << std::endl;
            return 0;
        } else if (arg == "--frames" && i + 1 < argc) {
            maxFrames = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: can_vhal_studio [options]\n"
                      << "Options:\n"
                      << "  --benchmark    Run 300+ node benchmark\n"
                      << "  --verify       Run verification of sample DBC and legacy C++\n"
                      << "  --frames <N>   Render N frames in GUI mode then exit cleanly\n"
                      << "  --help         Display this help message\n";
            return 0;
        }
    }

    // Launch GUI application
    can_vhal::AppWindow app;
    if (!app.init(1600, 960, "CAN-to-AOSP VHAL Visual Mapping Studio")) {
        std::cerr << "Could not initialize graphical window (No X11/Wayland display server found)." << std::endl;
        std::cerr << "Running verification benchmark mode automatically..." << std::endl;
        app.runBenchmark(150);
        return 0;
    }

    std::cout << "[GUI] Studio running at 60 FPS. Close window to exit." << std::endl;
    app.run(maxFrames);
    app.shutdown();

    return 0;
}
