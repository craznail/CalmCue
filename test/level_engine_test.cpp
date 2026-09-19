#include <cassert>
#include <cstdint>
#include <iostream>

#include "LevelEngine.h"

namespace {

constexpr uint32_t kFrameMs = CalmCueConfig::kMicFrameDurationMs;

LevelSnapshot feed(LevelEngine& engine,
                   uint32_t& nowMs,
                   float dbfs,
                   uint32_t durationMs,
                   bool valid = true) {
  LevelSnapshot snapshot;
  const uint32_t frameCount = durationMs / kFrameMs;
  for (uint32_t i = 0; i < frameCount; ++i) {
    nowMs += kFrameMs;
    snapshot = engine.update(dbfs, valid, nowMs);
  }
  return snapshot;
}

}  // namespace

int main() {
  LevelEngine engine;
  uint32_t nowMs = 0;
  engine.begin(nowMs);

  LevelSnapshot snapshot = feed(engine, nowMs, -60.0F, 5200);
  assert(snapshot.mode == EngineMode::Running);
  assert(snapshot.alertLevel == AlertLevel::Normal);
  assert(snapshot.baselineDbfs > -60.1F && snapshot.baselineDbfs < -59.9F);

  snapshot = feed(engine, nowMs, -43.0F, 4000);
  assert(snapshot.alertLevel == AlertLevel::Normal);

  snapshot = feed(engine, nowMs, -10.0F, kFrameMs);
  snapshot = feed(engine, nowMs, -60.0F, 2000);
  assert(snapshot.alertLevel == AlertLevel::Normal);

  snapshot = feed(engine, nowMs, -38.0F, 2400);
  assert(snapshot.alertLevel == AlertLevel::Mild);

  snapshot = feed(engine, nowMs, -34.0F, 2800);
  assert(snapshot.alertLevel == AlertLevel::Moderate);

  snapshot = feed(engine, nowMs, -30.0F, 3200);
  assert(snapshot.alertLevel == AlertLevel::Severe);

  snapshot = feed(engine, nowMs, -60.0F, 7500);
  assert(snapshot.alertLevel == AlertLevel::Normal);

  snapshot = feed(engine,
                  nowMs,
                  0.0F,
                  CalmCueConfig::kInvalidFramesBeforeFault * kFrameMs,
                  false);
  assert(snapshot.mode == EngineMode::Fault);
  assert(snapshot.alertLevel == AlertLevel::Normal);

  snapshot = feed(engine,
                  nowMs,
                  -60.0F,
                  CalmCueConfig::kValidFramesBeforeRecovery * kFrameMs);
  assert(snapshot.mode == EngineMode::Calibrating);
  assert(snapshot.alertLevel == AlertLevel::Normal);

  std::cout << "LevelEngine tests passed\n";
  return 0;
}
