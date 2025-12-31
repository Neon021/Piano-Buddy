#include <stdbool.h>

#ifndef PIANO_BUDDY_H
#define PIANO_BUDDY_H

// recorder
int record_audio(const char *filename, int seconds);

// recognizer
char* identify_song(const char *filename);

// processor_bridge
extern int processingProgress; // Share this variable
int process_song_with_python(const char* song_name, char* output_folder_buffer);

// library_manager
char** get_library_songs(int* count);
void free_song_list(char** songs, int count);

// jit_mixer
int create_temp_mix(const char* song_folder_name, bool vocals, bool drums, bool bass, bool other);

#endif