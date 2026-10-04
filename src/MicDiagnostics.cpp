#include "MicDiagnostics.h"

#include "CalmCueConfig.h"
#include "K10Hardware.h"

namespace {

const char* modeName(EngineMode mode) {
  return mode == EngineMode::Running ? "RUN" : "FAULT";
}

const char* stateName(bool loud) {
  return loud ? "RED" : "OFF";
}

const char* reasonName(TransitionReason reason) {
  switch (reason) {
    case TransitionReason::LoudHeld:
      return "loud_held";
    case TransitionReason::QuietHeld:
      return "quiet_held";
    case TransitionReason::InputFault:
      return "input_fault";
    case TransitionReason::InputRecovered:
      return "input_recovered";
    case TransitionReason::None:
    default:
      return "none";
  }
}

}  // namespace

void MicDiagnostics::begin(uint32_t nowMs) {
  totalErrors_ = 0;
  resetWindow(nowMs);
}

void MicDiagnostics::report(uint32_t nowMs,
                            const MicFrame& frame,
                            bool valid,
                            const LevelSnapshot& level) {
  ++frameCount_;
  readDurationSumUs_ += frame.readDurationUs;
  if (frame.readDurationUs > maxReadDurationUs_) {
    maxReadDurationUs_ = frame.readDurationUs;
  }

  if (!valid) {
    ++totalErrors_;
  } else {
    ++validFrameCount_;
    maxDbfsSum_ += frame.maxDbfs;

    if (validFrameCount_ == 1 || frame.maxDbfs < minimumDbfs_) {
      minimumDbfs_ = frame.maxDbfs;
    }
    if (validFrameCount_ == 1 || frame.maxDbfs > peakDbfs_) {
      peakDbfs_ = frame.maxDbfs;
    }
  }

  if (level.reason != TransitionReason::None) {
    windowReason_ = level.reason;
  }

  if (nowMs - windowStartedAtMs_ <
      CalmCueConfig::kDiagnosticReportIntervalMs) {
    return;
  }

  printWindow(nowMs, level);
  resetWindow(nowMs);
}

void MicDiagnostics::printWindow(uint32_t nowMs,
                                 const LevelSnapshot& level) const {
  const double validCount = static_cast<double>(validFrameCount_);
  const float levelAverage =
      validFrameCount_ == 0 ? 0.0F : maxDbfsSum_ / validCount;
  const uint32_t averageReadUs =
      frameCount_ == 0 ? 0 : readDurationSumUs_ / frameCount_;

  Serial.printf("%lu,%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%s,%s,%lu,%lu,%lu\n",
                static_cast<unsigned long>(nowMs),
                static_cast<unsigned long>(frameCount_),
                static_cast<unsigned long>(validFrameCount_),
                levelAverage,
                validFrameCount_ == 0 ? 0.0F : minimumDbfs_,
                validFrameCount_ == 0 ? 0.0F : peakDbfs_,
                level.smoothedDbfs,
                level.loudThresholdDbfs,
                modeName(level.mode),
                stateName(level.loud),
                reasonName(windowReason_),
                static_cast<unsigned long>(averageReadUs),
                static_cast<unsigned long>(maxReadDurationUs_),
                static_cast<unsigned long>(totalErrors_));
}

void MicDiagnostics::resetWindow(uint32_t nowMs) {
  windowStartedAtMs_ = nowMs;
  frameCount_ = 0;
  validFrameCount_ = 0;
  maxDbfsSum_ = 0.0;
  minimumDbfs_ = 0.0F;
  peakDbfs_ = 0.0F;
  readDurationSumUs_ = 0;
  maxReadDurationUs_ = 0;
  windowReason_ = TransitionReason::None;
}
