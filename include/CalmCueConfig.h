#pragma once

#include <cstddef>
#include <cstdint>

namespace CalmCueConfig {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kSerialStartupDelayMs = 500;

// K10 library brightness range is 0-9.
constexpr uint8_t kLedBrightness = 3;
constexpr uint32_t kScreenOnAfterAdjustmentMs = 10000;
constexpr uint32_t kScreenRefreshIntervalMs = 200;

constexpr uint32_t kMicSampleRateHz = 16000;
constexpr uint32_t kMicFrameDurationMs = 40;
constexpr size_t kMicChannelCount = 2;
constexpr size_t kSamplesPerChannelPerFrame =
    kMicSampleRateHz * kMicFrameDurationMs / 1000;
constexpr size_t kInterleavedSamplesPerFrame =
    kSamplesPerChannelPerFrame * kMicChannelCount;
constexpr uint32_t kMicReadTimeoutMs = 100;
constexpr float kPcm16FullScale = 32768.0F;
constexpr float kRmsFloor = 1.0F;
constexpr uint32_t kDiagnosticReportIntervalMs = 500;

constexpr float kFastSmoothingTimeConstantSeconds = 0.20F;

// The main tuning value. Raise it when normal speech turns the light on; lower
// it when loud speech does not. dBFS values nearer zero are louder.
constexpr float kLoudThresholdDbfs = -40.0F;
constexpr float kMinimumLoudThresholdDbfs = -70.0F;
constexpr float kMaximumLoudThresholdDbfs = -10.0F;
constexpr float kLoudThresholdStepDb = 1.0F;
constexpr float kLoudThresholdHysteresisDb = 4.0F;
constexpr uint32_t kLoudEnterDurationMs = 500;
constexpr uint32_t kLoudExitDurationMs = 1000;

constexpr uint32_t kInvalidFramesBeforeFault = 25;
constexpr uint32_t kValidFramesBeforeRecovery = 10;
constexpr float kMinimumValidDbfs = -100.0F;
constexpr float kMaximumValidDbfs = 1.0F;

constexpr uint32_t kColorOff = 0x000000;
constexpr uint32_t kColorLoud = 0xFF0000;

}  // namespace CalmCueConfig
