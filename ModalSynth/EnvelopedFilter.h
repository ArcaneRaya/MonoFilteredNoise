#pragma once

#include "Envelope.h"
#include "EnvelopePoint.h"
#include "Iir.h"

class EnvelopedFilter
{
public:
	EnvelopedFilter();
	EnvelopedFilter(const int samplerate);
	float ProcessSource(const float sourceSample);
	void SetSamplerate(const int samplerate);
	void SetFrequency(const float frequency);
	void AddEnvelopePoint(EnvelopePoint newPoint);
	void IncrementWithOneSample();
	void Reset();
private:
	void UpdateFilter();
	Envelope envelope;
	Iir::Butterworth::BandPass<8> filter;
	int samplerate = 48000;
	float currentFrequency = 1000;
	float currentWidth = 75;
};

