#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <sndfile.h>

#define SAMPLE_RATE (44100)
#define RECORD_LENGTH (5)
#define CHANNELS (1)
#define FRAMES_PER_BUFFER (512)

int main(){
    printf("Starting audio recording..\r\n");

    //SETUP
    PaStream *stream;
    PaError err;
    float *recordedSamples;
    int numSamples = SAMPLE_RATE * RECORD_LENGTH;
    int numBytes = numSamples * sizeof(float);

    //ALLOCATE MEMORY TO SAVE RECORDED SAMPLES
    recordedSamples = (float *) malloc(numBytes);
    if(recordedSamples == NULL){
        printf("Could not allocate memory for audio\r\n");
        return -1;
    }
    else{
        printf("Successfully allocated memory for audio samples\r\n");
    }

    for (int i = 0; i < numSamples; i++){
        recordedSamples[i] = 0.0f;
    }

    err = Pa_Initialize();
    if(err != paNoError){
        printf("Could not initialize PortAudio!\r\n");
        return -1;
    }
    else{
        printf("Successfully initialized PortAudio!\r\n");
    }

    return 0;
}