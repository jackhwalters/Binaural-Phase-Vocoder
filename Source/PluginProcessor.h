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
#include "IRCrossfade.h"
#include <cmath>
#include "StateMachine.h"

//==============================================================================
/**
*/
class DafxBinauralPhaseVocoderAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    DafxBinauralPhaseVocoderAudioProcessor();
    ~DafxBinauralPhaseVocoderAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //dsp::Convolution
    dsp::Convolution juceConvolution;
    IRBank irBank;
    IRCrossfade impulseResponseCrossfade;
    AudioSampleBuffer crossfadedImpulse;
    
    //Gain
    void setGain(float gainSend);
    
    //FFT
    void initFFT(int FFTLength);
    float phase;
    
    //Buttons
    bool bypass;
    bool passthrough;
    bool robotisation;
    bool whisperisation;
    
    //General global variables
    int numberofChannels;
    int sampleRate;
    int bufferSize;
    float audioGain;
    
    //Source positioning
    double azimuth;
    int elevation;
    float distance;
    ImpulseSelectionStateMachine impulseSelectionStateMachine;
    bool hasRun;
    
    Reverb reverb;
    Reverb::Parameters reverbParameters;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DafxBinauralPhaseVocoderAudioProcessor)

    //FFTW
    fftw_complex *fftSignalTimeDomain_,
    *fftSignalFrequencyDomain_;
    fftw_plan fftwSignalForwardPlan_,
    fftwSignalBackwardPlan_;

    // Overlap-add architecture
    int fftActualTransformSize_;
    int hopActualSize_;
    double fftSignalScaleFactor_;
    AudioBuffer<float> inputBuffer_;
    int inputBufferLength_;
    int inputBufferWritePosition_;

    bool fftInitialised_;
    bool preparedToPlay_;
    AudioBuffer<float> outputBuffer_;
    int outputBufferLength_;
    int outputBufferReadPosition_, outputBufferWritePosition_;

    int samplesSinceLastFFT_;
    double *windowBuffer_;
    int windowBufferLength_;
    int hopSelectedSize_;
    int windowType_;
    
    //ITD
    AudioSampleBuffer delayBuffer_;
    int delayBufferLength_;
    int leftEarDelayPosition_;
    int rightEarDelayPosition_;
    int delayWritePosition_;
    
    float Ldelay_;
    float Rdelay_;
    float c_;
    
//    AudioFormatReaderSource* source = nullptr;
};
