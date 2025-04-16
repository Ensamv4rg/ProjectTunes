#include <iostream>
#include <vector>
#include <cstring>

#include <portaudio.h>
//Compiling for VS-CODE- https://files.portaudio.com/docs/v19-doxydocs/compile_windows.html

#include <cstdint>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Structure to hold audio data and playback state.
struct AudioData {
    const uint8_t* data;  // pointer to the audio data buffer
    size_t totalBytes;    // total size of the audio data in bytes
    size_t position;      // current playback position in bytes
    int bytesPerFrame;    // number of bytes in one frame (sample) for all channels
};

// This callback is called by PortAudio when it needs more audio data.
static int audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void* userData) 
{
    AudioData* audio = reinterpret_cast<AudioData*>(userData);
    // outputBuffer should be filled with raw audio bytes.
    uint8_t* out = reinterpret_cast<uint8_t*>(outputBuffer);

    // Calculate how many bytes to provide.
    size_t requestedBytes = framesPerBuffer * audio->bytesPerFrame;
    size_t remainingBytes = audio->totalBytes - audio->position;
    size_t bytesToCopy = (requestedBytes < remainingBytes) ? requestedBytes : remainingBytes;

    // Copy audio data to the output.
    memcpy(out, audio->data + audio->position, bytesToCopy);

    // If we reached the end, fill the rest with zeros.
    if (bytesToCopy < requestedBytes) {
        memset(out + bytesToCopy, 0, requestedBytes - bytesToCopy);
        audio->position += bytesToCopy;
        return paComplete;
    }
    
    audio->position += bytesToCopy;
    return paContinue;
}

// Function to play audio using PortAudio.
// Parameters:
//   rawData       : pointer to the raw audio data (interleaved if stereo)
//   totalBytes    : total number of audio data bytes in rawData
//   sampleRate    : sample rate in Hz (e.g., 44100)
//   numChannels   : number of channels (1 for mono, 2 for stereo, etc.)
//   bitsPerSample : audio sample bit depth (e.g., 16, 32, etc.)



void playAudio(const uint8_t* rawData, size_t totalBytes, double sampleRate, int numChannels, int bitsPerSample) 
{
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "PortAudio error (Initialize): " << Pa_GetErrorText(err) << endl;
        return;
    }

    // Calculate bytes per frame.
    int bytesPerSample = bitsPerSample / 8;
    int bytesPerFrame = bytesPerSample * numChannels;
    
    // Set up our data structure.
    AudioData audio;
    audio.data = rawData;
    audio.totalBytes = totalBytes;
    audio.position = 0;
    audio.bytesPerFrame = bytesPerFrame;

    // Determine the PortAudio sample format.
    PaSampleFormat sampleFormat;
    if (bitsPerSample == 8) {
        // Typically, 8-bit PCM is unsigned.
        sampleFormat = paUInt8;
    } else if (bitsPerSample == 16) {
        sampleFormat = paInt16;
    } else if (bitsPerSample == 32) {
        // For 32-bit PCM, you might be using either int32 or float32.
        // Here we assume 32-bit integer.
        sampleFormat = paInt32;
    } else {
        cerr << "Unsupported bits per sample: " << bitsPerSample << endl;
        Pa_Terminate();
        return;
    }

    // Open the default audio stream.
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               0,               // no input channels
                               numChannels,     // output channels
                               sampleFormat,    // sample format
                               sampleRate,
                               paFramesPerBufferUnspecified,
                               audioCallback,
                               &audio);
    if (err != paNoError) {
        cerr << "PortAudio error (OpenDefaultStream): " << Pa_GetErrorText(err) << endl;
        Pa_Terminate();
        return;
    }

    // Start the stream.
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error (StartStream): " << Pa_GetErrorText(err) << endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }

    // Wait until playback is complete.
    while ((err = Pa_IsStreamActive(stream)) == 1) {
        Pa_Sleep(100);
    }
    if (err < 0) {
        cerr << "PortAudio error (IsStreamActive): " << Pa_GetErrorText(err) << endl;
    }

    // Stop the stream.
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error (StopStream): " << Pa_GetErrorText(err) << endl;
    }

    // Clean up.
    Pa_CloseStream(stream);
    Pa_Terminate();
}

int main() {
    cout<<"Program Started"<<endl;
    cout << "Current Working Directory: " << fs::current_path() <<endl;
    // Open the WAV file.

    ifstream file("practice1.wav", ios::binary);

    if (!file) {
        cerr << "Error opening file!" << endl;
        cerr << "Error details: " << strerror(errno) << endl;
        return 1;
    }
    // Read entire file into a vector.
    vector<uint8_t> data((istreambuf_iterator<char>(file)),
                              istreambuf_iterator<char>());
    file.close();

    cout<<"Done reading file"<<endl;
    // Extract header information (as in your previous code).
    uint32_t size = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
    uint16_t format_tag = data[20] | (data[21] << 8);
    uint16_t channel_size = data[22] | (data[23] << 8);
    uint32_t sample_rate = data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24);
    uint32_t byte_rate = data[28] | (data[29] << 8) | (data[30] << 16) | (data[31] << 24);
    uint16_t sample_size = data[32] | (data[33] << 8);
    uint16_t bits_per_sample = data[34] | (data[35] << 8);
    
    // Find the "data" chunk.
    size_t data_starts = 0;
    for (size_t i = 0; i < data.size() - 3; i++) {
        if (data[i] == 'd' && data[i+1] == 'a' && data[i+2] == 't' && data[i+3] == 'a') {
            data_starts = i;
            break;
        }
    }
    if (data_starts == 0) {
        cerr << "data chunk not found." << endl;
        return 1;
    }
    
    // Data chunk size.
    uint32_t data_size = data[data_starts + 4] |
                         (data[data_starts + 5] << 8) |
                         (data[data_starts + 6] << 16) |
                         (data[data_starts + 7] << 24);


    // Debug: Print header values.
    cout << "File Size: " << static_cast<double>(size)/1048576 << " MB\n";
    cout << "Format Tag: " << format_tag << "\n";
    cout << channel_size << " channels\n";
    cout << "Sample Rate: " << sample_rate << " Hz\n";
    cout << "Byte Rate: " << byte_rate << "\n";
    cout << "Sample Size: " << sample_size << "\n";
    cout << "Bits per Sample: " << bits_per_sample << "\n";
    cout << "Data Size: " << data_size << " bytes\n";

    // Extract the audio data from the data chunk.
    const uint8_t* audioDataPtr = data.data() + data_starts + 8;
    size_t audioDataBytes = data_size;

    // Call the playAudio function.
    playAudio(audioDataPtr, audioDataBytes, sample_rate, channel_size, bits_per_sample);

    return 0;
}
