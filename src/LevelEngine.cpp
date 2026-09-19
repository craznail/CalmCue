#include "LevelEngine.h"

#include <algorithm>
#include <cmath>

namespace {

float clampValue(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

uint8_t levelIndex(AlertLevel level) {
  return static_cast<uint8_t>(level);
}

float enterThreshold(AlertLevel level) {
  switch (level) {
    case AlertLevel::Mild:
      return CalmCueConfig::kMildEnterRelativeDb;
    case AlertLevel::Moderate:
      return CalmCueConfig::kModerateEnterRelativeDb;
    case AlertLevel::Severe:
      return CalmCueConfig::kSevereEnterRelativeDb;
    case AlertLevel::Normal:
    default:
      return 0.0F;
  }
}

float exitThreshold(AlertLevel level) {
  switch (level) {
    case AlertLevel::Mild:
      return CalmCueConfig::kMildExitRelativeDb;
    case AlertLevel::Moderate:
      return CalmCueConfig::kModerateExitRelativeDb;
    case AlertLevel::Severe:
      return CalmCueConfig::kSevereExitRelativeDb;
    case AlertLevel::Normal:
    default:
      return 0.0F;
  }
}

uint32_t enterDuration(AlertLevel level) {
  switch (level) {
    case AlertLevel::Mild:
      return CalmCueConfig::kMildEnterDurationMs;
    case AlertLevel::Moderate:
      return CalmCueConfig::kModerateEnterDurationMs;
    case AlertLevel::Severe:
      return CalmCueConfig::kSevereEnterDurationMs;
    case AlertLevel::Normal:
    default:
      return 0;
  }
}

uint32_t exitDuration(AlertLevel level) {
  switch (level) {
    case AlertLevel::Mild:
      return CalmCueConfig::kMildExitDurationMs;
    case AlertLevel::Moderate:
      return CalmCueConfig::kModerateExitDurationMs;
    case AlertLevel::Severe:
      return CalmCueConfig::kSevereExitDurationMs;
    case AlertLevel::Normal:
    default:
      return 0;
  }
}

}  // namespace

void LevelEngine::begin(uint32_t nowMs) {
  mode_ = EngineMode::Calibrating;
  alertLevel_ = AlertLevel::Normal;
  calibrationSampleCount_ = 0;
  calibrationStartedAtMs_ = nowMs;
  sustainedHistoryCount_ = 0;
  sustainedHistoryIndex_ = 0;
  lastUpdateAtMs_ = nowMs;
  consecutiveInvalidFrames_ = 0;
  consecutiveValidFrames_ = 0;
  exitTimerActive_ = false;
  pendingReason_ = TransitionReason::None;
  resetEntryTimers();
}

LevelSnapshot LevelEngine::update(float inputDbfs,
                                  bool valid,
                                  uint32_t nowMs) {
  const EngineMode previousMode = mode_;
  const AlertLevel previousLevel = alertLevel_;
  TransitionReason reason = TransitionReason::None;

  const bool validFeature = valid && std::isfinite(inputDbfs) &&
                            inputDbfs >= CalmCueConfig::kMinimumValidDbfs &&
                            inputDbfs <= CalmCueConfig::kMaximumValidDbfs;

  if (!validFeature) {
    ++consecutiveInvalidFrames_;
    consecutiveValidFrames_ = 0;
    if (consecutiveInvalidFrames_ >=
        CalmCueConfig::kInvalidFramesBeforeFault) {
      mode_ = EngineMode::Fault;
      alertLevel_ = AlertLevel::Normal;
      reason = TransitionReason::InputFault;
    }
    return makeSnapshot(inputDbfs,
                        reason,
                        previousMode != mode_ || previousLevel != alertLevel_,
                        nowMs);
  }

  consecutiveInvalidFrames_ = 0;
  ++consecutiveValidFrames_;

  if (mode_ == EngineMode::Fault) {
    if (consecutiveValidFrames_ >= CalmCueConfig::kValidFramesBeforeRecovery) {
      begin(nowMs);
      reason = TransitionReason::InputRecovered;
    }
    return makeSnapshot(inputDbfs,
                        reason,
                        previousMode != mode_ || previousLevel != alertLevel_,
                        nowMs);
  }

  if (mode_ == EngineMode::Calibrating) {
    if (calibrationSampleCount_ <
        CalmCueConfig::kCalibrationSampleCapacity) {
      calibrationSamples_[calibrationSampleCount_++] = inputDbfs;
    }

    if (nowMs - calibrationStartedAtMs_ >=
            CalmCueConfig::kCalibrationDurationMs &&
        calibrationSampleCount_ >=
            CalmCueConfig::kMinimumCalibrationSamples) {
      finishCalibration(nowMs);
      reason = TransitionReason::CalibrationComplete;
    }

    return makeSnapshot(inputDbfs,
                        reason,
                        previousMode != mode_ || previousLevel != alertLevel_,
                        nowMs);
  }

  updateRunning(inputDbfs, nowMs);
  reason = pendingReason_;
  pendingReason_ = TransitionReason::None;

  return makeSnapshot(inputDbfs,
                      reason,
                      previousMode != mode_ || previousLevel != alertLevel_,
                      nowMs);
}

void LevelEngine::finishCalibration(uint32_t nowMs) {
  std::sort(calibrationSamples_,
            calibrationSamples_ + calibrationSampleCount_);
  const size_t percentileIndex = static_cast<size_t>(
      (calibrationSampleCount_ - 1) * CalmCueConfig::kCalibrationPercentile);
  baselineDbfs_ = clampValue(
      calibrationSamples_[percentileIndex],
      CalmCueConfig::kMinimumBaselineDbfs,
      CalmCueConfig::kMaximumBaselineDbfs);

  fastDbfs_ = baselineDbfs_;
  sustainedDbfs_ = baselineDbfs_;
  sustainedHistoryCount_ = 0;
  sustainedHistoryIndex_ = 0;
  lastUpdateAtMs_ = nowMs;
  alertLevel_ = AlertLevel::Normal;
  mode_ = EngineMode::Running;
  exitTimerActive_ = false;
  resetEntryTimers();
}

void LevelEngine::updateRunning(float inputDbfs, uint32_t nowMs) {
  uint32_t elapsedMs = nowMs - lastUpdateAtMs_;
  if (elapsedMs > 200) {
    elapsedMs = 200;
  }
  lastUpdateAtMs_ = nowMs;

  const float elapsedSeconds = elapsedMs / 1000.0F;
  const float fastAlpha = 1.0F - expf(
      -elapsedSeconds / CalmCueConfig::kFastSmoothingTimeConstantSeconds);
  fastDbfs_ += fastAlpha * (inputDbfs - fastDbfs_);

  addSustainedSample(inputDbfs);
  sustainedDbfs_ = calculateSustainedMedian();

  updateAlertState(nowMs);

  const float relativeFast = fastDbfs_ - baselineDbfs_;
  const float relativeSustained = sustainedDbfs_ - baselineDbfs_;
  if (alertLevel_ == AlertLevel::Normal &&
      relativeFast <= CalmCueConfig::kBaselineUpdateFastMarginDb &&
      relativeSustained <=
          CalmCueConfig::kBaselineUpdateSustainedMarginDb) {
    const float baselineAlpha = 1.0F - expf(
        -elapsedSeconds /
        CalmCueConfig::kBaselineTrackingTimeConstantSeconds);
    baselineDbfs_ += baselineAlpha * (inputDbfs - baselineDbfs_);
    baselineDbfs_ = clampValue(baselineDbfs_,
                               CalmCueConfig::kMinimumBaselineDbfs,
                               CalmCueConfig::kMaximumBaselineDbfs);
  }
}

void LevelEngine::updateAlertState(uint32_t nowMs) {
  const float relativeFast = fastDbfs_ - baselineDbfs_;
  const float relativeSustained = sustainedDbfs_ - baselineDbfs_;

  for (uint8_t index = levelIndex(AlertLevel::Mild);
       index <= levelIndex(AlertLevel::Severe);
       ++index) {
    const AlertLevel level = static_cast<AlertLevel>(index);
    const bool condition = relativeFast >= enterThreshold(level) &&
                           relativeSustained >= enterThreshold(level);
    if (condition) {
      if (!entryTimerActive_[index]) {
        entryTimerActive_[index] = true;
        entryTimerStartedAtMs_[index] = nowMs;
      }
    } else {
      entryTimerActive_[index] = false;
    }
  }

  AlertLevel maturedLevel = alertLevel_;
  for (uint8_t index = levelIndex(AlertLevel::Mild);
       index <= levelIndex(AlertLevel::Severe);
       ++index) {
    const AlertLevel level = static_cast<AlertLevel>(index);
    if (entryTimerActive_[index] &&
        nowMs - entryTimerStartedAtMs_[index] >= enterDuration(level) &&
        index > levelIndex(maturedLevel)) {
      maturedLevel = level;
    }
  }

  if (levelIndex(maturedLevel) > levelIndex(alertLevel_)) {
    alertLevel_ = maturedLevel;
    exitTimerActive_ = false;
    pendingReason_ = TransitionReason::ThresholdHeld;
    return;
  }

  if (alertLevel_ == AlertLevel::Normal) {
    exitTimerActive_ = false;
    return;
  }

  if (relativeSustained < exitThreshold(alertLevel_)) {
    if (!exitTimerActive_) {
      exitTimerActive_ = true;
      exitTimerStartedAtMs_ = nowMs;
    } else if (nowMs - exitTimerStartedAtMs_ >=
               exitDuration(alertLevel_)) {
      alertLevel_ = static_cast<AlertLevel>(
          levelIndex(alertLevel_) - 1);
      exitTimerActive_ = false;
      pendingReason_ = TransitionReason::BelowExitHeld;
      resetEntryTimers();
    }
  } else {
    exitTimerActive_ = false;
  }
}

void LevelEngine::resetEntryTimers() {
  for (size_t i = 0; i < 4; ++i) {
    entryTimerActive_[i] = false;
    entryTimerStartedAtMs_[i] = 0;
  }
}

void LevelEngine::addSustainedSample(float inputDbfs) {
  sustainedHistory_[sustainedHistoryIndex_] = inputDbfs;
  sustainedHistoryIndex_ =
      (sustainedHistoryIndex_ + 1) %
      CalmCueConfig::kSustainedWindowFrames;
  if (sustainedHistoryCount_ < CalmCueConfig::kSustainedWindowFrames) {
    ++sustainedHistoryCount_;
  }
}

float LevelEngine::calculateSustainedMedian() const {
  if (sustainedHistoryCount_ == 0) {
    return baselineDbfs_;
  }

  float sorted[CalmCueConfig::kSustainedWindowFrames] = {};
  for (size_t i = 0; i < sustainedHistoryCount_; ++i) {
    sorted[i] = sustainedHistory_[i];
  }
  std::sort(sorted, sorted + sustainedHistoryCount_);

  const size_t middle = sustainedHistoryCount_ / 2;
  if (sustainedHistoryCount_ % 2 == 0) {
    return (sorted[middle - 1] + sorted[middle]) * 0.5F;
  }
  return sorted[middle];
}

LevelSnapshot LevelEngine::makeSnapshot(float inputDbfs,
                                        TransitionReason reason,
                                        bool stateChanged,
                                        uint32_t nowMs) const {
  LevelSnapshot snapshot;
  snapshot.mode = mode_;
  snapshot.alertLevel = alertLevel_;
  snapshot.reason = reason;
  snapshot.inputDbfs = inputDbfs;
  snapshot.baselineDbfs = baselineDbfs_;
  snapshot.fastDbfs = fastDbfs_;
  snapshot.sustainedDbfs = sustainedDbfs_;
  snapshot.relativeFastDb = fastDbfs_ - baselineDbfs_;
  snapshot.relativeSustainedDb = sustainedDbfs_ - baselineDbfs_;
  snapshot.consecutiveInvalidFrames = consecutiveInvalidFrames_;
  snapshot.stateChanged = stateChanged;

  if (mode_ == EngineMode::Calibrating) {
    const uint32_t elapsed = nowMs - calibrationStartedAtMs_;
    const uint32_t percent =
        elapsed >= CalmCueConfig::kCalibrationDurationMs
            ? 100
            : elapsed * 100 / CalmCueConfig::kCalibrationDurationMs;
    snapshot.calibrationPercent = static_cast<uint8_t>(percent);
  } else {
    snapshot.calibrationPercent = 100;
  }

  return snapshot;
}
