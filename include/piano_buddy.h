#ifndef PIANO_BUDDY_H
#define PIANO_BUDDY_H

// recorder.c
int record_audio(const char *filename, int seconds);

// recognizer.c
char* identify_song(const char *filename);

// scraper_bridge.c
char* get_midi_url_from_python(const char *query);

// downloader.c
int download_file(const char *url, const char *output_filename);

// midi_player.c
int play_midi_muted(const char *soundfont, const char *midi_file);

#endif