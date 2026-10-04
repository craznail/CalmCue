#include "K10Hardware.h"

#include <cmath>

#include "CalmCueConfig.h"

namespace {

float rmsToDbfs(float rms) {
  const float safeRms = rms < CalmCueConfig::kRmsFloor
                            ? CalmCueConfig::kRmsFloor
                            : rms;
  return 20.0F * log10f(safeRms / CalmCueConfig::kPcm16FullScale);
}

}  // namespace

void K10Hardware::begin() {
  k10_.begin();
  k10_.rgb->brightness(CalmCueConfig::kLedBrightness);
  k10_.rgb->write(-1, CalmCueConfig::kColorOff);
  k10_.initScreen(2);
  k10_.creatCanvas();
  digital_write(eLCD_BLK, 0);
}

bool K10Hardware::readMicFrame(MicFrame& frame) {
  frame = {};

  size_t bytesRead = 0;
  const uint32_t startedAtUs = micros();
  xSemaphoreTake(xI2SMutex, portMAX_DELAY);
  const esp_err_t result = i2s_read(
      I2S_NUM_0,
      micBuffer_,
      sizeof(micBuffer_),
      &bytesRead,
      pdMS_TO_TICKS(CalmCueConfig::kMicReadTimeoutMs));
  xSemaphoreGive(xI2SMutex);
  frame.readDurationUs = micros() - startedAtUs;
  frame.bytesRead = bytesRead;

  if (result != ESP_OK || bytesRead != sizeof(micBuffer_)) {
    return false;
  }

  const size_t interleavedSampleCount = bytesRead / sizeof(int16_t);
  if (interleavedSampleCount % CalmCueConfig::kMicChannelCount != 0) {
    return false;
  }

  const size_t samplesPerChannel =
      interleavedSampleCount / CalmCueConfig::kMicChannelCount;
  if (samplesPerChannel == 0) {
    return false;
  }

  int64_t channel0Sum = 0;
  int64_t channel1Sum = 0;
  int64_t channel0SquareSum = 0;
  int64_t channel1SquareSum = 0;

  for (size_t i = 0; i < samplesPerChannel; ++i) {
    const int32_t channel0 = micBuffer_[i * 2];
    const int32_t channel1 = micBuffer_[i * 2 + 1];
    channel0Sum += channel0;
    channel1Sum += channel1;
    channel0SquareSum += static_cast<int64_t>(channel0) * channel0;
    channel1SquareSum += static_cast<int64_t>(channel1) * channel1;
  }

  const double sampleCount = static_cast<double>(samplesPerChannel);
  const double channel0Mean = channel0Sum / sampleCount;
  const double channel1Mean = channel1Sum / sampleCount;
  double channel0Variance = channel0SquareSum / sampleCount -
                            channel0Mean * channel0Mean;
  double channel1Variance = channel1SquareSum / sampleCount -
                            channel1Mean * channel1Mean;

  if (channel0Variance < 0.0) {
    channel0Variance = 0.0;
  }
  if (channel1Variance < 0.0) {
    channel1Variance = 0.0;
  }

  frame.samplesPerChannel = samplesPerChannel;
  frame.channel0Mean = static_cast<float>(channel0Mean);
  frame.channel1Mean = static_cast<float>(channel1Mean);
  frame.channel0Rms = static_cast<float>(sqrt(channel0Variance));
  frame.channel1Rms = static_cast<float>(sqrt(channel1Variance));
  frame.channel0Dbfs = rmsToDbfs(frame.channel0Rms);
  frame.channel1Dbfs = rmsToDbfs(frame.channel1Rms);
  frame.maxRms = frame.channel0Rms > frame.channel1Rms
                     ? frame.channel0Rms
                     : frame.channel1Rms;
  frame.maxDbfs = rmsToDbfs(frame.maxRms);

  return true;
}

void K10Hardware::setLightColor(uint32_t color) {
  if (color == currentLightColor_) {
    return;
  }

  k10_.rgb->write(-1, color);
  currentLightColor_ = color;
}

int8_t K10Hardware::pollThresholdAdjustment() {
  const bool buttonAPressed = k10_.buttonA->isPressed();
  const bool buttonBPressed = k10_.buttonB->isPressed();

  int8_t adjustment = 0;
  if (buttonAPressed && !buttonBPressed && !previousButtonAPressed_) {
    adjustment = -1;
  } else if (buttonBPressed && !buttonAPressed && !previousButtonBPressed_) {
    adjustment = 1;
  }

  previousButtonAPressed_ = buttonAPressed;
  previousButtonBPressed_ = buttonBPressed;
  return adjustment;
}

void K10Hardware::drawSoundLevel(float soundDbfs, float thresholdDbfs) {
  k10_.canvas->canvasClear();
  k10_.canvas->canvasText(
      "CalmCue", 20, 30, 0x202020, k10_.canvas->eCNAndENFont24, 20, true);
  k10_.canvas->canvasText(
      String("Current: ") + String(soundDbfs, 1) + " dBFS",
      20, 85, 0x202020, k10_.canvas->eCNAndENFont24, 20, true);
  k10_.canvas->canvasText(
      String("Threshold: ") + String(thresholdDbfs, 0) + " dBFS",
      20, 135, 0x202020, k10_.canvas->eCNAndENFont24, 20, true);
  k10_.canvas->canvasText(
      "A: easier to trigger", 20, 205, 0x202020,
      k10_.canvas->eCNAndENFont16, 25, true);
  k10_.canvas->canvasText(
      "B: harder to trigger", 20, 240, 0x202020,
      k10_.canvas->eCNAndENFont16, 25, true);
  k10_.canvas->updateCanvas();
}

void K10Hardware::showSoundLevel(float soundDbfs,
                                 float thresholdDbfs,
                                 uint32_t nowMs) {
  drawSoundLevel(soundDbfs, thresholdDbfs);
  digital_write(eLCD_BLK, 1);
  screenOn_ = true;
  screenOffAtMs_ = nowMs + CalmCueConfig::kScreenOnAfterAdjustmentMs;
  lastScreenRefreshAtMs_ = nowMs;
}

void K10Hardware::updateScreen(uint32_t nowMs,
                               float soundDbfs,
                               float thresholdDbfs) {
  if (screenOn_ && static_cast<int32_t>(nowMs - screenOffAtMs_) >= 0) {
    digital_write(eLCD_BLK, 0);
    screenOn_ = false;
    return;
  }

  if (screenOn_ &&
      nowMs - lastScreenRefreshAtMs_ >=
          CalmCueConfig::kScreenRefreshIntervalMs) {
    drawSoundLevel(soundDbfs, thresholdDbfs);
    lastScreenRefreshAtMs_ = nowMs;
  }
}
