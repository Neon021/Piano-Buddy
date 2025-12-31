#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "piano_buddy.h"

// Returns a dynamically allocated array of strings (song names)
// count is an output parameter that will hold the number of songs
char** get_library_songs(int* count) {
    *count = 0;
    DIR *d;
    struct dirent *dir;
    
    d = opendir("library");
    if (!d) {
        // If library folder doesn't exist, create it and return empty
        mkdir("library", 0777);
        return NULL;
    }

    // First pass: Count folders
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                (*count)++;
            }
        }
    }
    rewinddir(d);

    if (*count == 0) {
        closedir(d);
        return NULL;
    }

    // Allocate memory for the list
    char** songs = malloc(sizeof(char*) * (*count));
    int i = 0;

    // Second pass: Store names
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                songs[i] = strdup(dir->d_name);
                i++;
            }
        }
    }
    closedir(d);
    return songs;
}

void free_song_list(char** songs, int count) {
    if (!songs) return;
    for (int i = 0; i < count; i++) {
        free(songs[i]);
    }
    free(songs);
}