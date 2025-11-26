#include <stdio.h>
#include <fluidsynth.h>
#include <stdbool.h>

int play_midi_muted(const char *soundfont, const char *midi_file, volatile bool *should_stop) {
    int res = 0;
    
    fluid_settings_t* settings = new_fluid_settings();

    // Default is 0.2. increased it to 2.0 to gain volume
    fluid_settings_setnum(settings, "synth.gain", 1.0);

    // Increasing the period size gives the CPU more time to process audio.
    // Default period-size is 64.We'll increase it to 1024. This adds a tiny bit of latency (milliseconds), but fixes the crackling.
    fluid_settings_setint(settings, "audio.period-size", 1024);
    fluid_settings_setint(settings, "audio.periods", 2);
    fluid_settings_setnum(settings, "synth.sample-rate", 44100);

    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");

    fluid_synth_t* synth = new_fluid_synth(settings);
    
    if (synth == NULL) {
        fprintf(stderr, "Error: Failed to create synthesizer.\n");
        delete_fluid_settings(settings);
        return 1;
    }

    fluid_audio_driver_t* adriver = new_fluid_audio_driver(settings, synth);
    
    if (adriver == NULL) {
        fprintf(stderr, "Error: Failed to create audio driver.\n");
        delete_fluid_synth(synth);
        delete_fluid_settings(settings);
        return 1;
    }

    // LOAD THE SOUNDFONT
    if (fluid_synth_sfload(synth, soundfont, 1) == FLUID_FAILED) {
        printf("Error: Failed to load the SoundFont file: %s\n", soundfont);
        res = 1;
        goto cleanup;
    }
    printf("🎹 SoundFont loaded successfully.\n");

    // CREATE THE MIDI PLAYER
    fluid_player_t* player = new_fluid_player(synth);
    
    if (fluid_player_add(player, midi_file) == FLUID_FAILED) {
        printf("Error: Failed to add MIDI file: %s\n", midi_file);
        res = 1;
        delete_fluid_player(player);
        goto cleanup;
    }
     printf("🎵 MIDI file added to playlist.\n");

    printf("Playing... Press Enter to stop.\n");
    fluid_player_play(player);
    
    while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
        // Check if the GUI has requested a stop
        if (should_stop != NULL && *should_stop) {
            printf("⏹️ Stop requested by user.\n");
            break; 
        }
    }

    fluid_player_stop(player);
    fluid_player_join(player); // Wait for playback to fully finish

cleanup:
    printf("Cleaning up resources.\n");
    delete_fluid_audio_driver(adriver);
    delete_fluid_player(player);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    return res;
}