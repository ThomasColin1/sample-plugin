/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2022 - Raw Material Software Limited=

  ==============================================================================
*/

#pragma once

#include "DemoUtilities.h"
#include "DSPDemos_Common.h"

#include <iostream>

using namespace dsp;
using namespace std;


//==============================================================================
struct GainDemoDSP
{
    void prepare (const ProcessSpec&)
    {
        gain.setGainDecibels (-6.0f);
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        gain.process (context);
    }

    void reset()
    {
        gain.reset();
    }

    void updateParameters()
    {
        gain.setGainDecibels (static_cast<float> (gainParam.getCurrentValue()));
    }

    //==============================================================================
    Gain<float> gain;
    SliderParameter gainParam { { -100.0, 20.0 }, 3.0, -6.0, "Gain", "dB" };

    std::vector<DSPDemoParameterBase*> parameters { &gainParam };
};

struct GainDemo    : public Component
{
    GainDemo()
    {
        addAndMakeVisible (fileReaderComponent);
    }

    void resized() override
    {
        fileReaderComponent.setBounds (getLocalBounds());
    }

    AudioFileReaderComponent<GainDemoDSP> fileReaderComponent;
};
