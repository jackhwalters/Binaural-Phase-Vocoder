/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
DafxBinauralPhaseVocoderAudioProcessor::DafxBinauralPhaseVocoderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif

// Initialise values that are assigned and used in following functions
{
    fftActualTransformSize_ = 512;
    inputBufferLength_ = 1;
    outputBufferLength_ = 1;
    inputBufferWritePosition_ = outputBufferWritePosition_ = outputBufferReadPosition_ = 0;
    samplesSinceLastFFT_ = 0;
    windowBuffer_ = 0;
    windowBufferLength_ = 0;
    preparedToPlay_ = false;
    fftSignalScaleFactor_ = 0.0;
    phase = 0.0;
    
    Ldelay_ = 0.0;
    Rdelay_ = 0.0;
    leftEarDelayPosition_ = 0.0;
    rightEarDelayPosition_ = 0.0;
    c_ = 343;
    azimuth = 0.0;
    elevation = 0;
    hasRun = false;
    
    reverbParameters.dryLevel = 1.0;
    reverbParameters.wetLevel = 0.0;
    reverbParameters.roomSize = 0.5;
    reverbParameters.damping = 0.0;
    reverb.setParameters(reverbParameters);
    
    passthrough = true;
}

DafxBinauralPhaseVocoderAudioProcessor::~DafxBinauralPhaseVocoderAudioProcessor()
{
}

//==============================================================================
/*
  * @brief Prepare variables that are to be used in audio processing
  * @param System sample rate
  * @param System buffer size
*/
void DafxBinauralPhaseVocoderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Create an instance of the the IRBank class, which loads impulse responses into an array of type AudioBuffer
    irBank.build();
    bufferSize = samplesPerBlock;
    
    if (isPowerOfTwo(bufferSize) == false)
    {
        throw std::invalid_argument("bufferSize must be a power of 2");
    }

    // Prepare the DSP specifications that are used to initialise our instance of the JUCE convolution class
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    // Set sample rate of our instance of the JUCE reveb class, and reset its buffer
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    
    // Set the window buffer length, scale factor (used to scale coefficients after the IFFT) and hopsize
    windowBufferLength_ = fftActualTransformSize_;
    fftSignalScaleFactor_ = 1.0 / fftActualTransformSize_;
    hopActualSize_ = fftActualTransformSize_;
    
    // As we are using a rectangular window, the windowBuffer_ array is filled with window coefficients of 1.0
    windowBuffer_ = (double *)malloc(fftActualTransformSize_ * sizeof(double));
    for (int i = 0; i < fftActualTransformSize_; i++)
    {
        windowBuffer_[i] = 1.0;
    }
    
    // Initialise the FFTW objects and methods used in the phase vocoder inside processBlock and IRCrossfade
    initFFT(fftActualTransformSize_);
    impulseResponseCrossfade.initFFT();
    
    // Initialise an empty delay buffer to hold the most recent 2 seconds worth of samples
    delayBufferLength_ = (int)(2.0*sampleRate);
    if(delayBufferLength_ < 1)    // Sanity check to ensure buffer length is positive and non-zero
        delayBufferLength_ = 1;
    delayBuffer_.setSize(getTotalNumInputChannels(), delayBufferLength_);
    delayBuffer_.clear();
    
    // Ensure that our instance of the JUCE convolution object is reset and prepared
    juceConvolution.reset();
    juceConvolution.prepare(spec);
}

/*
  * @brief Initialise FFTW objects and methods that are to be used in processBlock
  * @param FFT length
*/
void DafxBinauralPhaseVocoderAudioProcessor::initFFT(int FFTLength)
{
    // Utilise FFTW's wrapper function to allocate memory for the complex arrays used to store information in both the time and frequency domain inside processBlock
    fftSignalTimeDomain_ = fftw_alloc_complex(fftActualTransformSize_);
    fftSignalFrequencyDomain_ = fftw_alloc_complex(fftActualTransformSize_);


    // Create 1-dimensional FFT and IFFT plans through FFTW's fftw_plan_dft_1d method
    fftwSignalForwardPlan_ = fftw_plan_dft_1d(FFTLength, fftSignalTimeDomain_,
                                       fftSignalFrequencyDomain_, FFTW_FORWARD, FFTW_ESTIMATE);
    
    fftwSignalBackwardPlan_ = fftw_plan_dft_1d(FFTLength, fftSignalFrequencyDomain_,
                                       fftSignalTimeDomain_, FFTW_BACKWARD, FFTW_ESTIMATE);
    
    // Initialise and resize arrays used to store samples in intermediate stages of the phase vododer
    inputBufferLength_ = FFTLength;
    inputBuffer_.setSize(2, inputBufferLength_);
    inputBuffer_.clear();
    outputBufferLength_ = 2*FFTLength;
    outputBuffer_.setSize(2, outputBufferLength_);
    outputBuffer_.clear();
    
    // Initialise counters and read pointers
    inputBufferWritePosition_ = 0;
    samplesSinceLastFFT_ = 0;
    outputBufferReadPosition_ = 0;
    }

/*
  * @brief Perform audio processing on block of input samples
  * @param Input audio buffer
  * @param MIDI messages
*/
void DafxBinauralPhaseVocoderAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Toggle whether the function is bypassed or not (i.e. whether the incoming audio is processed or passed through)
    if (bypass == false)
    {
        ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        int sampleRate = getSampleRate();
        int channel, inwritepos, sampssincefft, dwp, lrp, rrp;
        int outreadpos, outwritepos;
        
        // In case we have more outputs than inputs, clear any output channels that don't contain input data
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        {
            buffer.clear (i, 0, bufferSize);
        }
        
        // Adjust revert wet level to the mapped distance reading from the GUI: the reverb increases as the distance increases
        reverbParameters.wetLevel = 0.0 + ((0.1 - 0.0) / (20 - 1)) * (distance - 1);
        reverb.setParameters(reverbParameters);
        // Process left and right channels with reverb
        reverb.processStereo (buffer.getWritePointer(0), buffer.getWritePointer(1), bufferSize);

        // Initialise a 'process' buffer, which is used inside the phase vocoder and time delay arcitectures
        AudioSampleBuffer processBuffer;
        processBuffer.setSize(totalNumInputChannels, bufferSize);
        processBuffer = buffer;
        buffer.clear();

        // Initialise an instance of the the impulse state machine, which selects the 4 impulse responses closest to the user's selected azimuth and elevation angles
        ImpulseSelectionStateMachine impulseStateMachine;
        impulseStateMachine.stateMachine (azimuth, elevation);

        //Iterate through the input channels
        for (channel = 0; channel < totalNumInputChannels; ++channel)
        {
            // Attain writable pointer to the the process buffer
            float* processData = processBuffer.getWritePointer(channel);

            // inputBuffer_ is the circular buffer for collecting input samples for the FFT
            float* inputBufferData = inputBuffer_.getWritePointer(jmin (channel, inputBuffer_.getNumChannels() - 1));
            float* outputBufferData = outputBuffer_.getWritePointer(jmin (channel, inputBuffer_.getNumChannels() - 1));

            // State variables temporarily cached for each channel so the two channel can have independent behaviour
            inwritepos = inputBufferWritePosition_;
            outwritepos = outputBufferWritePosition_;
            outreadpos = outputBufferReadPosition_;
            sampssincefft = samplesSinceLastFFT_;

            // Iterate through the samples in the buffer
            for (int i = 0; i < bufferSize; ++i)
            {
                const float in = processData[i];

                // Store the next buffered sample in the output. Do this first before anything changes the output buffer. Set the result to 0 when finished in preparation for the next overlap/add procedure
                processData[i] = outputBufferData[outreadpos];
                outputBufferData[outreadpos] = 0.0;
                if(++outreadpos >= outputBufferLength_)
                    outreadpos = 0;

                // Store current sample in input buffer
                inputBufferData[inwritepos] = in;
                // Increment the write pointer
                if (++inwritepos >= inputBufferLength_)
                {
                    inwritepos = 0;
                }
                // Also increment how many samples we've stored since last transform; if it reaches the hop size, perform an FFT, and subsequent convolution
                if (++sampssincefft >= hopActualSize_)
                {
                    sampssincefft = 0;

                    // Where N >= buffer, this will be inwritepos. Modulo precaution is for larger buffers
                    int inputBufferStartPosition = (inwritepos + inputBufferLength_
                                                    - fftActualTransformSize_) % inputBufferLength_;

                    int inputBufferIndex = inputBufferStartPosition;
                    for (int fftBufferIndex = 0; fftBufferIndex < fftActualTransformSize_; fftBufferIndex++)
                    {
                        // Set real part to windowed signal, imaginary part to 0
                        fftSignalTimeDomain_[fftBufferIndex][1] = 0.0;

                        // Safety check for if the window isn't ready
                        if (fftBufferIndex >= windowBufferLength_)
                        {
                            fftSignalTimeDomain_[fftBufferIndex][0] = 0.0;
                        }
                        // Fill real dimension of fftSignalTimeDomain_ array with the input data (stored in the inputBufferData vector) multiplied by 1.0 (the rectangular window)
                        else {
                            fftSignalTimeDomain_[fftBufferIndex][0] = windowBuffer_[fftBufferIndex] * inputBufferData[inputBufferIndex];
                        }

                        inputBufferIndex++;
                        if (inputBufferIndex >= inputBufferLength_)
                        {
                            inputBufferIndex = 0;
                        }
                    }

                    // Perform FFT on windowed data, inputting the signal in fftSignalTimeDomain_ and outputting fftFrequencyDomain_
                    fftw_execute(fftwSignalForwardPlan_);

                    // Phase vocoder
                    // As the FFT of a real signal is always conjugate symmetric, we only need to iterate through half the FFT size
                    for (int i = 0; i < fftActualTransformSize_/2; i++)
                    {
                        // Recover amplitude information, multiplied by the inverse of the user-specified sound source distance
                        float amplitude = sqrt(
                                               (fftSignalFrequencyDomain_[i][0] *
                                                fftSignalFrequencyDomain_[i][0]) +
                                               (fftSignalFrequencyDomain_[i][1] *
                                                 fftSignalFrequencyDomain_[i][1])) *
                                                1/distance;

                        // If user has selected pass-through, recover original phase information
                        if (passthrough)
                        {
                            phase = atan2(fftSignalFrequencyDomain_[i][1], fftSignalFrequencyDomain_[i][0]);
                        }
                        // If user has selected the robotisation effect, set phase value to 0.0
                        else if (robotisation)
                        {
                            phase = 0.0;
                        }
                        // If user has selected the whisperisation effect, replace the phase value with a random number between 0-2π
                        else if (whisperisation)
                        {
                            phase = 2.0 * M_PI * (float)rand() / (float)RAND_MAX;
                        }

                        // F(k)
                        fftSignalFrequencyDomain_[i][0] = amplitude * cos(phase);
                        fftSignalFrequencyDomain_[i][1] = amplitude * sin(phase);

                        // F(N-k)
                        if (i != 0)
                        {
                            fftSignalFrequencyDomain_[fftActualTransformSize_ - i][0] = amplitude * cos(phase);
                            fftSignalFrequencyDomain_[fftActualTransformSize_ - i][1] = -amplitude * sin(phase);
                        }
                    }

                    //Perform IFFT on augmented amplitude and phase information inside fftSignalFrequencyDomain_, outputting fftSignalTimeDomain_
                    fftw_execute(fftwSignalBackwardPlan_);

                    // Add the result to the output buffer, starting at the outputBufferIndex which is assigned according to the write position in outputBufferData
                    int outputBufferIndex = outwritepos;
                    for (int fftBufferIndex = 0; fftBufferIndex < fftActualTransformSize_; fftBufferIndex++)
                    {
                        // Inside outputBufferData accumulate each sample in the real dimension of fftSignalTimeDomain_, multiplied by the scale factor (in our case 1/K)
                        outputBufferData[outputBufferIndex] += fftSignalTimeDomain_[fftBufferIndex][0] * fftSignalScaleFactor_;
                        if (++outputBufferIndex >= outputBufferLength_)
                        {
                            outputBufferIndex = 0;
                        }
                    }
                    // Advance the write position in the output buffer by the hopsize
                    outwritepos = (outwritepos + hopActualSize_) % outputBufferLength_;
                }
            }

            // Update previously cached state variables
            inputBufferWritePosition_ = inwritepos;
            outputBufferWritePosition_ = outwritepos;
            outputBufferReadPosition_ = outreadpos;
            samplesSinceLastFFT_ = sampssincefft;
            
            // Load impulse response numbers stored a global variables LL, UL, LR and UR inside the ImpulseSelectionStateMachine class as parameters into the impulseResponseCrossfade object of type IRCrossfade
            impulseResponseCrossfade.loadImpulses(channel, irBank.bufferArray[impulseStateMachine.LL], irBank.bufferArray[impulseStateMachine.UL], irBank.bufferArray[impulseStateMachine.LR], irBank.bufferArray[impulseStateMachine.UR]);
            //Perform FFT, complex multiplication and IFFTs on the 4 impulse responses
            impulseResponseCrossfade.impulseFFTBlend();
            impulseResponseCrossfade.backwardFFTandStore(channel, totalNumInputChannels);

        }

        //Interaural delay
        // Attain writable pointers for each channel to the buffer, processBuffer and delayBuffer_
        float* LchannelData = buffer.getWritePointer(0);
        float* RchannelData = buffer.getWritePointer(1);
        float* LprocessData = processBuffer.getWritePointer(0);
        float* RprocessData = processBuffer.getWritePointer(1);
        float* LdelayData = delayBuffer_.getWritePointer(0);
        float* RdelayData = delayBuffer_.getWritePointer(1);

        // Depedent on which quadrant the virtual sound source is in, either of the listener's ears may be the 'leading ear', so azimuth-dependent delays are calculated accordingly for each situation
        // Sound source is in quadrant 1
        if (azimuth > 0 && azimuth < M_PI/2)
        {
            Ldelay_ = 0.07*(abs(rad2deg(azimuth)) + 1 - M_PI/2)/c_;
            Rdelay_ = 0.07*(1-cos(rad2deg(azimuth)))/c_;
        }
        // Sound source is in quadrant 2
        else if (azimuth > -M_PI/2 && azimuth < 0)
        {
            Ldelay_ = 0.07*(1-cos(rad2deg(azimuth)))/c_;
            Rdelay_ = 0.07*(abs(rad2deg(azimuth)) + 1 - M_PI/2)/c_;
        }
        // Sound source is in quadrant 3
        else if (azimuth < -M_PI/2 && azimuth > -M_PI)
        {
            Ldelay_ = 0.07*(1-cos(rad2deg(azimuth)))/c_;
            Rdelay_ = 0.07*(abs(rad2deg(azimuth + M_PI)) + 1 - M_PI/2)/c_;
        }
        // Sound source is in quadrant 4
        else if (azimuth > M_PI/2 && azimuth < M_PI)
        {
            Ldelay_ = 0.07*(abs(rad2deg(azimuth - M_PI)) + 1 - M_PI/2)/c_;
            Rdelay_ = 0.07*(1-cos(rad2deg(azimuth)))/c_;
        }
        // Sound source is directly in front or behind
        else if (azimuth == 0.0 || azimuth == -M_PI)
        {
            Ldelay_ = 0.0;
            Rdelay_ = 0.0;
        }
        // Sanity check incase GUI sends unstable values
        else if (azimuth < -M_PI || azimuth > M_PI)
        {
            azimuth = 0.0;
        }

        // Each ear's read position in its respective delay buffer is calculated dependent on the delay times calculated above
        leftEarDelayPosition_ = (int)(delayWritePosition_ - ((Ldelay_*0.01) * sampleRate) + delayBufferLength_) % delayBufferLength_;
        rightEarDelayPosition_ = (int)(delayWritePosition_ - ((Rdelay_*0.01) * sampleRate) + delayBufferLength_) % delayBufferLength_;

        // Similar to the phase vocoder arcitecture, state counters are cached so each channel can have independent behaviour
        dwp = delayWritePosition_;
        lrp = leftEarDelayPosition_;
        rrp = rightEarDelayPosition_;

        // Iterate through the buffer, storing samples from processBuffer into the delay buffers. The read each delay buffer at their own read pointer into the respective channels of the main buffer
        for (int i = 0; i < bufferSize; i++)
        {
            LdelayData[dwp] = LprocessData[i];
            RdelayData[dwp] = RprocessData[i];

            LchannelData[i] = LdelayData[lrp] * 2.0 * audioGain;
            RchannelData[i] = RdelayData[rrp] * 2.0 * audioGain;

            // Ensure pointers stay in range
            if (++lrp >= delayBufferLength_)
                lrp = 0;
            if (++rrp >= delayBufferLength_)
                rrp = 0;
            if (++dwp >= delayBufferLength_)
                dwp = 0;
        }
        // Update previously cached pointer state variables
        delayWritePosition_ = dwp;
        leftEarDelayPosition_ = lrp;
        rightEarDelayPosition_ = rrp;
        
        BinaryData::IR_wavSize;
    
        
        //Convolution
        // Load novel spectrally combined impulse response into juceConvolution object
        juceConvolution.copyAndLoadImpulseResponseFromBuffer(impulseResponseCrossfade.crossfadedImpulse, sampleRate, true, false, true, 0);

        // DSP audio block replaces buffer
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        
        //Process
        juceConvolution.process(context);
    }
}

/*
  * @brief Free up memory upon termination of audio processing
*/
void DafxBinauralPhaseVocoderAudioProcessor::releaseResources()
{
    fftw_destroy_plan(fftwSignalForwardPlan_);
    fftw_destroy_plan(fftwSignalBackwardPlan_);
    fftw_free(fftSignalTimeDomain_);
    fftw_free(fftSignalFrequencyDomain_);
    
    free(windowBuffer_);
    windowBuffer_ = 0;
    windowBufferLength_ = 0;
}

//==============================================================================
const String DafxBinauralPhaseVocoderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DafxBinauralPhaseVocoderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DafxBinauralPhaseVocoderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DafxBinauralPhaseVocoderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DafxBinauralPhaseVocoderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DafxBinauralPhaseVocoderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DafxBinauralPhaseVocoderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DafxBinauralPhaseVocoderAudioProcessor::setCurrentProgram (int index)
{
}

const String DafxBinauralPhaseVocoderAudioProcessor::getProgramName (int index)
{
    return {};
}

void DafxBinauralPhaseVocoderAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DafxBinauralPhaseVocoderAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DafxBinauralPhaseVocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
bool DafxBinauralPhaseVocoderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DafxBinauralPhaseVocoderAudioProcessor::createEditor()
{
    return new DafxBinauralPhaseVocoderAudioProcessorEditor (*this);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DafxBinauralPhaseVocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DafxBinauralPhaseVocoderAudioProcessor();
}
