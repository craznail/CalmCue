#pragma once

#include <cstddef>
#include <cstdint>

#include "CalmCueConfig.h"

enum class EngineMode : uint8_t {
  Running = 0,
  Fault,
};

enum class TransitionReason : uint8_t {
  None = 0,
  LoudHeld,
  QuietHeld,
  InputFault,
  InputRecovered,
};

struct LevelSnapshot {
  EngineMode mode = EngineMode::Running;
  bool loud = false;
  TransitionReason reason = TransitionReason::None;
  float inputDbfs = 0.0F;
  float smoothedDbfs = 0.0F;
  float loudThresholdDbfs = CalmCueConfig::kLoudThresholdDbfs;
  uint32_t consecutiveInvalidFrames = 0;
  bool stateChanged = false;
};

class LevelEngine {
 public:
  void begin(uint32_t nowMs);
  void setLoudThresholdDbfs(float thresholdDbfs);
  float loudThresholdDbfs() const;
  LevelSnapshot update(float inputDbfs, bool valid, uint32_t nowMs);

 private:
  void updateRunning(float inputDbfs, uint32_t nowMs);
  LevelSnapshot makeSnapshot(float inputDbfs,
                             TransitionReason reason,
                             bool stateChanged,
                             uint32_t nowMs) const;

  EngineMode mode_ = EngineMode::Running;
  bool loud_ = false;
  bool smoothingInitialized_ = false;
  float smoothedDbfs_ = -100.0F;
  float loudThresholdDbfs_ = CalmCueConfig::kLoudThresholdDbfs;
  bool enterTimerActive_ = false;
  uint32_t enterTimerStartedAtMs_ = 0;
  bool exitTimerActive_ = false;
  uint32_t exitTimerStartedAtMs_ = 0;

  uint32_t lastUpdateAtMs_ = 0;
  uint32_t consecutiveInvalidFrames_ = 0;
  uint32_t consecutiveValidFrames_ = 0;
  TransitionReason pendingReason_ = TransitionReason::None;
};
