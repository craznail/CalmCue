#include "LevelEngine.h"

#include <cmath>

void LevelEngine::begin(uint32_t nowMs) {
  mode_ = EngineMode::Running;
  loud_ = false;
  smoothingInitialized_ = false;
  smoothedDbfs_ = CalmCueConfig::kMinimumValidDbfs;
  enterTimerActive_ = false;
  exitTimerActive_ = false;
  lastUpdateAtMs_ = nowMs;
  consecutiveInvalidFrames_ = 0;
  consecutiveValidFrames_ = 0;
  pendingReason_ = TransitionReason::None;
}

void LevelEngine::setLoudThresholdDbfs(float thresholdDbfs) {
  loudThresholdDbfs_ = thresholdDbfs;
  enterTimerActive_ = false;
  exitTimerActive_ = false;
}

float LevelEngine::loudThresholdDbfs() const {
  return loudThresholdDbfs_;
}

LevelSnapshot LevelEngine::update(float inputDbfs,
                                  bool valid,
                                  uint32_t nowMs) {
  const EngineMode previousMode = mode_;
  const bool previousLoud = loud_;
  TransitionReason reason = TransitionReason::None;

  const bool validFeature = valid && std::isfinite(inputDbfs) &&
                            inputDbfs >= CalmCueConfig::kMinimumValidDbfs &&
                            inputDbfs <= CalmCueConfig::kMaximumValidDbfs;

  if (!validFeature) {
    ++consecutiveInvalidFrames_;
    consecutiveValidFrames_ = 0;
    if (consecutiveInvalidFrames_ >=
            CalmCueConfig::kInvalidFramesBeforeFault &&
        mode_ != EngineMode::Fault) {
      mode_ = EngineMode::Fault;
      loud_ = false;
      enterTimerActive_ = false;
      exitTimerActive_ = false;
      reason = TransitionReason::InputFault;
    }
    return makeSnapshot(inputDbfs,
                        reason,
                        previousMode != mode_ || previousLoud != loud_,
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
                        previousMode != mode_ || previousLoud != loud_,
                        nowMs);
  }

  updateRunning(inputDbfs, nowMs);
  reason = pendingReason_;
  pendingReason_ = TransitionReason::None;

  return makeSnapshot(inputDbfs,
                      reason,
                      previousMode != mode_ || previousLoud != loud_,
                      nowMs);
}

void LevelEngine::updateRunning(float inputDbfs, uint32_t nowMs) {
  uint32_t elapsedMs = nowMs - lastUpdateAtMs_;
  if (elapsedMs > 200) {
    elapsedMs = 200;
  }
  lastUpdateAtMs_ = nowMs;

  if (!smoothingInitialized_) {
    smoothedDbfs_ = inputDbfs;
    smoothingInitialized_ = true;
  } else {
    const float elapsedSeconds = elapsedMs / 1000.0F;
    const float alpha = 1.0F - expf(
        -elapsedSeconds / CalmCueConfig::kFastSmoothingTimeConstantSeconds);
    smoothedDbfs_ += alpha * (inputDbfs - smoothedDbfs_);
  }

  if (!loud_) {
    exitTimerActive_ = false;
    if (smoothedDbfs_ >= loudThresholdDbfs_) {
      if (!enterTimerActive_) {
        enterTimerActive_ = true;
        enterTimerStartedAtMs_ = nowMs;
      } else if (nowMs - enterTimerStartedAtMs_ >=
                 CalmCueConfig::kLoudEnterDurationMs) {
        loud_ = true;
        enterTimerActive_ = false;
        pendingReason_ = TransitionReason::LoudHeld;
      }
    } else {
      enterTimerActive_ = false;
    }
    return;
  }

  enterTimerActive_ = false;
  const float quietThreshold = loudThresholdDbfs_ -
                               CalmCueConfig::kLoudThresholdHysteresisDb;
  if (smoothedDbfs_ <= quietThreshold) {
    if (!exitTimerActive_) {
      exitTimerActive_ = true;
      exitTimerStartedAtMs_ = nowMs;
    } else if (nowMs - exitTimerStartedAtMs_ >=
               CalmCueConfig::kLoudExitDurationMs) {
      loud_ = false;
      exitTimerActive_ = false;
      pendingReason_ = TransitionReason::QuietHeld;
    }
  } else {
    exitTimerActive_ = false;
  }
}

LevelSnapshot LevelEngine::makeSnapshot(float inputDbfs,
                                        TransitionReason reason,
                                        bool stateChanged,
                                        uint32_t nowMs) const {
  (void)nowMs;
  LevelSnapshot snapshot;
  snapshot.mode = mode_;
  snapshot.loud = loud_;
  snapshot.reason = reason;
  snapshot.inputDbfs = inputDbfs;
  snapshot.smoothedDbfs = smoothedDbfs_;
  snapshot.loudThresholdDbfs = loudThresholdDbfs_;
  snapshot.consecutiveInvalidFrames = consecutiveInvalidFrames_;
  snapshot.stateChanged = stateChanged;
  return snapshot;
}
