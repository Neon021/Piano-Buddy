#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "piano_buddy.h"

int processingProgress = 0;

// Returns 0 on success, 1 on failure.
// Writes the resulting folder name into 'output_folder_buffer'
int process_song_with_python(const char* song_name, char* output_folder_buffer) {
    char command[512];
    char buffer[1024];
    int success = 0;
    
    processingProgress = 0;

    printf("DEBUG: C requesting processing for: [%s]\n", song_name);

    // Use the venv python
    snprintf(command, sizeof(command), 
             "./piano_buddy_venv/bin/python3 src/processor.py \"%s\" 2>&1", 
             song_name);

    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Failed to run Python processor.\n");
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; // Trim newline
        printf("PYTHON: %s\n", buffer);

        if (strncmp(buffer, "PROGRESS:", 9) == 0) {
            processingProgress = atoi(buffer + 9);
        }
        else if (strncmp(buffer, "OUTPUT:", 7) == 0) {
            if (output_folder_buffer) {
                strcpy(output_folder_buffer, buffer + 7);
            }
            success = 1;
        }
    }

    int status = pclose(fp);
    
    if (success || status == 0) {
        return 0; 
    } else {
        return 1;
    }
}