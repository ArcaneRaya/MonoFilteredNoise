#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmod.hpp"
#include "Iir.h"

#include "ModalSynthState.h"

extern "C" 
{
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

FMOD_RESULT F_CALLBACK ModalSynthCreate(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK ModalSynthRelease(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK ModalSynthReset(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK ModalSynthProcess(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation);
FMOD_RESULT SetSpeakerModeAndChannelCount(FMOD_DSP_BUFFER_ARRAY* outBufferArray);
FMOD_RESULT FillAudioBuffer(FMOD_DSP_STATE* dsp, FMOD_DSP_BUFFER_ARRAY* outBufferArray, unsigned int length);
void InitializeModalSynthState(FMOD_DSP_STATE* dsp);

FMOD_DSP_DESCRIPTION FMOD_ModalSynth_Description =
{
    FMOD_PLUGIN_SDK_VERSION,                // plugin SDK version
    "Modal Synth",                          // name (max 32char)
    0x00010000,                             // plug-in version
    0,                                      // number of input buffers to process
    1,                                      // number of output buffers to process
    ModalSynthCreate,                       // create
    ModalSynthRelease,                      // release
    ModalSynthReset,                        // reset
    0,                                      // read
    ModalSynthProcess,                      // process
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
        return &FMOD_ModalSynth_Description;
    }
}


FMOD_RESULT F_CALLBACK ModalSynthCreate(FMOD_DSP_STATE* dsp)
{
    dsp->plugindata = (ModalSynthState*)FMOD_DSP_ALLOC(dsp, sizeof(ModalSynthState));
    if (!dsp->plugindata) 
    {
        return FMOD_ERR_MEMORY;
    }

    InitializeModalSynthState(dsp);
    
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ModalSynthRelease(FMOD_DSP_STATE* dsp) 
{
    ModalSynthState* state = (ModalSynthState*)dsp->plugindata;
    state->release();
    FMOD_DSP_FREE(dsp, state);
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ModalSynthReset(FMOD_DSP_STATE* dsp)
{
    ModalSynthState* state = (ModalSynthState*)dsp->plugindata;
    state->reset();
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ModalSynthProcess(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* /*inBufferArray*/, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL /*inputsIdle*/, FMOD_DSP_PROCESS_OPERATION processOperation) 
{
    if (processOperation == FMOD_DSP_PROCESS_QUERY) 
    {
        return SetSpeakerModeAndChannelCount(outBufferArray);
    }

    return FillAudioBuffer(dsp, outBufferArray, length);
}

void InitializeModalSynthState(FMOD_DSP_STATE* dsp) 
{
    ModalSynthState* state = (ModalSynthState*)dsp->plugindata;
    int samplerate;
    dsp->functions->getsamplerate(dsp, &samplerate);
    state->initialize(samplerate);
}

FMOD_RESULT SetSpeakerModeAndChannelCount(FMOD_DSP_BUFFER_ARRAY* outBufferArray) {
    if (outBufferArray) {
        outBufferArray->speakermode = FMOD_SPEAKERMODE_MONO;
        outBufferArray->buffernumchannels[0] = 1;
    }
    return FMOD_OK;
}

FMOD_RESULT FillAudioBuffer(FMOD_DSP_STATE* dsp, FMOD_DSP_BUFFER_ARRAY* outBufferArray, unsigned int length)
{
    ModalSynthState* state = (ModalSynthState*)dsp->plugindata;
    state->generate(outBufferArray->buffers[0], length);
    return FMOD_OK;
}