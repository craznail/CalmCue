#include <Arduino.h>

#include "CalmCueConfig.h"
#include "K10Hardware.h"
#include "LevelEngine.h"
#include "MicDiagnostics.h"

namespace {

K10Hardware hardware;
LevelEngine levelEngine;
MicDiagnostics diagnostics;

uint32_t lightColorFor(const LevelSnapshot& level) {
  if (level.mode != EngineMode::Running) {
    return CalmCueConfig::kColorOff;
  }

  switch (level.alertLevel) {
    case AlertLevel::Mild:
      return CalmCueConfig::kColorMild;
    case AlertLevel::Moderate:
      return CalmCueConfig::kColorModerate;
    case AlertLevel::Severe:
      return CalmCueConfig::kColorSevere;
    case AlertLevel::Normal:
    default:
      return CalmCueConfig::kColorOff;
  }
}

}  // namespace

void setup() {
  Serial.begin(CalmCueConfig::kSerialBaudRate);
  delay(CalmCueConfig::kSerialStartupDelayMs);

  hardware.begin();
  const uint32_t nowMs = millis();
  levelEngine.begin(nowMs);
  diagnostics.begin(nowMs);

  Serial.println("# CalmCue V1 provisional level engine");
  Serial.println("# 16 kHz, signed 16-bit, two interleaved channels, 40 ms frames");
  Serial.println("# dBFS is a digital relative level, not calibrated dBA");
  Serial.println(
      "ms,frames,valid,dbfs_avg,dbfs_min,dbfs_peak,baseline_dbfs,"
      "fast_dbfs,sustained_dbfs,relative_fast_db,relative_sustained_db,"
      "mode,level,reason,calibration_percent,read_us_avg,read_us_max,"
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
}
