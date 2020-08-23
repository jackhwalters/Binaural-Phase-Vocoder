/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#include "IRCrossfade.h"

IRCrossfade::IRCrossfade()
{
    fftImpulseScaleFactor_ = 0.0;
    fftImpulseActualTransformSize_ = 0;
    initFFT();
}

/*
  * @brief Initialise FFTW objects and methods that are to be used in loadImpulses, impulseFFTBlend and backwardFFTandStore
*/
void IRCrossfade::initFFT()
{
    //Ensure FFT size is a power of 2, and set FFT coefficient scaling value (1/K)
    fftImpulseActualTransformSize_ = nextPowerOf2(HRIR_SIZE);
    fftImpulseScaleFactor_ = 1.0/fftImpulseActualTransformSize_;
    
    // Utilise FFTW's wrapper function to allocate memory for the complex arrays used to store information in both the time and frequency domain
    fftImpulseTimeDomain_1 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulseTimeDomain_2 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulseTimeDomain_3 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulseTimeDomain_4 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulsefrequencyDomain_1 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulsefrequencyDomain_2 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulsefrequencyDomain_3 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulsefrequencyDomain_4 = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulseTimeDomain_Product = fftw_alloc_complex(fftImpulseActualTransformSize_);
    fftImpulsefrequencyDomain_Product = fftw_alloc_complex(fftImpulseActualTransformSize_);
    
    // Create 1-dimensional FFT and IFFT plans through FFTW's fftw_plan_dft_1d method
    fftwImpulseForwardPlan_1 = fftw_plan_dft_1d(fftImpulseActualTransformSize_, fftImpulseTimeDomain_1,fftImpulsefrequencyDomain_1, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwImpulseForwardPlan_2 = fftw_plan_dft_1d(fftImpulseActualTransformSize_, fftImpulseTimeDomain_2,fftImpulsefrequencyDomain_2, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwImpulseForwardPlan_3 = fftw_plan_dft_1d(fftImpulseActualTransformSize_, fftImpulseTimeDomain_3,fftImpulsefrequencyDomain_3, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwImpulseForwardPlan_4 = fftw_plan_dft_1d(fftImpulseActualTransformSize_, fftImpulseTimeDomain_4,fftImpulsefrequencyDomain_4, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwImpulseBackwardPlan_ = fftw_plan_dft_1d(fftImpulseActualTransformSize_, fftImpulsefrequencyDomain_Product, fftImpulseTimeDomain_Product, FFTW_BACKWARD, FFTW_ESTIMATE);
}

/*
  * @brief Load impulse responses into arrays of type fftw_complex and perform a forward-FFT on each
  * @param Channel number (whichever is presently being interated through inside processBlock in the DafxBinauralPhaseVocoderAudioProcessor class)
  * @param Nearest HRIR to the lower left of the user's azimuth/elevation choice
  * @param Nearest HRIR to the upper left of the user's azimuth/elevation choice
  * @param Nearest HRIR to the lower right of the user's azimuth/elevation choice
  * @param Nearest HRIR to the upper right of the user's azimuth/elevation choice
*/
void IRCrossfade::loadImpulses(int channel, AudioSampleBuffer impulse1, AudioSampleBuffer impulse2, AudioSampleBuffer impulse3, AudioSampleBuffer impulse4)
{
    // Get write pointers to the 4 HRIRs passed into this method as parameters
    float* startIR1 = impulse1.getWritePointer(channel);
    float* startIR2 = impulse2.getWritePointer(channel);
    float* startIR3 = impulse3.getWritePointer(channel);
    float* startIR4 = impulse4.getWritePointer(channel);
    
    // All IRs are the sample length
    int LengthIR = impulse1.getNumSamples();
    
    // Copy the AudioSampleBuffer buffers into vectors of type float
    std::vector<float> impulse1vec (startIR1, startIR1 + LengthIR);
    std::vector<float> impulse2Vec (startIR2, startIR2 + LengthIR);
    std::vector<float> impulse3Vec (startIR3, startIR3 + LengthIR);
    std::vector<float> impulse4Vec (startIR4, startIR4 + LengthIR);

    // Load these vectors into the real dimension of their corresponding fftImpulseTimeDomain_1 arrays, and 0 the imaginary dimension
    for (int i = 0; i < fftImpulseActualTransformSize_; i++)
    {
        fftImpulseTimeDomain_1[i][0] = impulse1vec[i];
        fftImpulseTimeDomain_1[i][1] = 0.0;
        fftImpulseTimeDomain_2[i][0] = impulse2Vec[i];
        fftImpulseTimeDomain_2[i][1] = 0.0;
        fftImpulseTimeDomain_3[i][0] = impulse3Vec[i];
        fftImpulseTimeDomain_3[i][1] = 0.0;
        fftImpulseTimeDomain_4[i][0] = impulse4Vec[i];
        fftImpulseTimeDomain_4[i][1] = 0.0;
    }
    
    // Execute forward FFTs on these 4 fftw_complex arrays
    fftw_execute(fftwImpulseForwardPlan_1);
    fftw_execute(fftwImpulseForwardPlan_2);
    fftw_execute(fftwImpulseForwardPlan_3);
    fftw_execute(fftwImpulseForwardPlan_4);
}

/*
  * @brief Perform 4-way complex multiplication on the spectral data of the loaded impulse responses
*/
void IRCrossfade::impulseFFTBlend()
{
    // (a + bi)(c + di)(e + fi)(g + hi) = (eacg-ebdg-adfg-bcfg-eadh-ebch-acfh+bdfh) + i(each+eadg+ebcg-ebdh+acfg-adfh-bcfh-bdfg)
    // Only iterate through half the transform size as the FFT of a real signal is always conjugate symmetric
    for (int i = 0; i < fftImpulseActualTransformSize_/2; i++)
    {
        //Real
        fftImpulsefrequencyDomain_Product[i][0] =
        
        (( (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_4[i][0])
            
          - (fftImpulsefrequencyDomain_3[i][0] *  fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_4[i][0])
          
          - (fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][0])
          
          - (fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_3[i][i] * fftImpulsefrequencyDomain_4[i][0])
          
          - (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          - (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_4[i][1])
          
          - (fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          - (fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          * fftImpulseScaleFactor_));
        
        
        //Imaginary
        fftImpulsefrequencyDomain_Product[i][1] =
        
        (( (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_4[i][1])
          
          + (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_4[i][0])
          
          + (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_4[i][0])
          
          - (fftImpulsefrequencyDomain_3[i][0] * fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          + (fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][0])
          
          - (fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          - (fftImpulsefrequencyDomain_1[i][1] * fftImpulsefrequencyDomain_2[i][0] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][1])
          
          - (fftImpulsefrequencyDomain_1[i][0] * fftImpulsefrequencyDomain_2[i][1] * fftImpulsefrequencyDomain_3[i][1] * fftImpulsefrequencyDomain_4[i][0])
          
           * fftImpulseScaleFactor_));
        
        //Maintain conjugate symmetry
        if (i != 0)
        {
            fftImpulsefrequencyDomain_Product[fftImpulseActualTransformSize_ - i][0] = fftImpulsefrequencyDomain_Product[i][0];
            fftImpulsefrequencyDomain_Product[fftImpulseActualTransformSize_ - i][1] = fftImpulsefrequencyDomain_Product[i][1];
        }
    }
}

/*
  * @brief Perform backwards FFT on frequency domain projduct calculated in impulseFFTBlend and store in AudioSampleBuffer crossfadedImpulse
  * @param Channel number (whichever is presently being interated through inside processBlock in the DafxBinauralPhaseVocoderAudioProcessor class)
  * @param Number of input channels
*/
void IRCrossfade::backwardFFTandStore(int channel, int numberOfInputChannels)
{
    fftw_execute(fftwImpulseBackwardPlan_);
    
    crossfadedImpulse.setSize(numberOfInputChannels, HRIR_SIZE);
    float* crossfadedImpulseData = crossfadedImpulse.getWritePointer(channel);
    
    // Iterate through FFT size, setting each sample of the crossfadedImpulse buffer to its corresponding sample in the fftw_complex object fftImpulseTimeDomain_Product
    for (int i = 0; i < fftImpulseActualTransformSize_; i++)
    {
        crossfadedImpulseData[i] = fftImpulseTimeDomain_Product[i][0];
    }
}

/*
  * @brief Free up memory upon termination of audio processing
*/
void IRCrossfade::deinitFFT()
{
    fftw_destroy_plan(fftwImpulseForwardPlan_1);
    fftw_destroy_plan(fftwImpulseForwardPlan_2);
    fftw_destroy_plan(fftwImpulseForwardPlan_3);
    fftw_destroy_plan(fftwImpulseForwardPlan_4);
    fftw_destroy_plan(fftwImpulseBackwardPlan_);
    
    fftw_free(fftImpulseTimeDomain_1);
    fftw_free(fftImpulseTimeDomain_2);
    fftw_free(fftImpulseTimeDomain_3);
    fftw_free(fftImpulseTimeDomain_4);
    fftw_free(fftImpulseTimeDomain_Product);
    
    fftw_free(fftImpulsefrequencyDomain_1);
    fftw_free(fftImpulsefrequencyDomain_2);
    fftw_free(fftImpulsefrequencyDomain_3);
    fftw_free(fftImpulsefrequencyDomain_4);
    fftw_free(fftImpulsefrequencyDomain_Product);
}

IRCrossfade::~IRCrossfade()
{
    deinitFFT();
}
