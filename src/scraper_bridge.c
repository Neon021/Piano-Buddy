#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Calls the Python scraper script to find a MIDI URL.
 * * @param query The song title to search for.
 * @return A malloc'd string containing the URL, or NULL if failed.
 */
char* get_midi_url_from_python(const char* query) {
    char command[512];
    char buffer[1024];
    char* result_url = NULL;
    FILE *fp;

    printf("DEBUG: C is requesting MIDI for: [%s]\n", query);

    // Add "2>&1" at the end. This is a shell trick that sends 
    // Error messages (stderr) to the Output stream (stdout) so C can read them.
    snprintf(command, sizeof(command), "python3 src/scraper.py \"%s\" 2>&1", query);
    
    printf("DEBUG: Running command: %s\n", command);

    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: popen() failed entirely.\n");
        return NULL;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Strip newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        printf("DEBUG: Python script output: [%s]\n", buffer);

        // Check for specific failure keywords
        if (strcmp(buffer, "NOT_FOUND") == 0 || strcmp(buffer, "ERROR") == 0) {
            printf("DEBUG: Python reported failure.\n");
            result_url = NULL;
        } 
        // Check if the output looks like a python error (e.g., Traceback)
        else if (strstr(buffer, "Traceback") != NULL || strstr(buffer, "Error") != NULL) {
             printf("DEBUG: It looks like Python crashed!\n");
             result_url = NULL;
        }
        else {
            // Success!
            result_url = strdup(buffer);
        }
    } 
    else {
        printf("DEBUG: Python script produced NO output at all.\n");
    }

    pclose(fp);
    return result_url;
}
