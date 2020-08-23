/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#include "IRBank.h"

IRBank::IRBank()
{

}

/*
  * @brief Iterates through all binary resources and builds AudioSampleBuffers for each
*/
void IRBank::build()
{
    // Iterate through the list of binary resources (i.e. the HRIRs)
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const char* binaryData = 0;
        int binaryDataSize = 0;
        
        // Extract HRIR from binary resource list
        binaryDataSize = HRIR_SIZE_FILE_SIZE;
        binaryData = BinaryData::getNamedResource(BinaryData::namedResourceList[i], binaryDataSize);
        
        // Create a memory stream for that HRIR
        auto* inputStream = new MemoryInputStream (binaryData, binaryDataSize, false);
        
        // Create WAV format reader for this stream
        WavAudioFormat format;
        reader = format.createReaderFor (inputStream, true);  // takes ownership

        // If reader was successfully created, read stream into corresponding index of AudioSampleBuffer array
        if (reader)
        {
            int streamNumChannels = reader->numChannels;
            int streamNumSamples = (int)reader->lengthInSamples;
            
        bufferArray[i] = juce::AudioBuffer<float>(streamNumChannels, streamNumSamples);
        reader->read(&bufferArray[i], 0, streamNumSamples, 0, true, true);
        }
    }
}

IRBank::~IRBank()
{

}
