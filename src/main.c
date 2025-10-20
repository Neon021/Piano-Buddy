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

    //INITIALIZE PORTAUDIO
    err = Pa_Initialize();
    if(err != paNoError){
        printf("Could not initialize PortAudio!\r\n");
        return -1;
    }
    else{
        printf("Successfully initialized PortAudio!\r\n");
    }

    //OPEN THE AUDIO STREAM
    err = Pa_OpenDefaultStream(&stream, CHANNELS, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, NULL, NULL);
    if(err != paNoError){
        printf("Could not open the audio stream!\r\n");
        PaErrorCode errCode = (PaErrorCode) err;
        printf("Error code: %d", errCode);
        return -1;
    }
    else{
        printf("Successfully opened the audio stream!\r\n");
    }

    //START RECORDING
    err = Pa_StartStream(stream);
    if(err != paNoError){
        printf("Could not start the audio stream!\r\n");
        PaErrorCode errCode = (PaErrorCode) err;
        printf("Error code: %d", errCode);
        return -1;
    }
    else{
        printf("Successfully started the audio stream!\r\n");
    }

    printf("🎤 Now recording for %d seconds... Say something!\r\n", RECORD_LENGTH);
    int totalFrames = RECORD_LENGTH * SAMPLE_RATE;
    int framesRead = 0;
    while(framesRead < totalFrames){
        int framesToRead = (totalFrames - framesRead > FRAMES_PER_BUFFER) ? FRAMES_PER_BUFFER : totalFrames - framesRead;
        Pa_ReadStream(stream, recordedSamples + framesRead, framesToRead);
        framesRead += framesToRead;
        printf("framesToRead: %d and framesRead: %d\r\n", framesToRead, framesRead);
    }
    printf("Recording finished.\n");

    //STOP RECORDING
    err = Pa_StopStream(stream);
    if(err != paNoError){
        printf("Could not stop the audio stream!\r\n");
        PaErrorCode errCode = (PaErrorCode) err;
        printf("Error code: %d", errCode);
        return -1;
    }
    else{
        printf("Successfully stopped the audio stream!\r\n");
    }

    err = Pa_CloseStream(stream);
    if(err != paNoError){
        printf("Could not close the audio stream!\r\n");
        PaErrorCode errCode = (PaErrorCode) err;
        printf("Error code: %d", errCode);
        return -1;
    }
    else{
        printf("Successfully closed the audio stream!\r\n");
    }
    Pa_Terminate();


    //SAVE TO FILE
    printf("💾 Saving recording to 'recording.wav'...\n");
    SF_INFO sfInfo;
    sfInfo.samplerate = SAMPLE_RATE;
    sfInfo.channels = CHANNELS;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT; // WAV format, 32-bit float

    SNDFILE *outFile = sf_open("recording.wav", SFM_WRITE, &sfInfo);
    if (!outFile) {
        printf("Error opening output file: %s\r\n", sf_strerror(NULL));
        free(recordedSamples);
        return 1;
    }

    sf_count_t count = sf_write_float(outFile, recordedSamples, numSamples);
    if(count!=numSamples){
        printf("Error writing to file. Wrote %ld frames, expected %d\n", count, numSamples);
    }
    
    // Close the file
    sf_close(outFile);
    
    // Free the memory we allocated
    free(recordedSamples);

    printf("Done!\n");
    return 0;
}