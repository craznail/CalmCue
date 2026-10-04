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

  LevelSnapshot snapshot = feed(engine, nowMs, -55.0F, 2000);
  assert(snapshot.mode == EngineMode::Running);
  assert(!snapshot.loud);

  snapshot = feed(engine, nowMs, -10.0F, kFrameMs);
  snapshot = feed(engine, nowMs, -55.0F, 1500);
  assert(!snapshot.loud);

  snapshot = feed(engine, nowMs, -30.0F, 1600);
  assert(snapshot.loud);

  snapshot = feed(engine, nowMs, -42.0F, 2000);
  assert(snapshot.loud);

  snapshot = feed(engine, nowMs, -55.0F, 2500);
  assert(!snapshot.loud);

  engine.setLoudThresholdDbfs(-50.0F);
  assert(engine.loudThresholdDbfs() == -50.0F);
  snapshot = feed(engine, nowMs, -45.0F, 1600);
  assert(snapshot.loud);

  snapshot = feed(engine,
                  nowMs,
                  0.0F,
                  CalmCueConfig::kInvalidFramesBeforeFault * kFrameMs,
                  false);
  assert(snapshot.mode == EngineMode::Fault);
  assert(!snapshot.loud);

  snapshot = feed(engine,
                  nowMs,
                  -55.0F,
                  CalmCueConfig::kValidFramesBeforeRecovery * kFrameMs);
  assert(snapshot.mode == EngineMode::Running);
  assert(!snapshot.loud);
  assert(engine.loudThresholdDbfs() == -50.0F);

  std::cout << "LevelEngine tests passed\n";
  return 0;
}
