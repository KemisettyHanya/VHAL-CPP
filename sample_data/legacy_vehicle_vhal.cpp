// Legacy Automotive VHAL CAN receiver implementation
#include <cstdint>
#include <linux/can.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropValue.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>

#define CAN_ID_VEH_DYNAMICS 0x1A4   // 420
#define CAN_ID_ENGINE_STATUS 0x2B0  // 688
#define CAN_ID_TRANS_STATUS 0x2C0   // 704
#define CAN_ID_CLIMATE_STATUS 0x2D0 // 720

namespace legacy::vhal {

using aidl::android::hardware::automotive::vehicle::VehicleProperty;
using aidl::android::hardware::automotive::vehicle::VehiclePropValue;

void onCanFrameReceived(const struct can_frame& frame) {
    switch (frame.can_id) {
        case 0x1A4: { // VehDynamics
            // Unpack vehicle speed (Intel little endian, 16 bits, factor 0.01)
            uint16_t rawSpeed = frame.data[0] | (static_cast<uint16_t>(frame.data[1]) << 8);
            float speedKmh = static_cast<float>(rawSpeed) * 0.01f;
            float speedMps = speedKmh / 3.6f;
            speedMps = std::clamp(speedMps, 0.0f, 90.0f);

            VehiclePropValue propSpeed{};
            propSpeed.prop = static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED);
            propSpeed.areaId = 0;
            propSpeed.value.floatValues = { speedMps };
            break;
        }

        case 0x2B0: { // EngineStatus
            // Unpack engine RPM (Intel little endian, 16 bits, factor 0.25)
            uint16_t rawRpm = frame.data[0] | (static_cast<uint16_t>(frame.data[1]) << 8);
            float engineRpm = static_cast<float>(rawRpm) * 0.25f;

            VehiclePropValue propRpm{};
            propRpm.prop = static_cast<int32_t>(VehicleProperty::ENGINE_RPM);
            propRpm.areaId = 0;
            propRpm.value.floatValues = { engineRpm };

            // Coolant temp (byte 2, offset -40)
            uint8_t rawTemp = frame.data[2];
            float coolantCelsius = static_cast<float>(rawTemp) - 40.0f;

            VehiclePropValue propTemp{};
            propTemp.prop = static_cast<int32_t>(VehicleProperty::ENGINE_COOLANT_TEMP);
            propTemp.areaId = 0;
            propTemp.value.floatValues = { coolantCelsius };
            break;
        }

        case 0x2C0: { // TransmissionStatus
            // Selected Gear (byte 0 bits 4..7)
            int32_t selectedGear = (frame.data[0] >> 4) & 0x0F;

            VehiclePropValue propGear{};
            propGear.prop = static_cast<int32_t>(VehicleProperty::GEAR_SELECTION);
            propGear.areaId = 0;
            propGear.value.int32Values = { selectedGear };
            break;
        }

        default:
            break;
    }
}

} // namespace legacy::vhal

