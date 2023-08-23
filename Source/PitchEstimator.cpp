#include "PitchEstimator.hpp"
#include <cmath>

PitchDetector::PitchDetector(int sampleRate, int bufferSize) {
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    this->maxLag = bufferSize / 2;
}

PitchDetector::~PitchDetector() {}

float PitchDetector::calculateAutocorrelation(std::vector<float>& audioBuffer, int timeStep, int lag) {
    float autocorrelation = 0.0f;
    for (int t =  timeStep; t < audioBuffer.size() - lag; t++) {
        autocorrelation += audioBuffer[t] * audioBuffer[t + lag];
    }
    return autocorrelation;
}

float PitchDetector::differenceFunction(std::vector<float>& audioBuffer, int t, int lag) {
    return calculateAutocorrelation(audioBuffer, t, 0) + calculateAutocorrelation(audioBuffer, t + lag, 0) - (2*calculateAutocorrelation(audioBuffer, t, lag));
}

float PitchDetector::CMNDF(std::vector<float>& audioBuffer, int t, int lag) {
    if(lag == 0)
    {
        return 1;
    }
    else
    {
        float sum = 0;
        for(int i = 0; i < lag; ++i)
        {
            sum += differenceResults[i + 1];
        }
        return differenceResults[lag] / sum * lag;
    }
}

float PitchDetector::estimatePitch(std::vector<float>& audioBuffer) {
    bool pitchFound = false;
    float lowestCorr = 0.0f;
    int index = 0;
    int minLag = minBounds;
    differenceResults.clear();
    
    for(int i = 0; i<=maxLag;++i)
    {
        differenceResults.push_back(differenceFunction(audioBuffer, 1, i));
    }
    
    for (int i = minLag; i <= maxLag; i++) {
        float cmndfVal = CMNDF(audioBuffer, 1, i);
        if(cmndfVal < threshold)
        {
            
            index = i;
            break;
        }
        if(cmndfVal < lowestCorr || lowestCorr == 0.0f)
        {
            if(lowestCorr != 0.0f && !pitchFound)
                pitchFound = true;
            
            lowestCorr = cmndfVal;
            index = i;
        }
    }
    
    if(pitchFound)
        return static_cast<float>(sampleRate) / static_cast<float>(index);
    else
        return -1.0f;
}

void PitchDetector::setMinBounds(float minBounds) { 
    this->minBounds = static_cast<int>(std::round(minBounds));
}

void PitchDetector::setThreshold(float threshold) { 
    this->threshold = threshold;
}





