#include <Arduino.h>
#include <Preferences.h>

#include <cmath>

#include "CalmCueConfig.h"
#include "K10Hardware.h"
#include "LevelEngine.h"
#include "MicDiagnostics.h"

namespace {

constexpr char kPreferencesNamespace[] = "calmcue";
constexpr char kThresholdPreferenceKey[] = "loudDbfs";

K10Hardware hardware;
LevelEngine levelEngine;
MicDiagnostics diagnostics;
Preferences preferences;

float clampThreshold(float thresholdDbfs) {
  if (!std::isfinite(thresholdDbfs)) {
    return CalmCueConfig::kLoudThresholdDbfs;
  }
  if (thresholdDbfs < CalmCueConfig::kMinimumLoudThresholdDbfs) {
    return CalmCueConfig::kMinimumLoudThresholdDbfs;
  }
  if (thresholdDbfs > CalmCueConfig::kMaximumLoudThresholdDbfs) {
    return CalmCueConfig::kMaximumLoudThresholdDbfs;
  }
  return thresholdDbfs;
}

uint32_t lightColorFor(const LevelSnapshot& level) {
  return level.mode == EngineMode::Running && level.loud
             ? CalmCueConfig::kColorLoud
             : CalmCueConfig::kColorOff;
}

void applyThresholdAdjustment(int8_t direction,
                              float currentDbfs,
                              uint32_t nowMs) {
  if (direction == 0) {
    return;
  }

  const float previousThreshold = levelEngine.loudThresholdDbfs();
  const float nextThreshold = clampThreshold(
      previousThreshold + direction * CalmCueConfig::kLoudThresholdStepDb);
  if (nextThreshold == previousThreshold) {
    return;
  }

  levelEngine.setLoudThresholdDbfs(nextThreshold);
  preferences.putFloat(kThresholdPreferenceKey, nextThreshold);
  hardware.showSoundLevel(currentDbfs, nextThreshold, nowMs);
  Serial.printf("# threshold changed to %.0f dBFS\n", nextThreshold);
}

}  // namespace

void setup() {
  Serial.begin(CalmCueConfig::kSerialBaudRate);
  delay(CalmCueConfig::kSerialStartupDelayMs);

  hardware.begin();
  preferences.begin(kPreferencesNamespace, false);
  const float savedThreshold = clampThreshold(preferences.getFloat(
      kThresholdPreferenceKey, CalmCueConfig::kLoudThresholdDbfs));

  const uint32_t nowMs = millis();
  levelEngine.begin(nowMs);
  levelEngine.setLoudThresholdDbfs(savedThreshold);
  diagnostics.begin(nowMs);

  Serial.println("# CalmCue loud voice detector");
  Serial.println("# Button A: easier to trigger; Button B: harder to trigger");
  Serial.println("# dBFS is a digital relative level, not calibrated dBA");
  Serial.println(
      "ms,frames,valid,dbfs_avg,dbfs_min,dbfs_peak,smoothed_dbfs,"
      "threshold_dbfs,mode,state,reason,read_us_avg,read_us_max,"
      "errors_total");
}

void loop() {
  MicFrame frame;
  const bool valid = hardware.readMicFrame(frame);
  const uint32_t nowMs = millis();
  const LevelSnapshot level =
      levelEngine.update(frame.maxDbfs, valid, nowMs);

  hardware.setLightColor(lightColorFor(level));
  diagnostics.report(nowMs, frame, valid, level);
  applyThresholdAdjustment(
      hardware.pollThresholdAdjustment(), level.smoothedDbfs, nowMs);
  hardware.updateScreen(
      millis(), level.smoothedDbfs, levelEngine.loudThresholdDbfs());
}
