#include "EnvelopedFilter.h"

EnvelopedFilter::EnvelopedFilter(const int samplerate) : EnvelopedFilter()
{
	this->samplerate = samplerate;
	UpdateFilter();
}

float EnvelopedFilter::ProcessSource(const float sourceSample)
{
	return filter.filter(sourceSample) * envelope.GetCurrentLevel();
}

void EnvelopedFilter::SetSamplerate(const int samplerate)
{
	this->samplerate = samplerate;
	UpdateFilter();
}

void EnvelopedFilter::SetFrequency(const float frequency)
{
	currentFrequency = frequency;
	float widthModulation = frequency / 2000;
	currentWidth = 25 / widthModulation;
	if (currentWidth > 25) {
		currentWidth = 25;
	}
	UpdateFilter();
}

void EnvelopedFilter::AddEnvelopePoint(EnvelopePoint newPoint)
{
	envelope.AddEnvelopePoint(newPoint);
}

void EnvelopedFilter::IncrementWithOneSample()
{
	envelope.IncrementWithOneSample();
}

void EnvelopedFilter::Reset()
{
	envelope.Reset();
	filter.reset();
	filter.setup(samplerate, currentFrequency, currentWidth);
}

EnvelopedFilter::EnvelopedFilter()
{
	envelope = Envelope();
}

void EnvelopedFilter::UpdateFilter()
{
	filter.reset();
	filter.setup(samplerate, currentFrequency, currentWidth);
	//filter.setup(samplerate, currentFrequency, currentWidth, permittedStopbandDB);
}
