#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmod.hpp"
#include "Iir.h"

extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

enum
{
    PARAMCOUNT_FREQUENCY = 0,
    PARAMCOUNT_WIDTH,
    PARAMCOUNT_TOTAL
};

const float FREQUENCY_MIN = 0;
const float FREQUENCY_MAX = 23999;
const float FREQUENCY_DEFAULT = 400;

const float WIDTH_MIN = 1;
const float WIDTH_MAX = 2000;
const float WIDTH_DEFAULT = 50;

const int RAMP_LENGTH = 256;

FMOD_RESULT F_CALLBACK MFN_dspCreate(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspRelease(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspReset(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK MFN_dspProcess(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation);
FMOD_RESULT F_CALLBACK MFN_dspParameterSetFloat(FMOD_DSP_STATE* dsp, int index, float value);
FMOD_RESULT F_CALLBACK MFN_dspParameterGetFloat(FMOD_DSP_STATE* dsp, int index, float* value, char* valuestr);

static FMOD_DSP_PARAMETER_DESC parameterFrequency;
static FMOD_DSP_PARAMETER_DESC parameterWidth;

FMOD_DSP_PARAMETER_DESC* parameterDescriptions[PARAMCOUNT_TOTAL]{
    &parameterFrequency,
    &parameterWidth
};

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
    PARAMCOUNT_TOTAL,                       // number of parameters
    parameterDescriptions,                  // parameter descriptions
    MFN_dspParameterSetFloat,               // set float parameter
    0,                                      // set int parameter
    0,                                      // set bool parameter
    0,                                      // set data parameter
    MFN_dspParameterGetFloat,               // get float parameter
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
        FMOD_DSP_INIT_PARAMDESC_FLOAT(parameterFrequency, "Frequency", "Hz", "Frequency in Hz. 0 - 23999. Default = 0.", FREQUENCY_MIN, FREQUENCY_MAX, FREQUENCY_DEFAULT);
        FMOD_DSP_INIT_PARAMDESC_FLOAT(parameterWidth, "Width", "Hz", "Filterwidth in Hz. 0 - 2000. Default = 500", WIDTH_MIN, WIDTH_MAX, WIDTH_DEFAULT);
        return &FMOD_MFN_Desc;
    }
}

class MonoFilteredNoiseState
{
public:
    MonoFilteredNoiseState();
    void generate(float* outBuffer, unsigned int length);
    void initialize(float samplerate);
    void setTargetFrequency(float value);
    void setTargetWidth(float value);
    float getTargetFrequency() const { return targetFrequency; }
    float getTargetWidth() const { return targetWidth; }
    void reset();
    void cleanup();
private:
    // TODO: make filters a non-static amount
    Iir::ChebyshevII::BandPass<8>* filters[20];
    float generatedValue = 0;
    float filteredValue = 0;
    float samplerate = 0;
    float currentFrequency = 0;
    float targetFrequency = 0;
    float currentWidth = 0;
    float targetWidth = 0;
    int samplesLeftRampFrequency = 0;
    int samplesLeftRampWidth = 0;
    int filterCount = 0;
};

MonoFilteredNoiseState::MonoFilteredNoiseState()
{
}

void MonoFilteredNoiseState::generate(float* outBuffer, unsigned int length)
{
    if (samplesLeftRampFrequency || samplesLeftRampWidth) 
    {
        float frequencyDelta = (targetFrequency - currentFrequency) / samplesLeftRampFrequency;
        float widthDelta = (targetWidth - currentWidth) / samplesLeftRampWidth;
        while (length--) 
        {
            if (samplesLeftRampFrequency <= 0 && samplesLeftRampWidth <= 0) 
            {
                currentFrequency = targetFrequency;
                currentWidth = targetWidth;
                for (int i = 0; i < filterCount; i++)
                {
                    filters[i]->setup(samplerate, currentFrequency + currentFrequency * ((float)(i * 0.3f)), currentWidth, 60);
                    filters[i]->reset();
                }
                break;
            }
            if (--samplesLeftRampFrequency > 0) {
                currentFrequency += frequencyDelta;
            }

            if (--samplesLeftRampWidth > 0) {
                currentWidth += widthDelta;
            }

            for (int i = 0; i < filterCount; i++)
            {
                filters[i]->setup(samplerate, currentFrequency + currentFrequency * ((float)(i * 0.3f)), currentWidth, 60);
                filters[i]->reset();
            }

            generatedValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);
            filteredValue = 0;

            for (int i = 0; i < filterCount; i++)
            {
                filteredValue += filters[i]->filter(generatedValue);
            }

            *outBuffer++ = filteredValue / filterCount;
        }
    }

    while (length--) 
    {
        generatedValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);
        filteredValue = 0;

        for (int i = 0; i < filterCount; i++)
        {
            filteredValue += filters[i]->filter(generatedValue);
        }

        *outBuffer++ = filteredValue / filterCount;
    }
}

void MonoFilteredNoiseState::initialize(float samplerate)
{
    this->samplerate = samplerate;
    currentFrequency, targetFrequency = FREQUENCY_DEFAULT;
    currentWidth, targetWidth = WIDTH_DEFAULT;
    samplesLeftRampFrequency = 0;
    samplesLeftRampWidth = 0;
    filterCount = 3;

    for (int i = 0; i < filterCount; i++)
    {
        filters[i] = new Iir::ChebyshevII::BandPass<8>();
        filters[i]->setup(samplerate, currentFrequency + currentFrequency * ((float)(i * 0.3f)), currentWidth, 60);
    }
}

void MonoFilteredNoiseState::setTargetFrequency(float value)
{
    targetFrequency = value;
    samplesLeftRampFrequency = RAMP_LENGTH;
}

void MonoFilteredNoiseState::setTargetWidth(float value)
{
    targetWidth = value;
    samplesLeftRampWidth = RAMP_LENGTH;
}

void MonoFilteredNoiseState::reset()
{
    for (int i = 0; i < filterCount; i++)
    {
        filters[i]->reset();
    }
}

void MonoFilteredNoiseState::cleanup()
{
    for (int i = 0; i < filterCount; i++)
    {
        delete filters[i];
    }
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
    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;
    state->reset();
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK MFN_dspParameterSetFloat(FMOD_DSP_STATE* dsp, int index, float value)
{
    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;

    switch (index)
    {
    case PARAMCOUNT_FREQUENCY:
        state->setTargetFrequency(value);
        return FMOD_OK;
    case PARAMCOUNT_WIDTH:
        state->setTargetWidth(value);
        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK MFN_dspParameterGetFloat(FMOD_DSP_STATE* dsp, int index, float* value, char* valuestr)
{
    MonoFilteredNoiseState* state = (MonoFilteredNoiseState*)dsp->plugindata;

    switch (index) {
    case PARAMCOUNT_FREQUENCY:
        *value = state->getTargetFrequency();
        return FMOD_OK;
    case PARAMCOUNT_WIDTH:
        *value = state->getTargetWidth();
        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}