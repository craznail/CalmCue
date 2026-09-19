#pragma once

#include <cstddef>
#include <cstdint>

namespace CalmCueConfig {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kSerialStartupDelayMs = 500;

// K10 library brightness range is 0-9. LEDs remain off during microphone tests.
constexpr uint8_t kLedBrightness = 3;
constexpr uint32_t kLightsOffColor = 0x000000;

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

constexpr uint32_t kCalibrationDurationMs = 5000;
constexpr size_t kCalibrationSampleCapacity = 160;
constexpr size_t kMinimumCalibrationSamples = 80;
constexpr float kCalibrationPercentile = 0.30F;
constexpr float kMinimumBaselineDbfs = -85.0F;
constexpr float kMaximumBaselineDbfs = -35.0F;
constexpr float kBaselineTrackingTimeConstantSeconds = 60.0F;
constexpr float kBaselineUpdateFastMarginDb = 8.0F;
constexpr float kBaselineUpdateSustainedMarginDb = 6.0F;

constexpr float kFastSmoothingTimeConstantSeconds = 0.20F;
constexpr uint32_t kSustainedWindowMs = 1000;
constexpr size_t kSustainedWindowFrames =
    kSustainedWindowMs / kMicFrameDurationMs;

// Conservative provisional thresholds. Tune after labelled raised-voice tests.
constexpr float kMildEnterRelativeDb = 20.0F;
constexpr float kModerateEnterRelativeDb = 24.0F;
constexpr float kSevereEnterRelativeDb = 28.0F;
constexpr float kMildExitRelativeDb = 16.0F;
constexpr float kModerateExitRelativeDb = 20.0F;
constexpr float kSevereExitRelativeDb = 24.0F;

constexpr uint32_t kMildEnterDurationMs = 800;
constexpr uint32_t kModerateEnterDurationMs = 1000;
constexpr uint32_t kSevereEnterDurationMs = 1200;
constexpr uint32_t kMildExitDurationMs = 1500;
constexpr uint32_t kModerateExitDurationMs = 1800;
constexpr uint32_t kSevereExitDurationMs = 2000;

constexpr uint32_t kInvalidFramesBeforeFault = 25;
constexpr uint32_t kValidFramesBeforeRecovery = 10;
constexpr float kMinimumValidDbfs = -100.0F;
constexpr float kMaximumValidDbfs = 1.0F;

constexpr uint32_t kColorOff = 0x000000;
constexpr uint32_t kColorMild = 0xFFD000;
constexpr uint32_t kColorModerate = 0xFF6A00;
constexpr uint32_t kColorSevere = 0xFF0000;

}  // namespace CalmCueConfig
