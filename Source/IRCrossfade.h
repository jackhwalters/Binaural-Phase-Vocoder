/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <api/fftw3.h>
#include "IRBank.h"
#include "Util.h"
#include <cmath>

class IRCrossfade
{
public:
    IRCrossfade();
    ~IRCrossfade();
    
    void initFFT();
    void loadImpulses(int channel, AudioSampleBuffer impulse1, AudioSampleBuffer impulse2, AudioSampleBuffer impulse3, AudioSampleBuffer impulse4);
    void impulseFFTBlend();
    void backwardFFTandStore(int channel, int numberOfInputChannels);
    void deinitFFT();
    
    AudioSampleBuffer crossfadedImpulse;
        
private:
    
    // Overlap-add architecture
    int fftImpulseActualTransformSize_;
    double fftImpulseScaleFactor_;
    
    //FFTW
    fftw_complex *fftImpulseTimeDomain_1,
    *fftImpulseTimeDomain_2,
    *fftImpulseTimeDomain_3,
    *fftImpulseTimeDomain_4,
    *fftImpulseTimeDomain_Product,
    *fftImpulsefrequencyDomain_1,
    *fftImpulsefrequencyDomain_2,
    *fftImpulsefrequencyDomain_3,
    *fftImpulsefrequencyDomain_4,
    *fftImpulsefrequencyDomain_Product;
    
    fftw_plan fftwImpulseForwardPlan_1,
    fftwImpulseForwardPlan_2,
    fftwImpulseForwardPlan_3,
    fftwImpulseForwardPlan_4,
    fftwImpulseBackwardPlan_;
};
