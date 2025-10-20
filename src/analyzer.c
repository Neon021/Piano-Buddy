#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include <fftw3.h>

#define BUFFER_LEN (1024)

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("Usage: %s <input.wav>\r\n", argv[0]);
        return -1;
    }

    //OPEN SOUND FILE
    char *file_name = argv[1];
    SF_INFO sf_info;
    SNDFILE *in_file = sf_open(file_name, SFM_READ, &sf_info);

    if(!in_file){
        printf("ERROR: Could not open file %s\r\n", file_name);
        printf("lidsndfile error: %s\r\n", sf_strerror(NULL));
        return -1;
    }

    if(sf_info.channels > 1){
        printf("ERROR: Sound file has more than 1 channel. Please select a mono file!\r\n");
        sf_close(in_file);
        return -1;
    }

    printf("File opened: %s\r\n", file_name);
    printf("Sample Rate: %d\r\n", sf_info.samplerate);
    printf("Frames (samples): %ld\r\n", sf_info.frames);

    //READ ALL AUDIO DATA INTO MEMORY
    double *audio_data = (double *) malloc(sizeof(double) * sf_info.frames);
    if(!audio_data){
        printf("ERROR: Could not allocate memory for audio!\r\n");
        sf_close(in_file);
        return -1;
    }

    sf_count_t frames_read = sf_readf_double(in_file, audio_data, sf_info.frames);
    if(frames_read != sf_info.frames){
        printf("ERROR: Frames read is not equal to total frame in the sample file!\r\n");
        free(audio_data);
        sf_close(in_file);
        return -1;
    }

    sf_close(in_file);

    //PREPARE AND EXECUTE FFTW
    // An FFT of N real samples gives N/2 + 1 complex output bins
    int num_frames = (int)sf_info.frames;
    int num_output_bins = (num_frames / 2) + 1;
    fftw_complex *fft_out;
    fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * num_output_bins);

    // Create a "plan". This is FFTW's way of pre-calculating the
    // fastest possible FFT for a given data size.
    fftw_plan plan;
    plan = (fftw_plan)fftw_plan_dft_r2c_1d(num_frames, audio_data, fft_out, FFTW_ESTIMATE);

    printf("Executing FFTW plan\r\n");
    fftw_execute(plan);

    //ANALYZE FFT OUTPUT
    printf("Finding dominant frequency\r\n");
    double max_magnitude = -1.0;
    int max_index = -1;
    for(int i = 0; i <num_output_bins; i++){
        // The output is in complex form (real and imaginary parts)
        double real_part = fft_out[i][0];
        double imag_part = fft_out[i][1];

        // We need the magnitude: sqrt(real^2 + imag^2)
        double magnitude = sqrt(real_part * real_part + imag_part * imag_part);

        // Find the bin with the highest magnitude
        if(magnitude > max_magnitude){
            printf("Current max_magnitude: %f\n", magnitude);
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    //CONVERT BIN INDEX TO HERTZ
    double dom_frequency = (double)max_index * sf_info.samplerate / num_frames;
    printf("\n--- FFT Analysis Complete ---\n");
    printf("Strongest frequency bin: %d\n", max_index);
    printf("Magnitude: %f\n", max_magnitude);
    printf("Dominant Frequency: %.2f Hz\n", dom_frequency);

    fftw_destroy_plan(plan);
    fftw_free(fft_out);
    free(audio_data);

    return 0;
}