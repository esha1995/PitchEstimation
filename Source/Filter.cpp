//
//  DelayUnit.cpp
//  DelayPlugin
//
//  Created by James Kelly on 14/12/2018.
//  Copyright © 2018 James Kelly. All rights reserved.
//
//

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "FMODAPI.h"
#include "PitchEstimator.hpp"


extern "C"
{
    F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

// ==================== //
// CALLBACK DEFINITIONS //
// ==================== //
FMOD_RESULT Create_Callback                     (FMOD_DSP_STATE *dsp_state);
FMOD_RESULT Release_Callback                    (FMOD_DSP_STATE *dsp_state);
FMOD_RESULT Reset_Callback                      (FMOD_DSP_STATE *dsp_state);
FMOD_RESULT Read_Callback                       (FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels);
FMOD_RESULT Process_Callback                    (FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray, FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op);
FMOD_RESULT SetPosition_Callback                (FMOD_DSP_STATE *dsp_state, unsigned int pos);
FMOD_RESULT ShouldIProcess_Callback             (FMOD_DSP_STATE *dsp_state, FMOD_BOOL inputsidle, unsigned int length, FMOD_CHANNELMASK inmask, int inchannels, FMOD_SPEAKERMODE speakermode);

FMOD_RESULT SetFloat_Callback                   (FMOD_DSP_STATE *dsp_state, int index, float value);
FMOD_RESULT SetInt_Callback                     (FMOD_DSP_STATE *dsp_state, int index, int value);
FMOD_RESULT SetBool_Callback                    (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL value);
FMOD_RESULT SetData_Callback                    (FMOD_DSP_STATE *dsp_state, int index, void *data, unsigned int length);
FMOD_RESULT GetFloat_Callback                   (FMOD_DSP_STATE *dsp_state, int index, float *value, char *valuestr);
FMOD_RESULT GetInt_Callback                     (FMOD_DSP_STATE *dsp_state, int index, int *value, char *valuestr);
FMOD_RESULT GetBool_Callback                    (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL *value, char *valuestr);
FMOD_RESULT GetData_Callback                    (FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *valuestr);

FMOD_RESULT SystemRegister_Callback             (FMOD_DSP_STATE *dsp_state);
FMOD_RESULT SystemDeregister_Callback           (FMOD_DSP_STATE *dsp_state);
FMOD_RESULT SystemMix_Callback                  (FMOD_DSP_STATE *dsp_state, int stage);

// ==================== //
//      PARAMETERS      //
// ==================== //

// set all parameters in ENUM + a NUM_PARAMS which will know how many parameters the program has

enum
{
    THRESHOLD,
    MIN_BOUNDS,
    NUM_PARAMS
};

// create parameters as FMOD_DSP_PARAMETER DESC
static FMOD_DSP_PARAMETER_DESC threshold;
static FMOD_DSP_PARAMETER_DESC minBounds;

// create a list with NUM_PARAM elements to get parameters
FMOD_DSP_PARAMETER_DESC* PluginsParameters[NUM_PARAMS] =
{
    &threshold,
    &minBounds
};


// ==================== //
//     SET CALLBACKS    //
// ==================== //

FMOD_DSP_DESCRIPTION PluginCallbacks =
{
    FMOD_PLUGIN_SDK_VERSION,    // version
    "Pitch Estimator",     // name
    0x00010000,                 // plugin version
    1,                          // no. input buffers
    1,                          // no. output buffers
    Create_Callback,            // create
    Release_Callback,           // release
    Reset_Callback,             // reset
    Read_Callback,              // read
    Process_Callback,           // process
    SetPosition_Callback,       // setposition
    NUM_PARAMS,                          // no. parameter
    PluginsParameters,                          // pointer to parameter descriptions
    SetFloat_Callback,          // Set float
    SetInt_Callback,            // Set int
    SetBool_Callback,           // Set bool
    SetData_Callback,           // Set data
    GetFloat_Callback,          // Get float
    GetInt_Callback,            // Get int
    GetBool_Callback,           // Get bool
    GetData_Callback,           // Get data
    ShouldIProcess_Callback,    // Check states before processing
    0,                          // User data
    SystemRegister_Callback,    // System register
    SystemDeregister_Callback,  // System deregister
    SystemMix_Callback          // Mixer thread exucute / after execute
};


// assign the parameters with a name and value, to be controlled from FMOD
extern "C"
{
    F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription ()
    {
        FMOD_DSP_INIT_PARAMDESC_FLOAT(threshold, "Threshold", "", "", 0.01f, 1.0f, 0.1f);
        FMOD_DSP_INIT_PARAMDESC_FLOAT(minBounds, "Min Bounds", "", "", 1.0f, 200.0f, 20.0f);
        return &PluginCallbacks;
    }

}

// ==================== //
//     PLUGIN CLASS     //
// ==================== //

// in the plugin class you do all of your DSP stuff
class Plugin
{
public:
    Plugin() : estimator() {};
    void Prepare (FMOD_DSP_STATE *dsp_state, int numChannels, int numSamples);
    void Release (FMOD_DSP_STATE *dsp_state);
    void Process (float* inbuffer, float* outbuffer, unsigned int length, int channels, FMOD_DSP_STATE *dsp_state);
    void setParameterFloat(int index, float value);
    void getParameterFloat(int index, float* value);
    
    
private:
    int sampleRate = 44100;
    int numChannels = 0;
    int numSamples = 0;
    std::vector<float>* leftBuffer;
    std::vector<float>* rightBuffer;
    PitchDetector* estimator;
    int minBounds = 20;
    float threshold = 0.1f;
    float currentPitch = 0.0f;
};

void Plugin::Prepare(FMOD_DSP_STATE *dsp_state, int numChannels, int numSamples)
{
    FMOD_DSP_GETSAMPLERATE(dsp_state, &sampleRate);
    this->numChannels = numChannels;
    this->numSamples = numSamples;
    leftBuffer = new std::vector<float>(numSamples,0.0f);
    rightBuffer = new std::vector<float>(numSamples,0.0f);
    estimator = new PitchDetector(sampleRate, numSamples);
}

void Plugin::Process(float *inbuffer, float *outbuffer, unsigned int numSamples, int numChannels, FMOD_DSP_STATE *dsp_state)
{
    // update params
    
    estimator->setMinBounds(minBounds);
    estimator->setThreshold(threshold);
    
    // Loop through all samples
    for (unsigned int s = 0; s < numSamples; s++)
    {
        // Loop through all channels within the sample (audio is interleaved) i
        // increase pointer to index in buffer array each time to look at new channel
        for (unsigned int ch = 0; ch < numChannels; ch++, *outbuffer++, *inbuffer++)
        {
            if(ch==0)
            {
                // add sample to buffer and pass it directly to output
                float sample = *inbuffer;
                (*leftBuffer)[s] = sample;
                *outbuffer = sample;
            }
            else if(ch==1)
            {
                // add sample to buffer and pass it directly to output
                float sample = *inbuffer;
                (*rightBuffer)[s] = sample;
                *outbuffer = sample; // do nothing to audio
            }
            else
            {
                // pass directly to output
                *outbuffer = 0.0f;
            }
        }
    }
    
    /*
     If only one channel just use left pitch detector,
     else make signal mono and detect pitch
     */
    
    if(numChannels == 1)
    {
        currentPitch = estimator->estimatePitch((*leftBuffer));
    }
    else
    {
        // create a mono signal and estimate pitch on that
        std::vector<float> monoBuffer(numSamples);
        for(int i = 0; i<numSamples;++i)
        {
            monoBuffer[i] = ((*leftBuffer)[i] + (*rightBuffer)[i]) / 2.0f;
        }
        currentPitch = estimator->estimatePitch(monoBuffer);
    }

}

void Plugin::Release(FMOD_DSP_STATE *dsp_state)
{
}

void Plugin::setParameterFloat(int index, float value) {
    if(index == 0) // Update threshold
    {
        this->threshold = value;
    }
    if(index == 1) // Update min bounds
    {
        this->minBounds = value;
    }
}

void Plugin::getParameterFloat(int index, float *value)
{
    if(index == 0) // Get current pitch
    {
        *value = currentPitch;
    }
}

// set/get bool, int etc. can also be gennerated
    

// ======================= //
// CALLBACK IMPLEMENTATION //
// ======================= //

FMOD_RESULT Create_Callback                     (FMOD_DSP_STATE *dsp_state)
{
    // create our plugin class and attach to fmod
    Plugin* state = (Plugin* )FMOD_DSP_ALLOC(dsp_state, sizeof(Plugin));
    dsp_state->plugindata = state;
    if (!dsp_state->plugindata)
    {
        return FMOD_ERR_MEMORY;
    }
    
    return FMOD_OK;
}

FMOD_RESULT Release_Callback                    (FMOD_DSP_STATE *dsp_state)
{
    // release our plugin class
    Plugin* state = (Plugin* )dsp_state->plugindata;
    state->Release(dsp_state);
    FMOD_DSP_FREE(dsp_state, state);
    return FMOD_OK;
}

FMOD_RESULT Reset_Callback                      (FMOD_DSP_STATE *dsp_state)
{
    return FMOD_OK;
}

FMOD_RESULT Read_Callback                       (FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
    return FMOD_OK;
}

FMOD_RESULT Process_Callback                    (FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray, FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op)
{
    Plugin* state = (Plugin* )dsp_state->plugindata;
    
    switch (op) {
        case FMOD_DSP_PROCESS_QUERY:
            if (outbufferarray && inbufferarray)
            {
            outbufferarray[0].bufferchannelmask[0] = inbufferarray[0].bufferchannelmask[0];
            outbufferarray[0].buffernumchannels[0] = inbufferarray[0].buffernumchannels[0];
            outbufferarray[0].speakermode       = inbufferarray[0].speakermode;
            }
            
            
            
            if (inputsidle)
            {
                return FMOD_ERR_DSP_DONTPROCESS;
            }
            
            // QUERY is before process is run first time, here we call prepare function of plugin to give number of channels, number of samples, sample rate
            state->Prepare(dsp_state, outbufferarray[0].buffernumchannels[0], length);
            
            break;
            
        case FMOD_DSP_PROCESS_PERFORM:
            
            if (inputsidle)
            {
                return FMOD_ERR_DSP_DONTPROCESS;
            }
            
            // actually process
            state->Process(inbufferarray[0].buffers[0], outbufferarray[0].buffers[0], length, outbufferarray[0].buffernumchannels[0], dsp_state);
            
            return FMOD_OK;
            break;
    }
    
    return FMOD_OK;
}

FMOD_RESULT SetPosition_Callback                (FMOD_DSP_STATE *dsp_state, unsigned int pos)
{
    return FMOD_OK;
}

FMOD_RESULT ShouldIProcess_Callback             (FMOD_DSP_STATE *dsp_state, FMOD_BOOL inputsidle, unsigned int length, FMOD_CHANNELMASK inmask, int inchannels, FMOD_SPEAKERMODE speakermode)
{
    if (inputsidle)
    {
        return FMOD_ERR_DSP_DONTPROCESS;
    }
    return FMOD_OK;
}

FMOD_RESULT SetFloat_Callback                   (FMOD_DSP_STATE *dsp_state, int index, float value)
{
    Plugin* state = (Plugin* )dsp_state->plugindata;
    state->setParameterFloat(index, value);
    return FMOD_OK;
}

FMOD_RESULT SetInt_Callback                     (FMOD_DSP_STATE *dsp_state, int index, int value)
{
    return FMOD_OK;
}

FMOD_RESULT SetBool_Callback                    (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL value)
{
    return FMOD_OK;
}

FMOD_RESULT SetData_Callback                    (FMOD_DSP_STATE *dsp_state, int index, void *data, unsigned int length)
{
    return FMOD_OK;
}

FMOD_RESULT GetFloat_Callback                   (FMOD_DSP_STATE *dsp_state, int index, float *value, char *valuestr)
{
    Plugin* state = (Plugin* )dsp_state->plugindata;
    state->getParameterFloat(index, value);
    return FMOD_OK;
}

FMOD_RESULT GetInt_Callback                     (FMOD_DSP_STATE *dsp_state, int index, int *value, char *valuestr)
{
    return FMOD_OK;
}

FMOD_RESULT GetBool_Callback                    (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL *value, char *valuestr)
{
    return FMOD_OK;
}

FMOD_RESULT GetData_Callback                    (FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *valuestr)
{
    return FMOD_OK;
}

FMOD_RESULT SystemRegister_Callback             (FMOD_DSP_STATE *dsp_state)
{
    return FMOD_OK;
}

FMOD_RESULT SystemDeregister_Callback           (FMOD_DSP_STATE *dsp_state)
{
    return FMOD_OK;
}

FMOD_RESULT SystemMix_Callback                  (FMOD_DSP_STATE *dsp_state, int stage)
{
    return FMOD_OK;
}
