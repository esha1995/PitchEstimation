#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "fmod.hpp"
/*
 
 class PitchEstimator
 {
 public:
 PitchEstimator();
 void UpdateParams(int sampleRate, int windowSize, int step, int minBounds, int maxBounds, float threshold);
 float ACF(float* x, int tau, int step);
 float DF(float* x, int tau);
 float PitchDetector(float* x);
 private:
 int sampleRate = 44100;
 int estimationLength = sampleRate * 2;
 int windowSize = 200;
 int step = 1;
 int minBounds = 20;
 int maxBounds = 2000;
 float threshold = 0.1f;
 int argMin = 0;
 };
 
 */

#include <vector>

class PitchDetector {
public:
    PitchDetector(int sampleRate, int bufferSize);
    ~PitchDetector();
    float estimatePitch(std::vector<float>& audioBuffer);
    void setMinBounds(float minBounds);
    void setThreshold(float threshold);
    float randomVal = 0;
private:
    float threshold = 0.1f;
    int minBounds = 20;
    int sampleRate;
    int bufferSize;
    int maxLag;
    std::vector<float> differenceResults;
    int bufferIndex;
    float calculateAutocorrelation(std::vector<float>& audioBuffer, int t, int lag);
    float differenceFunction(std::vector<float>& audioBuffer, int t, int lag);
    float CMNDF(std::vector<float>& audioBuffer, int t, int lag);
};








