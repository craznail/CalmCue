#include "MicDiagnostics.h"

#include "CalmCueConfig.h"
#include "K10Hardware.h"

namespace {

const char* modeName(EngineMode mode) {
  switch (mode) {
    case EngineMode::Calibrating:
      return "CAL";
    case EngineMode::Running:
      return "RUN";
    case EngineMode::Fault:
      return "FAULT";
    default:
      return "UNKNOWN";
  }
}

const char* levelName(AlertLevel level) {
  switch (level) {
    case AlertLevel::Normal:
      return "L0";
    case AlertLevel::Mild:
      return "L1";
    case AlertLevel::Moderate:
      return "L2";
    case AlertLevel::Severe:
      return "L3";
    default:
      return "LX";
  }
}

const char* reasonName(TransitionReason reason) {
  switch (reason) {
    case TransitionReason::CalibrationComplete:
      return "calibration_complete";
    case TransitionReason::ThresholdHeld:
      return "threshold_held";
    case TransitionReason::BelowExitHeld:
      return "below_exit_held";
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

  Serial.printf("%lu,%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%s,%s,%u,%lu,%lu,%lu\n",
                static_cast<unsigned long>(nowMs),
                static_cast<unsigned long>(frameCount_),
                static_cast<unsigned long>(validFrameCount_),
                levelAverage,
                validFrameCount_ == 0 ? 0.0F : minimumDbfs_,
                validFrameCount_ == 0 ? 0.0F : peakDbfs_,
                level.baselineDbfs,
                level.fastDbfs,
                level.sustainedDbfs,
                level.relativeFastDb,
                level.relativeSustainedDb,
                modeName(level.mode),
                levelName(level.alertLevel),
                reasonName(windowReason_),
                static_cast<unsigned int>(level.calibrationPercent),
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
