#pragma once

#include <Arduino.h>

#include "LevelEngine.h"

struct MicFrame;

class MicDiagnostics {
 public:
  void begin(uint32_t nowMs);
  void report(uint32_t nowMs,
              const MicFrame& frame,
              bool valid,
              const LevelSnapshot& level);

 private:
  void printWindow(uint32_t nowMs, const LevelSnapshot& level) const;
  void resetWindow(uint32_t nowMs);

  uint32_t windowStartedAtMs_ = 0;
  uint32_t frameCount_ = 0;
  uint32_t validFrameCount_ = 0;
  double maxDbfsSum_ = 0.0;
  float minimumDbfs_ = 0.0F;
  float peakDbfs_ = 0.0F;
  uint64_t readDurationSumUs_ = 0;
  uint32_t maxReadDurationUs_ = 0;
  uint32_t totalErrors_ = 0;
  TransitionReason windowReason_ = TransitionReason::None;
};
