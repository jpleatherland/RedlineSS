#pragma once

class GainStage2 {
  public:
    void prepare(double sampleRate);
    float processSample(float x, float gain);
};
