#pragma once

#include <cstddef>
#include <cstdint>

#include "CalmCueConfig.h"

enum class EngineMode : uint8_t {
  Calibrating = 0,
  Running,
  Fault,
};

enum class AlertLevel : uint8_t {
  Normal = 0,
  Mild,
  Moderate,
  Severe,
};

enum class TransitionReason : uint8_t {
  None = 0,
  CalibrationComplete,
  ThresholdHeld,
  BelowExitHeld,
  InputFault,
  InputRecovered,
};

struct LevelSnapshot {
  EngineMode mode = EngineMode::Calibrating;
  AlertLevel alertLevel = AlertLevel::Normal;
  TransitionReason reason = TransitionReason::None;
  float inputDbfs = 0.0F;
  float baselineDbfs = 0.0F;
  float fastDbfs = 0.0F;
  float sustainedDbfs = 0.0F;
  float relativeFastDb = 0.0F;
  float relativeSustainedDb = 0.0F;
  uint8_t calibrationPercent = 0;
  uint32_t consecutiveInvalidFrames = 0;
  bool stateChanged = false;
};

class LevelEngine {
 public:
  void begin(uint32_t nowMs);
  LevelSnapshot update(float inputDbfs, bool valid, uint32_t nowMs);

 private:
  void finishCalibration(uint32_t nowMs);
  void updateRunning(float inputDbfs, uint32_t nowMs);
  void updateAlertState(uint32_t nowMs);
  void resetEntryTimers();
  void addSustainedSample(float inputDbfs);
  float calculateSustainedMedian() const;
  LevelSnapshot makeSnapshot(float inputDbfs,
                             TransitionReason reason,
                             bool stateChanged,
                             uint32_t nowMs) const;

  EngineMode mode_ = EngineMode::Calibrating;
  AlertLevel alertLevel_ = AlertLevel::Normal;
  float baselineDbfs_ = -60.0F;
  float fastDbfs_ = -60.0F;
  float sustainedDbfs_ = -60.0F;

  float calibrationSamples_[CalmCueConfig::kCalibrationSampleCapacity] = {};
  size_t calibrationSampleCount_ = 0;
  uint32_t calibrationStartedAtMs_ = 0;

  float sustainedHistory_[CalmCueConfig::kSustainedWindowFrames] = {};
  size_t sustainedHistoryCount_ = 0;
  size_t sustainedHistoryIndex_ = 0;

  bool entryTimerActive_[4] = {};
  uint32_t entryTimerStartedAtMs_[4] = {};
  bool exitTimerActive_ = false;
  uint32_t exitTimerStartedAtMs_ = 0;

  uint32_t lastUpdateAtMs_ = 0;
  uint32_t consecutiveInvalidFrames_ = 0;
  uint32_t consecutiveValidFrames_ = 0;
  TransitionReason pendingReason_ = TransitionReason::None;
};
