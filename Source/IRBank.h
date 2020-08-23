/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include <iostream>
#include <string>
#define HRIR_SIZE 256
#define HRIR_SIZE_FILE_SIZE 1068

class IRBank
{
public:
    IRBank();
    ~IRBank();
    
    void build();
    
    AudioFormatReader* reader;
    int streamNumChannels;
    int streamNumSamples;
    
    AudioSampleBuffer bufferArray[BinaryData::namedResourceListSize];
};
