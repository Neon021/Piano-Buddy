#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include "piano_buddy.h"

// Helper to remove newline characters from user input
void strip_newline(char* str) {
    str[strcspn(str, "\n")] = 0;
}

int main() {
    const char* RECORDING_FILE = "recording.wav";
    const char* SOUNDFONT_FILE = "FluidR3Mono_GM.sf3";
    const char* DOWNLOADED_MIDI = "song.mid";
    
    char manual_query[256]; 
    char* search_term = NULL;
    
    printf("\n🎹 --- Welcome to Piano Buddy --- 🎹\n");

    // 0. Check for SoundFont
    if (access(SOUNDFONT_FILE, F_OK) != 0) {
        fprintf(stderr, "❌ Error: SoundFont file '%s' not found!\n", SOUNDFONT_FILE);
        return 1;
    }

    // 1. Record Audio
    printf("\n[1/5] 🎤 Recording 15 seconds of audio...\n");
    if (record_audio(RECORDING_FILE, 15) != 0) {
        fprintf(stderr, "❌ Error: Recording failed.\n");
        return 1;
    }

    // 2. Identify Song
    printf("\n[2/5] ☁️  Identifying song with ACRCloud...\n");
    char* identified_title = identify_song(RECORDING_FILE);
    
    if (identified_title == NULL) {
        printf("⚠️  Could not identify song automatically.\n");
        // If identification fails, we can either quit or ask the user directly.
        search_term = NULL;
    } else {
        printf("✅ Match Found: \"%s\"\n", identified_title);
        search_term = strdup(identified_title);
    }

    // 3. Find MIDI URL (with Interactive Fallback)
    char* midi_url = NULL;

    while (midi_url == NULL) {
        if (search_term != NULL) {
            printf("\n[3/5] 🐍 Asking Python to find MIDI for: \"%s\"...\n", search_term);
            midi_url = get_midi_url_from_python(search_term);
        }

        if (midi_url == NULL) {
            printf("\n❌ Could not find a MIDI file for \"%s\".\n", search_term ? search_term : "unknown");
            printf("👉 Please enter the song name manually (or type 'q' to quit): ");
            
            if (fgets(manual_query, sizeof(manual_query), stdin) != NULL) {
                strip_newline(manual_query);
                
                if (strcmp(manual_query, "q") == 0 || strcmp(manual_query, "Q") == 0) {
                    printf("Quitting.\n");
                    if (search_term) free(search_term);
                    if (identified_title) free(identified_title);
                    return 1;
                }
                
                if (search_term) 
                    free(search_term);
                search_term = strdup(manual_query);
            }
        }
    }
    
    printf("✅ URL Found: %s\n", midi_url);

    // 4. Download MIDI
    printf("\n[4/5] ⬇️  Downloading MIDI file...\n");
    if (download_file(midi_url, DOWNLOADED_MIDI) != 0) {
        fprintf(stderr, "❌ Error: Download failed.\n");
        free(search_term);
        if (identified_title) free(identified_title);
        free(midi_url);
        return 1;
    }
    printf("✅ Download complete.\n");

    // 5. Play with Piano Muted
    printf("\n[5/5] 🎵 Playing \"%s\" (Piano Track Muted)...\n", search_term);
    printf("    (Press Enter to stop playback)\n");
    
    play_midi_muted(SOUNDFONT_FILE, DOWNLOADED_MIDI);

    // Cleanup
    if (search_term) free(search_term);
    if (identified_title) free(identified_title);
    free(midi_url);
    
    printf("\n👋 Thanks for using Piano Buddy!\n");
    return 0;
}