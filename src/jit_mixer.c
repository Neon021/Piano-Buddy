#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "piano_buddy.h"

// Mixes the specific stems into 'current_session_mix.mp3'
// Returns 0 on success
int create_temp_mix(const char* song_folder_name, bool vocals, bool drums, bool bass, bool other) {
    char command[1024];
    char inputs[512] = "";
    int input_count = 0;

    printf("DEBUG: Mixing stems for [%s]...\n", song_folder_name);

    // Build the input list dynamically
    if (vocals) {
        char path[256];
        snprintf(path, sizeof(path), "library/%s/vocals.wav", song_folder_name);
        if (access(path, F_OK) == 0) {
            strcat(inputs, "-i \"");
            strcat(inputs, path);
            strcat(inputs, "\" ");
            input_count++;
        }
    }
    if (drums) {
        char path[256];
        snprintf(path, sizeof(path), "library/%s/drums.wav", song_folder_name);
        if (access(path, F_OK) == 0) {
            strcat(inputs, "-i \"");
            strcat(inputs, path);
            strcat(inputs, "\" ");
            input_count++;
        }
    }
    if (bass) {
        char path[256];
        snprintf(path, sizeof(path), "library/%s/bass.wav", song_folder_name);
        if (access(path, F_OK) == 0) {
            strcat(inputs, "-i \"");
            strcat(inputs, path);
            strcat(inputs, "\" ");
            input_count++;
        }
    }
    if (other) {
        char path[256];
        snprintf(path, sizeof(path), "library/%s/other.wav", song_folder_name);
        if (access(path, F_OK) == 0) {
            strcat(inputs, "-i \"");
            strcat(inputs, path);
            strcat(inputs, "\" ");
            input_count++;
        }
    }

    if (input_count == 0) {
        fprintf(stderr, "ERROR: No stems selected or files missing!\n");
        return 1;
    }

    // ffmpeg -y -i vocals.wav -i drums.wav ... -filter_complex amix=inputs=N:duration=longest current_session_mix.mp3
    snprintf(command, sizeof(command), 
             "ffmpeg -y %s -filter_complex amix=inputs=%d:duration=longest current_session_mix.mp3 > /dev/null 2>&1", 
             inputs, input_count);

    // Execute
    int status = system(command);
    
    if (status == 0) {
        printf("DEBUG: Mix created successfully.\n");
        return 0;
    } else {
        fprintf(stderr, "ERROR: FFmpeg mix failed.\n");
        return 1;
    }
}