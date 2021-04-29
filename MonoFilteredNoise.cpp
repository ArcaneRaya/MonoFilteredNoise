#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmod.hpp"
#include "Iir.h"

extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}


FMOD_RESULT F_CALLBACK MFN_dspCreate(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspRelease(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspReset(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspProcess(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation);

FMOD_DSP_DESCRIPTION FMOD_MFN_Desc =
{
    FMOD_PLUGIN_SDK_VERSION,                // plugin SDK version
    "MonoFilteredNoise",                    // name (max 32char)
    0x00010000,                             // plug-in version
    0,                                      // number of input buffers to process
    1,                                      // number of output buffers to process
    MFN_dspCreate,                          // create
    MFN_dspRelease,                         // release
    MFN_dspReset,                           // reset
    0,                                      // read
    MFN_dspProcess,                         // process
    0,                                      // set position
    0,                                      // number of parameters
    0,                                      // parameter descriptions
    0,                                      // set float parameter
    0,                                      // set int parameter
    0,                                      // set bool parameter
    0,                                      // set data parameter
    0,                                      // get float parameter
    0,                                      // get int parameter
    0,                                      // get bool parameter
    0,                                      // get data parameter
    0,                                      // should i process
    0,                                      // userdata
    0,                                      // Register
    0,                                      // Deregister
    0                                       // Mix
};

extern "C"
{
    F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription()
    {
        return &FMOD_MFN_Desc;
    }
}

class MonoFilteredNoiseState
{
public:
    MonoFilteredNoiseState();
    void generate(float* outBuffer, unsigned int length);
    void initialize(float samplerate);
    void cleanup();
private:
    Iir::Butterworth::BandPass<8>* filter;
    float generatedValue = 0;
    float filteredValue = 0;
};

MonoFilteredNoiseState::MonoFilteredNoiseState()
{
}

void MonoFilteredNoiseState::generate(float* outBuffer, unsigned int length)
{
    while (length--) 
    {
        generatedValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);
        filteredValue = filter->filter(generatedValue);
        *outBuffer++ = filteredValue; 
    }
}

void MonoFilteredNoiseState::initialize(float samplerate)
{
    filter = new Iir::Butterworth::BandPass<8>();
    filter->setup(samplerate, 500, 250);
}

void MonoFilteredNoiseState::cleanup()
{
    delete filter;
}

FMOD_RESULT F_CALLBACK MFN_dspCreate(FMOD_DSP_STATE* dsp) 
{
    dsp->plugindata = (MonoFilteredNoiseState*)FMOD_DSP_ALLOC(dsp, sizeof(MonoFilteredNoiseState));
    if (!dsp->plugindata)
    {
        return FMOD_ERR_MEMORY;
    }

    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;
    int rate;
    dsp->functions->getsamplerate(dsp, &rate);
    state->initialize(rate);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK MFN_dspRelease(FMOD_DSP_STATE* dsp)
{
    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;
    state->cleanup();
    FMOD_DSP_FREE(dsp, state);
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK MFN_dspProcess(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* /*inbufferarray*/, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL /*inputsidle*/, FMOD_DSP_PROCESS_OPERATION op)
{
    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;

    if (op == FMOD_DSP_PROCESS_QUERY)
    {
        if (outBufferArray) 
        {
            outBufferArray->speakermode = FMOD_SPEAKERMODE_MONO;
            outBufferArray->buffernumchannels[0] = 1;
        }

        return FMOD_OK;
    }

    state->generate(outBufferArray->buffers[0], length);
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK MFN_dspReset(FMOD_DSP_STATE* dsp) 
{
    return FMOD_OK;
}