#pragma once

#include <Arduino.h>
#include "CalmCueConfig.h"
#include "unihiker_k10.h"

struct MicFrame {
  size_t samplesPerChannel = 0;
  size_t bytesRead = 0;
  float channel0Mean = 0.0F;
  float channel0Rms = 0.0F;
  float channel0Dbfs = 0.0F;
  float channel1Mean = 0.0F;
  float channel1Rms = 0.0F;
  float channel1Dbfs = 0.0F;
  float maxRms = 0.0F;
  float maxDbfs = 0.0F;
  uint32_t readDurationUs = 0;
};

class K10Hardware {
 public:
  void begin();

  // Reads one fixed-duration, interleaved 16-bit stereo PCM frame.
  bool readMicFrame(MicFrame& frame);
  void setLightColor(uint32_t color);
  int8_t pollThresholdAdjustment();
  void showSoundLevel(float soundDbfs,
                      float thresholdDbfs,
                      uint32_t nowMs);
  void updateScreen(uint32_t nowMs,
                    float soundDbfs,
                    float thresholdDbfs);

 private:
  void drawSoundLevel(float soundDbfs, float thresholdDbfs);

  UNIHIKER_K10 k10_;
  int16_t micBuffer_[CalmCueConfig::kInterleavedSamplesPerFrame] = {};
  uint32_t currentLightColor_ = CalmCueConfig::kColorOff;
  bool previousButtonAPressed_ = false;
  bool previousButtonBPressed_ = false;
  bool screenOn_ = false;
  uint32_t screenOffAtMs_ = 0;
  uint32_t lastScreenRefreshAtMs_ = 0;
};
