#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "piano_buddy.h"

int main() {
    const char* RECORDING_FILE = "recording.wav";
    const char* SOUNDFONT_FILE = "FluidR3Mono_GM.sf3"; // Ensure this matches file name!
    const char* DOWNLOADED_MIDI = "song.mid";
    
    printf("\n🎹 --- Welcome to Piano Buddy --- 🎹\n");

    // 0. Check for SoundFont
    if (access(SOUNDFONT_FILE, F_OK) != 0) {
        fprintf(stderr, "❌ Error: SoundFont file '%s' not found!\n", SOUNDFONT_FILE);
        fprintf(stderr, "   Please download it and place it in the project root.\n");
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
    char* song_title = identify_song(RECORDING_FILE);
    
    if (song_title == NULL) {
        fprintf(stderr, "❌ Error: Could not identify the song.\n");
        return 1;
    }
    printf("✅ Match Found: \"%s\"\n", song_title);

    // 3. Find MIDI URL (Using Python)
    printf("\n[3/5] 🐍 Asking Python to find a MIDI file for \"%s\"...\n", song_title);
    char* midi_url = get_midi_url_from_python(song_title);
    
    if (midi_url == NULL) {
        fprintf(stderr, "❌ Error: Python scraper could not find a MIDI file.\n");
        free(song_title);
        return 1;
    }
    printf("✅ URL Found: %s\n", midi_url);

    // 4. Download MIDI
    printf("\n[4/5] ⬇️  Downloading MIDI file...\n");
    if (download_file(midi_url, DOWNLOADED_MIDI) != 0) {
        fprintf(stderr, "❌ Error: Download failed.\n");
        free(song_title);
        free(midi_url);
        return 1;
    }
    printf("✅ Download complete.\n");

    // 5. Play with Piano Muted
    printf("\n[5/5] 🎵 Playing \"%s\" (Piano Track Muted)...\n", song_title);
    printf("    (Press Enter to stop playback)\n");
    
    play_midi_muted(SOUNDFONT_FILE, DOWNLOADED_MIDI);

    // Cleanup
    free(song_title);
    free(midi_url);
    
    printf("\n👋 Thanks for using Piano Buddy!\n");
    return 0;
}