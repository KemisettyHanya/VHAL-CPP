#include "core/AutomotiveTypes.hpp"
#include "core/VhalCatalog.hpp"
#include "core/DbcParser.hpp"
#include "core/CppReverseParser.hpp"
#include "core/CppSynthesizer.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <cstdlib>

#undef assert
#define assert(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
} while(0)

#include "core/PathUtils.hpp"

using namespace can_vhal;

static std::string resolveSamplePath(const std::string& rel) {
    return resolvePath(rel);
}

void testDbcParser() {
    std::cout << "[TEST] Running testDbcParser..." << std::endl;
    DbcParser parser;
    std::string err;
    bool ok = parser.parseFile(resolveSamplePath("sample_data/sample_vehicle_speed.dbc"), err);
    if (!ok) {
        std::cerr << "DbcParser failed: " << err << std::endl;
    }
    assert(ok);
    assert(parser.getMessages().size() == 2);

    const CanMessage* msg256 = parser.findMessage(256);
    assert(msg256 != nullptr);
    assert(msg256->name == "VehicleSpeedMsg");
    assert(msg256->dlc == 8);
    assert(msg256->signals.size() == 2);

    const CanSignal* sigSpeed = msg256->findSignal("VehicleSpeed");
    assert(sigSpeed != nullptr);
    assert(sigSpeed->startBit == 0);
    assert(sigSpeed->bitLength == 16);
    assert(sigSpeed->endianness == Endianness::LittleEndian);
    assert(std::abs(sigSpeed->factor - 0.01) < 1e-6);
    assert(sigSpeed->unit == "km/h");

    const CanSignal* sigStatus = msg256->findSignal("VehicleSpeedStatus");
    assert(sigStatus != nullptr);
    assert(sigStatus->startBit == 16);
    assert(sigStatus->bitLength == 8);
    assert(!sigStatus->valueDescriptions.empty());
    assert(sigStatus->valueDescriptions[0].description == "OK");

    // Test Bit Unpacking Logic
    uint8_t payload[8] = { 0x58, 0x1B, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    // 0x1B58 = 7000 decimal. 7000 * 0.01 = 70.0 km/h
    uint64_t raw = sigSpeed->decodeRaw(payload, sizeof(payload));
    assert(raw == 0x1B58);
    double physical = sigSpeed->decodePhysical(payload, sizeof(payload));
    assert(std::abs(physical - 70.0) < 1e-4);

    uint64_t rawStatus = sigStatus->decodeRaw(payload, sizeof(payload));
    assert(rawStatus == 1); // 0x01 = Degraded

    std::cout << "  -> testDbcParser passed!" << std::endl;
}

void testMotorolaUnpack() {
    std::cout << "[TEST] Running testMotorolaUnpack..." << std::endl;
    CanSignal beSig;
    beSig.name = "MotorolaSignal";
    beSig.startBit = 7; // MSB in byte 0, bit 7
    beSig.bitLength = 16;
    beSig.endianness = Endianness::BigEndian;
    beSig.factor = 1.0;
    beSig.offset = 0.0;

    // Big endian: byte 0 is high byte (0x12), byte 1 is low byte (0x34)
    uint8_t payload[8] = { 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint64_t raw = beSig.decodeRaw(payload, sizeof(payload));
    assert(raw == 0x1234);

    std::cout << "  -> testMotorolaUnpack passed!" << std::endl;
}

void testReverseParser() {
    std::cout << "[TEST] Running testReverseParser..." << std::endl;
    CppReverseParser revParser;
    DbcParser dbc;
    std::string err;
    bool okDbc = dbc.parseFile(resolveSamplePath("sample_data/sample_vehicle_speed.dbc"), err);
    assert(okDbc);

    auto result = revParser.parseFile(resolveSamplePath("sample_data/legacy_vhal_speed.cpp"), dbc.getMessages());
    assert(result.success);
    assert(!result.extractedLinks.empty());

    const auto& link = result.extractedLinks.front();
    assert(link.vhalPropertyName == "PERF_VEHICLE_SPEED");
    assert(link.transform.conversion == UnitConversion::KMH_TO_MPS);

    std::cout << "  -> Extracted link: CAN 0x" << std::hex << link.canMsgId
              << " (" << link.signalName << ") -> " << link.vhalPropertyName
              << ", UnitConversion: " << unitConversionToString(link.transform.conversion)
              << std::dec << std::endl;
    std::cout << "  -> testReverseParser passed!" << std::endl;
}

void testSynthesizer() {
    std::cout << "[TEST] Running testSynthesizer..." << std::endl;
    DbcParser dbc;
    std::string err;
    bool okDbc = dbc.parseFile(resolveSamplePath("sample_data/sample_vehicle_speed.dbc"), err);
    assert(okDbc);

    CppReverseParser revParser;
    auto revResult = revParser.parseFile(resolveSamplePath("sample_data/legacy_vhal_speed.cpp"), dbc.getMessages());

    CppSynthesizer synth;
    SynthesizerOptions opts;
    std::string code = synth.synthesize(revResult.extractedLinks, dbc.getMessages(), opts);

    assert(code.find("case 0x100:") != std::string::npos);
    assert(code.find("val_VehicleSpeed / 3.6f") != std::string::npos);
    assert(code.find("VehicleProperty::PERF_VEHICLE_SPEED") != std::string::npos);
    assert(code.find("mCallback(dispatchedProps);") != std::string::npos);

    std::cout << "  -> testSynthesizer passed!" << std::endl;
}

void testCompletePowertrainDbcAndLegacyVhal() {
    std::cout << "[TEST] Running testCompletePowertrainDbcAndLegacyVhal..." << std::endl;
    DbcParser dbc;
    std::string err;
    bool okDbc = dbc.parseFile(resolveSamplePath("sample_data/complete_vehicle_powertrain.dbc"), err);
    assert(okDbc);
    assert(dbc.getMessages().size() == 4);
    assert(dbc.totalSignalsCount() == 16);

    const CanMessage* msgDynamics = dbc.findMessage(420);
    assert(msgDynamics != nullptr);
    assert(msgDynamics->signals.size() == 4);

    const CanMessage* msgEngine = dbc.findMessage(688);
    assert(msgEngine != nullptr);
    assert(msgEngine->signals.size() == 4);

    CppReverseParser revParser;
    auto revResult = revParser.parseFile(resolveSamplePath("sample_data/legacy_vehicle_vhal.cpp"), dbc.getMessages());
    assert(revResult.success);
    assert(revResult.parsedCasesCount >= 3);
    assert(revResult.extractedLinks.size() >= 3);

    // Verify property extractions
    bool foundSpeed = false, foundRpm = false, foundGear = false;
    for (const auto& link : revResult.extractedLinks) {
        if (link.vhalPropertyName == "PERF_VEHICLE_SPEED") foundSpeed = true;
        if (link.vhalPropertyName == "ENGINE_RPM") foundRpm = true;
        if (link.vhalPropertyName == "GEAR_SELECTION") foundGear = true;
    }
    assert(foundSpeed);
    assert(foundRpm);
    assert(foundGear);

    CppSynthesizer synth;
    std::string code = synth.synthesize(revResult.extractedLinks, dbc.getMessages());
    assert(code.find("case 0x1A4:") != std::string::npos);
    assert(code.find("case 0x2B0:") != std::string::npos);
    assert(code.find("case 0x2C0:") != std::string::npos);
    assert(code.find("VehicleProperty::PERF_VEHICLE_SPEED") != std::string::npos);
    assert(code.find("VehicleProperty::ENGINE_RPM") != std::string::npos);
    assert(code.find("VehicleProperty::GEAR_SELECTION") != std::string::npos);

    std::cout << "  -> Successfully verified 4 messages, 16 signals, 3 legacy cases and synthesis!" << std::endl;
    std::cout << "  -> testCompletePowertrainDbcAndLegacyVhal passed!" << std::endl;
}

void testBenchmark300Nodes() {
    std::cout << "[TEST] Running testBenchmark300Nodes (Stress test 300+ elements)..." << std::endl;
    std::vector<CanMessage> messages;
    std::vector<NodeLink> links;

    const size_t COUNT = 150;
    messages.reserve(COUNT);
    links.reserve(COUNT);

    auto& catalog = VhalCatalog::instance();
    const auto& allProps = catalog.getAllProperties();
    assert(!allProps.empty());

    for (size_t i = 0; i < COUNT; ++i) {
        CanMessage msg;
        msg.id = static_cast<uint32_t>(0x200 + i);
        msg.name = "BenchMsg_" + std::to_string(i);
        msg.dlc = 8;

        CanSignal sig;
        sig.name = "Sig_" + std::to_string(i);
        sig.startBit = static_cast<uint32_t>((i % 4) * 16);
        sig.bitLength = 16;
        sig.endianness = (i % 2 == 0) ? Endianness::LittleEndian : Endianness::BigEndian;
        sig.factor = 0.1 * (i + 1);
        sig.offset = 0.0;
        sig.unit = (i % 3 == 0) ? "km/h" : "rpm";

        msg.signals.push_back(sig);
        messages.push_back(msg);

        NodeLink link;
        link.id = 10000 + i;
        link.canMsgId = msg.id;
        link.canMsgName = msg.name;
        link.signalName = sig.name;

        const auto& prop = allProps[i % allProps.size()];
        link.vhalPropertyId = prop.id;
        link.vhalPropertyName = prop.name;
        link.fromPinId = (static_cast<uint64_t>(msg.id) << 32) | 1ULL | (1ULL << 63);
        link.toPinId = (static_cast<uint64_t>(prop.id) << 32) | 1ULL;
        if (sig.unit == "km/h" && prop.name.find("SPEED") != std::string::npos) {
            link.transform.conversion = UnitConversion::KMH_TO_MPS;
        }
        links.push_back(link);
    }

    auto start = std::chrono::high_resolution_clock::now();
    CppSynthesizer synth;
    std::string synthesizedCode = synth.synthesize(links, messages);
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  -> Synthesized " << links.size() << " links across "
              << messages.size() << " messages (" << synthesizedCode.size()
              << " bytes) in " << durationMs << " ms" << std::endl;
    assert(!synthesizedCode.empty());
    assert(durationMs < 500); // Must be under 500ms

    std::cout << "  -> testBenchmark300Nodes passed!" << std::endl;
}

void testFullVehicleProductionDbc() {
    std::cout << "[TEST] Running testFullVehicleProductionDbc..." << std::endl;
    DbcParser parser;
    std::string err;
    bool ok = parser.parseFile(resolveSamplePath("sample_data/full_vehicle_production.dbc"), err);
    if (!ok) {
        std::cerr << "Full vehicle production DBC parse failed: " << err << std::endl;
    }
    assert(ok);
    assert(parser.getMessages().size() >= 16);
    assert(parser.totalSignalsCount() >= 65);

    // Verify key vehicle messages
    const CanMessage* wheelSpeed = parser.findMessage(256);
    assert(wheelSpeed != nullptr);
    assert(wheelSpeed->name == "WheelSpeedMsg");
    assert(wheelSpeed->findSignal("WheelSpeedFL") != nullptr);

    const CanMessage* eng = parser.findMessage(512);
    assert(eng != nullptr);
    assert(eng->name == "EngineStatus");
    assert(eng->findSignal("EngineRPM") != nullptr);

    const CanMessage* batt = parser.findMessage(640);
    assert(batt != nullptr);
    assert(batt->name == "HighVoltageBattery");
    assert(batt->findSignal("BatteryStateOfCharge") != nullptr);

    const CanMessage* climate = parser.findMessage(768);
    assert(climate != nullptr);
    assert(climate->name == "FrontClimateControl");
    assert(climate->findSignal("CabinTempActual") != nullptr);

    const CanMessage* adas = parser.findMessage(1280);
    assert(adas != nullptr);
    assert(adas->name == "ADASDriveAssist");
    assert(adas->findSignal("ACCSetSpeed") != nullptr);

    std::cout << "  -> Successfully loaded " << parser.getMessages().size() << " messages and "
              << parser.totalSignalsCount() << " signals from full_vehicle_production.dbc" << std::endl;
    std::cout << "  -> testFullVehicleProductionDbc passed!" << std::endl;
}

void testRoundtripSynthesisAndReload() {
    std::cout << "[TEST] Running testRoundtripSynthesisAndReload (Save & Reload Connections)..." << std::endl;
    DbcParser dbc;
    std::string err;
    bool okDbc = dbc.parseFile(resolveSamplePath("sample_data/full_vehicle_production.dbc"), err);
    assert(okDbc);

    // Create 3 active links with transforms
    std::vector<NodeLink> originalLinks;

    NodeLink l1;
    l1.id = 101;
    l1.canMsgId = 256;
    l1.canMsgName = "WheelSpeedMsg";
    l1.signalName = "WheelSpeedFL";
    l1.vhalPropertyId = 0x11600207;
    l1.vhalPropertyName = "PERF_VEHICLE_SPEED";
    l1.transform.conversion = UnitConversion::KMH_TO_MPS;
    originalLinks.push_back(l1);

    NodeLink l2;
    l2.id = 102;
    l2.canMsgId = 512;
    l2.canMsgName = "EngineStatus";
    l2.signalName = "EngineRPM";
    l2.vhalPropertyId = 0x11400305;
    l2.vhalPropertyName = "ENGINE_RPM";
    l2.transform.hasClamp = true;
    l2.transform.clampMin = 0.0f;
    l2.transform.clampMax = 9000.0f;
    originalLinks.push_back(l2);

    NodeLink l3;
    l3.id = 103;
    l3.canMsgId = 640;
    l3.canMsgName = "HighVoltageBattery";
    l3.signalName = "BatteryStateOfCharge";
    l3.vhalPropertyId = 0x11400405;
    l3.vhalPropertyName = "EV_BATTERY_LEVEL";
    l3.transform.conversion = UnitConversion::NONE;
    originalLinks.push_back(l3);

    // 1. Synthesize C++ code
    CppSynthesizer synth;
    std::string generatedCpp = synth.synthesize(originalLinks, dbc.getMessages());
    assert(!generatedCpp.empty());
    assert(generatedCpp.find("CAN_VHAL_STUDIO_METADATA_START") != std::string::npos);

    // 2. Reload / Reverse Parse the generated C++ code
    CppReverseParser rev;
    auto revResult = rev.parseString(generatedCpp, dbc.getMessages());
    assert(revResult.success);
    assert(revResult.extractedLinks.size() == 3);

    // Verify exact fidelity
    assert(revResult.extractedLinks[0].signalName == "WheelSpeedFL");
    assert(revResult.extractedLinks[0].vhalPropertyName == "PERF_VEHICLE_SPEED");
    assert(revResult.extractedLinks[0].transform.conversion == UnitConversion::KMH_TO_MPS);

    assert(revResult.extractedLinks[1].signalName == "EngineRPM");
    assert(revResult.extractedLinks[1].vhalPropertyName == "ENGINE_RPM");
    assert(revResult.extractedLinks[1].transform.hasClamp == true);
    assert(revResult.extractedLinks[1].transform.clampMax == 9000.0f);

    assert(revResult.extractedLinks[2].signalName == "BatteryStateOfCharge");
    assert(revResult.extractedLinks[2].vhalPropertyName == "EV_BATTERY_LEVEL");

    std::cout << "  -> Reconstituted all 3 links and transformations from generated C++ code with 100% fidelity!" << std::endl;
    std::cout << "  -> testRoundtripSynthesisAndReload passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Core Engine Unit & Benchmark Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testDbcParser();
    testMotorolaUnpack();
    testReverseParser();
    testSynthesizer();
    testCompletePowertrainDbcAndLegacyVhal();
    testBenchmark300Nodes();
    testFullVehicleProductionDbc();
    testRoundtripSynthesisAndReload();

    std::cout << "========================================" << std::endl;
    std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
