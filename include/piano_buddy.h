#ifndef PIANO_BUDDY_H
#define PIANO_BUDDY_H

// recorder
int record_audio(const char *filename, int seconds);

// recognizer
char* identify_song(const char *filename);

// processor_bridge
int process_song_with_python(const char* song_name);

// library_manager
char** get_library_songs(int* count);
void free_song_list(char** songs, int count);

#endif