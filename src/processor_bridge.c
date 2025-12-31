#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "piano_buddy.h"

// Calls the Python script to Download -> Separate -> Mix
// Returns 0 on success, 1 on failure
int process_song_with_python(const char* song_name) {
    char command[512];
    char buffer[1024];
    int success = 0;

    printf("DEBUG: C requesting processing for: [%s]\n", song_name);

    snprintf(command, sizeof(command), 
             "./piano_buddy_venv/bin/python3 src/processor.py \"%s\" 2>&1", 
             song_name);

    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Failed to run Python processor.\n");
        return 1;
    }

    // Read output line by line to check for success
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        printf("PYTHON: %s\n", buffer); // Forward Python logs to C terminal

        // Check for our specific success signal
        if (strstr(buffer, "OUTPUT:backing_track.mp3") != NULL) {
            success = 1;
        }
    }

    int status = pclose(fp);
    
    if (success && status == 0) {
        return 0; // Success
    } else {
        return 1; // Failed
    }
}