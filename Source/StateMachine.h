/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Util.h"

class ImpulseSelectionStateMachine
{
public:
    
    ImpulseSelectionStateMachine();
    ~ImpulseSelectionStateMachine();
    
    void stateMachine (float azimuth, int elevation);
    
    int UR, LR, UL, LL;
    
};
