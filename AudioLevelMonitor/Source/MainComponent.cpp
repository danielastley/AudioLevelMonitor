#include "MainComponent.h"
#include <JuceHeader.h>

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    
    isInputActive = false;
    

    // --- TEMPORARY TEST ---
       // juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
       //                                   [&] (bool granted) {
       //                                        DBG("Runtime permission callback executing.");
       //                                        if (granted) { DBG("Permission GRANTED."); } else { DBG("Permission DENIED."); }
       //                                        setAudioChannels (granted ? 2 : 0, 2);
       //                                    });
       DBG("!!! WARNING: Bypassing permission request - calling setAudioChannels(2, 2) directly for testing !!!");
       setAudioChannels(2, 2); // Directly request inputs
       // --- END OF TEMPORARY TEST ---
 
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

    // Set starting value for quietest: a very large positive number.
    quietestSoundSoFar = 2147483647;
    
    // Set starting value for loudest: a very large negative number.
    loudestSoundSoFar = -2147483648;
    
    // Reset the block counter
    blocksProcessedCounter = 0;
    
    // Calculate how many blocks make up rouhly one second.
    // We need to handle the case where sampleRate or samplesPerBlockExpected might be zero
    // to avoid divison by zero, although this is unlikely in prepareToPlay.
    if (sampleRate > 0 && samplesPerBlockExpected > 0)
    {
        // Calculate blocks per second. Since sampleRate is double and we want int,
        // we cast the result to int after division.
        outputIntervalInBlocks = static_cast<int>(sampleRate / samplesPerBlockExpected);
    }
    else
    {
        // If settings are weird, default to a reasonable interval (e.g. 100 blocks)
        outputIntervalInBlocks = 100;
    }
    
    // Make sure the interval is at least 1
    if (outputIntervalInBlocks <=0)
    {
        outputIntervalInBlocks = 1;
    }
    // Print a message to the debug console so we know this function ran
    DBG("prepareToPlay called: Initialised quietest/loudest values.");
    DBG("Output interval set to approx every ");
    DBG(outputIntervalInBlocks);
    DBG(" blocks");

}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // We are only listening, so make sure we don't accidentally send out sound.
    // This clears the audio buffer that would normally go to the speakers.
    bufferToFill.buffer->clear();
    
    // Ask JUCE: "Is the microphone (input) is actually connected and active.
    auto* device = deviceManager.getCurrentAudioDevice();
    bool isInputAvailable = device != nullptr && device->getActiveInputChannels().countNumberOfSetBits() > 0;
    if (!isInputAvailable) // The '!' means 'not'
        
    {
        // If NO input is available, stop processing this block right here.
        return; // Exit the function early
    }
    // --- If we got here, input IS available! --
    // How many individual sound samples are in this chunk?
    int numberOfSamplesInThisChunk = bufferToFill.numSamples;
    
    // Get read-only access to the audio samples for channel 0 (Left mic)
    const float* channelData = bufferToFill.buffer->getReadPointer(0);
    
    // Print a message to the console showing how many samples we got.
    // This helps confirm the function is running and getting data.
    DBG("getNextAudioBlock called. Samples in this Chunk: ");
    DBG(numberOfSamplesInThisChunk); // DBG can print numbers directly
    
    // --- Loop through each sample in this chunk ---
    for (int i = 0; i < numberOfSamplesInThisChunk; ++i)
    {
        // We wil put the processing for ONE sample inside these braces.
        // 1. Get the current samples's value (it's a decimal number)
        float decimalSample = channelData[i]; // Use 'i' to get the i-th sample
        
        // 2. Convert the decimal sample [-1.0, +1.0] to a whole number [-32767, +32767]
        int wholeNumberSample = static_cast<int>(decimalSample * 32767.0f);
        
        // --- TEMPORARY DEBUG: Print the first sample of each block ---
        if (i == 0) // Only for the very first sample (index 0) in this block
        {
            DBG("First sample in block (int): "); DBG(wholeNumberSample);
        }
        // --- End of temporary debug ---
        
        // 3. Check if this sample is the new quietest (using only if')
        if (wholeNumberSample < quietestSoundSoFar)
        {
            // If yes, update our remembered quietest value
            quietestSoundSoFar = wholeNumberSample;
        }
        
        // 4. Check if this sample is the new ludest (using only 'if')
        if (wholeNumberSample > loudestSoundSoFar)
        {
            // If yes, update our remembered loudest value
            loudestSoundSoFar = wholeNumberSample;
            
        } // --- End of the loop for this chunk ---
        
    // --- After processing all samples in the block, update the counter ---
        
    // Increment the block counter (using +1 to stick strictly to in/if/else idea)
    blocksProcessedCounter = blocksProcessedCounter + 1;
        
    // Check if enough blocks have passed to print the results
    if (blocksProcessedCounter >= outputIntervalInBlocks)
    {
        // --- Time to print! ---
        
        // Use DBG to print the results to the Xcode console
        DBG("--- Audio Metrics (Approx Every Second) ---");
        DBG("Min Int Sample: "); DBG(quietestSoundSoFar);
        DBG("Max Int Sample: "); DBG(loudestSoundSoFar);
        int range = loudestSoundSoFar - loudestSoundSoFar;
        DBG("Int Range:      "); DBG(range);
        DBG("-------------------------------------------");
        
        // Reset the block counter so we wait another interval
        blocksProcessedCounter = 0;
        
        // OPTIONAL: Reset min/max here if you want metrics *for the last second*
        // instead of *overall* since the start. For overall, leave these commented out
        
        // quietestSoundSoFar = 2147483647;
        // loudestSoundSoFar = -2147483648;
        
        
    } // --- End of the check for printing ---
        
    } // <<< THIS is the closing brace for the 'for' loop. It signifies:
    // "We are done processing sample 'i'. Now go back to the
    // start of the loop and proess sample 'i+1' (if available)
}
    void MainComponent::releaseResources()
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.
        
        // For more details, see the help for AudioProcessor::releaseResources()
    }
    
    //==============================================================================
    void MainComponent::paint (juce::Graphics& g)
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        
        // You can add your drawing code here!
    }
    
    void MainComponent::resized()
    {
        // This is called when the MainContentComponent is resized.
        // If you add any child components, this is where you should
        // update their positions.
    }

