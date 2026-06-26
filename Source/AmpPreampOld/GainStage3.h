#pragma once

class GainStage3 {
  public:
    void prepare(double sampleRate);
    float processSample(float x, float gain);
};
