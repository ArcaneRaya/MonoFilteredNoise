#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include "fmod.hpp"

extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

enum {
	PARAMCOUNT_DATA = 0,
	PARAMCOUNT_TOTAL
};

FMOD_RESULT F_CALLBACK DataContainer_create(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK DataContainer_release(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK DataContainer_reset(FMOD_DSP_STATE* dsp);
FMOD_RESULT F_CALLBACK DataContainer_process(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation);
FMOD_RESULT F_CALLBACK DataContainer_parameterSetData(FMOD_DSP_STATE* dsp, int index, void* data, unsigned int length);
FMOD_RESULT F_CALLBACK DataContainer_parameterGetData(FMOD_DSP_STATE* dsp, int index, void** value, unsigned int* length, char* valuestr);

static FMOD_DSP_PARAMETER_DESC parameterData;

FMOD_DSP_PARAMETER_DESC* parameterDescriptions[PARAMCOUNT_TOTAL]{
	&parameterData
};


FMOD_DSP_DESCRIPTION FMOD_DataContainer_Desc =
{
	FMOD_PLUGIN_SDK_VERSION,
	"DataContainer",                        // name (max 32char)
    0x00010000,                             // plug-in version
    0,                                      // number of input buffers to process
    1,                                      // number of output buffers to process
    DataContainer_create,                   // create
    DataContainer_release,                  // release
    DataContainer_reset,                    // reset
    0,                                      // read
    DataContainer_process,                  // process
    0,                                      // set position
    PARAMCOUNT_TOTAL,                       // number of parameters
    parameterDescriptions,                  // parameter descriptions
    0,                                      // set float parameter
    0,                                      // set int parameter
    0,                                      // set bool parameter
    DataContainer_parameterSetData,         // set data parameter
    0,                                      // get float parameter
    0,                                      // get int parameter
    0,                                      // get bool parameter
    DataContainer_parameterGetData,         // get data parameter
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
        FMOD_DSP_INIT_PARAMDESC_DATA(parameterData, "Data", ".txt", "Some Data type", 0);
        return &FMOD_DataContainer_Desc;
    }
}

class EnvelopePoint
{
public:
    EnvelopePoint();
    EnvelopePoint(int length, float target);
    int samplelength = 0;
    float targetValue = 0;
};

EnvelopePoint::EnvelopePoint() 
{

}

EnvelopePoint::EnvelopePoint(int length, float target)
{
    samplelength = length;
    targetValue = target;
}

const int MAX_ENVELOPEPOINT_COUNT = 10;

class ModeData 
{
public:
    ModeData();
    int frequency = 0;
    EnvelopePoint envelopePoints[MAX_ENVELOPEPOINT_COUNT];
    int envelopePointCount = 10;
};

ModeData::ModeData()
{
    frequency = 0;
    envelopePointCount = 10;
}


const int MAXNUMLENGTH = 5;
const int MAX_AMPLITUDE_LENGTH = 8;
class DataContainerState
{
public:
    DataContainerState();
    void generate(float* outBuffer, unsigned int length);
    void setData(void* newDataPointer, unsigned int newDataLength);
    bool parseForFrequency(char* data, int* position, const int length, int* result);
    bool parseForLength(char* data, int* position, const int length, int* result);
    bool parseForAmplitude(char* data, int* position, const int length, float* result);
    bool validateData(void* newDataPointer, unsigned int newDataLength);
    void* getData();
    unsigned int getDataLength();
    void reset();
    void cleanup();
private:
    void* dataPointer;
    unsigned int dataLength;
    ModeData* modeData;
};

DataContainerState::DataContainerState()
{
}

void DataContainerState::generate(float* outBuffer, unsigned int length)
{
    while (length--) 
    {
        *outBuffer++ = 0;
    }
}

void DataContainerState::setData(void* newDataPointer, unsigned int newDataLength)
{
    if (newDataPointer == dataPointer && newDataLength == dataLength) { return; }

    dataPointer = newDataPointer;
    dataLength = newDataLength;

    if (modeData != nullptr) {
        delete modeData;
    }

    modeData = new ModeData();

    char* cPtr = (char*)dataPointer;
    int position = 0;
    int length = 0;
    float amplitude = 0;

    if (parseForFrequency(cPtr, &position, dataLength, &modeData->frequency) == false) {
        return;
    }

    int currentEnvelopePointCount = 0;
    while (position < dataLength && currentEnvelopePointCount < MAX_ENVELOPEPOINT_COUNT) {

        if (parseForLength(cPtr, &position, dataLength, &modeData->envelopePoints[currentEnvelopePointCount].samplelength) == false) {
            return;
        }
        if (parseForAmplitude(cPtr, &position, dataLength, &modeData->envelopePoints[currentEnvelopePointCount].targetValue) == false) {
            return;
        }

        currentEnvelopePointCount++;
    }
    modeData->envelopePointCount = currentEnvelopePointCount;
}

bool DataContainerState::parseForFrequency(char* data, int* position, const int length, int* result)
{
    int startPoint = *position;
    while (*position <= length) {
        if ((*position) - startPoint > MAXNUMLENGTH) {
            return false;
        }

        if (*(data + *position) == '-') {
            return false;
        }

        if (*(data + *position) == ';' || *position == length) {
            char value[(MAXNUMLENGTH + 1)];
            for (int i = 0; i < *position - startPoint; i++)
            {
                value[i] = *(data + startPoint + i);
            }
            value[*position - startPoint] = 0;
            *result = std::atoi(value);
            (*position)++;
            return true;
        }
        (*position)++;
    }
    return false;
}

bool DataContainerState::parseForLength(char* data, int* position, const int length, int* result)
{
    int startPoint = *position;
    while (*position <= length) {
        if ((*position) - startPoint > MAXNUMLENGTH) {
            return false;
        }

        if (*(data + *position) == ';') {
            return false;
        }

        if (*(data + *position) == '-' || *position == length) {
            char value[(MAXNUMLENGTH + 1)];
            for (int i = 0; i < *position - startPoint; i++)
            {
                value[i] = *(data + startPoint + i);
            }
            value[*position - startPoint] = 0;
            *result = std::atoi(value);
            (*position)++;
            return true;
        }
        (*position)++;
    }
    return false;
}

bool DataContainerState::parseForAmplitude(char* data, int* position, const int length, float* result)
{
    int startPoint = *position;
    while (*position <= length) {
        if ((*position) - startPoint > MAX_AMPLITUDE_LENGTH) {
            return false;
        }

        if (*(data + *position) == '-') {
            return false;
        }

        if (*(data + *position) == ';' || *position == length) {
            char value[(MAX_AMPLITUDE_LENGTH + 1)];
            for (int i = 0; i < *position - startPoint; i++)
            {
                value[i] = *(data + startPoint + i);
            }
            value[*position - startPoint] = 0;
            *result = std::atof(value);
            (*position)++;
            return true;
        }
        (*position)++;
    }
    return false;
}

bool DataContainerState::validateData(void* newDataPointer, unsigned int newDataLength)
{
    //char* cPtr = (char*)newDataPointer;
    //int startPoint = 0;
    //for (int i = 0; i <= newDataLength; i++)
    //{
    //    if (*(cPtr + i) == ';' || i == newDataLength) {
    //        if (i - startPoint > MAXNUMLENGTH) {
    //            return false;
    //        }
    //        if (i - startPoint == 0) {
    //            return false;
    //        }
    //        startPoint = i + 1;
    //    }
    //}
    return true;
}

void* DataContainerState::getData()
{
    return dataPointer;
}

unsigned int DataContainerState::getDataLength()
{
    return dataLength;
}

void DataContainerState::reset()
{
}

void DataContainerState::cleanup()
{
    if (modeData != nullptr) {
        delete modeData;
    }
}

FMOD_RESULT F_CALLBACK DataContainer_create(FMOD_DSP_STATE* dsp) 
{
    dsp->plugindata = (DataContainerState*)FMOD_DSP_ALLOC(dsp, sizeof(DataContainerState));

    if (!dsp->plugindata) 
    {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DataContainer_release(FMOD_DSP_STATE* dsp) 
{
    DataContainerState* state = (DataContainerState*)dsp->plugindata;
    FMOD_DSP_FREE(dsp, state);
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DataContainer_reset(FMOD_DSP_STATE* dsp)
{
    DataContainerState* state = (DataContainerState*)dsp->plugindata;
    state->reset();
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DataContainer_process(FMOD_DSP_STATE* dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* inBufferArray, FMOD_DSP_BUFFER_ARRAY* outBufferArray, FMOD_BOOL inputsIdle, FMOD_DSP_PROCESS_OPERATION processOperation)
{
    DataContainerState* state = (DataContainerState*)dsp->plugindata;

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

FMOD_RESULT F_CALLBACK DataContainer_parameterSetData(FMOD_DSP_STATE* dsp, int index, void* data, unsigned int length)
{
    DataContainerState* state = (DataContainerState*)dsp->plugindata;

    if (state->validateData(data, length) == false) {
        return FMOD_ERR_FORMAT;
    }

    state->setData(data, length);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DataContainer_parameterGetData(FMOD_DSP_STATE* dsp, int index, void** value, unsigned int* length, char* valuestr)
{
    DataContainerState* state = (DataContainerState*)dsp->plugindata;

    *value = state->getData();
    *length = state->getDataLength();

    return FMOD_OK;
}
