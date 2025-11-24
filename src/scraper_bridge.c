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

    snprintf(command, sizeof(command), "python3 src/scraper.py \"%s\"", query);

    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run Python scraper.\n");
        return NULL;
    }

    // Read the output (the URL)
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Remove the newline character at the end if it exists
        buffer[strcspn(buffer, "\n")] = 0;

        // Check if Python returned an error or not found
        if (strcmp(buffer, "NOT_FOUND") == 0 || strcmp(buffer, "ERROR") == 0) {
            result_url = NULL;
        } else {
            // Duplicate the string to return it
            result_url = strdup(buffer);
        }
    }

    pclose(fp);

    return result_url;
}

// --- Simple Main to Test It ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"Song Name\"\n", argv[0]);
        return 1;
    }

    printf("Calling Python to find: %s...\n", argv[1]);
    char* url = get_midi_url_from_python(argv[1]);

    if (url) {
        printf("Success! URL found: %s\n", url);
        free(url);
    } else {
        printf("Failed to find MIDI URL.\n");
    }

    return 0;
}