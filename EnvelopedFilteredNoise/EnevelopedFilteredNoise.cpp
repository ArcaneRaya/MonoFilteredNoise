#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmod.hpp"
#include "Iir.h"

extern "C"
{
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

enum
{
	PARAMCOUNT_ATTACK = 0,
	PARAMCOUNT_HOLD,
	PARAMCOUNT_DECAY,
	PARAMCOUNT_TOTAL
};

const float PARAM_ATTACK_MIN = 10;
const float PARAM_ATTACK_MAX = 1000;
const float PARAM_ATTACK_DEFAULT = 700;

const float PARAM_HOLD_MIN = 10;
const float PARAM_HOLD_MAX = 1000;
const float PARAM_HOLD_DEFAULT = 5000;

const float PARAM_DECAY_MIN = 10;
const float PARAM_DECAY_MAX = 1000;
const float PARAM_DECAY_DEFAULT = 100;

FMOD_RESULT F_CALLBACK EFN_create(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK EFN_release(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK EFN_reset(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK EFN_process(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation);

FMOD_DSP_DESCRIPTION FMOD_EFN_Desc =
{
    FMOD_PLUGIN_SDK_VERSION,                // plugin SDK version
    "EnvelopedFilteredNoise",               // name (max 32char)
    0x00010000,                             // plug-in version
    0,                                      // number of input buffers to process
    1,                                      // number of output buffers to process
    EFN_create,                             // create
    EFN_release,                            // release
    EFN_reset,                              // reset
    0,                                      // read
    EFN_process,                            // process
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
        return &FMOD_EFN_Desc;
    }
}

class EnvelopePoint
{
public:
    EnvelopePoint();
    EnvelopePoint(int sampleLength, float targetValue);
    int samplelength;
    float targetValue;
};

EnvelopePoint::EnvelopePoint()
{
    samplelength = 0;
    targetValue = 0;
}

EnvelopePoint::EnvelopePoint(int sampleLength, float targetValue)
{
    this->samplelength = sampleLength;
    this->targetValue = targetValue;
}

class EnvelopedFilteredNoiseState
{
public:
    EnvelopedFilteredNoiseState();
    void initialize(float samplerate);
    void generate(float* outBuffer, unsigned int length);
    void reset();
    void cleanup();
private:
    EnvelopePoint* envelope;
    int envelopePointCount;
    int currentEnvelopePoint;
    int samplesLeftEnvelopePoint;
    float targetLevelEnvelopePoint;

    float currentLevel;

    int samplesLeftAttack;
    int samplesLeftHold;
    int samplesLeftDecay;

    float samplerate;
};

EnvelopedFilteredNoiseState::EnvelopedFilteredNoiseState()
{
}

void EnvelopedFilteredNoiseState::initialize(float samplerate)
{
    this->samplerate = samplerate;
    reset();
}

void EnvelopedFilteredNoiseState::generate(float* outBuffer, unsigned int length)
{
    float generatedValue;
    float delta;
    float gain = currentLevel;

    while (samplesLeftEnvelopePoint <= 0)
    {
        gain = targetLevelEnvelopePoint;
        currentEnvelopePoint++;
        if (currentEnvelopePoint < envelopePointCount) {
            samplesLeftEnvelopePoint = envelope[currentEnvelopePoint].samplelength;
            targetLevelEnvelopePoint = envelope[currentEnvelopePoint].targetValue;
        }
        else {
            break;
        }
    }

    if (samplesLeftEnvelopePoint)
    {
        delta = (targetLevelEnvelopePoint - currentLevel) / samplesLeftEnvelopePoint;
        while (length) 
        {
            if (--samplesLeftEnvelopePoint)
            {
                gain += delta;
                generatedValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);
                *outBuffer++ = generatedValue * gain;
            }
            else
            {
                gain = targetLevelEnvelopePoint;
                currentEnvelopePoint++;
                if (currentEnvelopePoint < envelopePointCount) 
                {
                    samplesLeftEnvelopePoint = envelope[currentEnvelopePoint].samplelength;
                    targetLevelEnvelopePoint = envelope[currentEnvelopePoint].targetValue;
                    delta = (targetLevelEnvelopePoint - currentLevel) / samplesLeftEnvelopePoint;
                }
                else 
                {
                    break;
                }
            }
            length--;
        }
    }

    while (length--)
    {
        *outBuffer++ = 0;
    }

    currentLevel = gain;
}

void EnvelopedFilteredNoiseState::reset()
{
    currentEnvelopePoint = 0;
    envelopePointCount = 7;
    envelope = new EnvelopePoint[envelopePointCount];
    envelope[0] = EnvelopePoint(0, 0);
    envelope[1] = EnvelopePoint((PARAM_ATTACK_DEFAULT / 1000) * samplerate, 1);
    envelope[2] = EnvelopePoint((PARAM_HOLD_DEFAULT / 1000) * samplerate, .7f);
    envelope[3] = EnvelopePoint((PARAM_DECAY_DEFAULT / 1000) * samplerate, .5f);
    envelope[4] = EnvelopePoint((PARAM_ATTACK_DEFAULT / 1000) * samplerate, 1);
    envelope[5] = EnvelopePoint((PARAM_DECAY_DEFAULT / 1000) * samplerate, .95f);
    envelope[6] = EnvelopePoint((PARAM_HOLD_DEFAULT / 1000) * samplerate, 0);

    samplesLeftEnvelopePoint = envelope[currentEnvelopePoint].samplelength;
    targetLevelEnvelopePoint = envelope[currentEnvelopePoint].targetValue;
    currentLevel = 0;

    samplesLeftAttack = (PARAM_ATTACK_DEFAULT / 1000) * samplerate;
    samplesLeftHold = (PARAM_HOLD_DEFAULT / 1000) * samplerate;
    samplesLeftDecay = (PARAM_DECAY_DEFAULT / 1000) * samplerate;
}

void EnvelopedFilteredNoiseState::cleanup()
{
    delete envelope;
}


FMOD_RESULT F_CALLBACK EFN_create(FMOD_DSP_STATE* dsp)
{
    dsp->plugindata = (EnvelopedFilteredNoiseState*)FMOD_DSP_ALLOC(dsp, sizeof(EnvelopedFilteredNoiseState));
    if (!dsp->plugindata)
    {
        return FMOD_ERR_MEMORY;
    }

    EnvelopedFilteredNoiseState* state = (EnvelopedFilteredNoiseState*)dsp->plugindata;
    int rate;
    dsp->functions->getsamplerate(dsp, &rate);
    state->initialize(rate);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK EFN_release(FMOD_DSP_STATE* dsp)
{
    EnvelopedFilteredNoiseState* state = (EnvelopedFilteredNoiseState*)dsp->plugindata;
    state->cleanup();
    FMOD_DSP_FREE(dsp, state);
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK EFN_reset(FMOD_DSP_STATE* dsp)
{
    EnvelopedFilteredNoiseState* state = (EnvelopedFilteredNoiseState*)dsp->plugindata;
    state->reset();
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK EFN_process(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation)
{
    EnvelopedFilteredNoiseState* state = (EnvelopedFilteredNoiseState*)dsp->plugindata;

    if (processOperation == FMOD_DSP_PROCESS_QUERY)
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