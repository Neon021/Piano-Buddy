#ifndef PIANO_BUDDY_H
#define PIANO_BUDDY_H

// recorder.c
int record_audio(const char *filename, int seconds);

// recognizer.c
char* identify_song(const char *filename);

// processor_bridge.c
int process_song_with_python(const char* song_name);

#endif