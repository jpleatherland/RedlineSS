#pragma once

class GainStage1 {
  public:
    void prepare(double sampleRate);
    float processSample(float x, float gain, float bias);
};
