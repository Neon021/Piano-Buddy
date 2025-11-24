#include <stdio.h>
#include <fluidsynth.h>

int play_midi_muted(const char *soundfont, const char *midi_file) {
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_player_t* player;
    fluid_audio_driver_t* adriver;

    settings = new_fluid_settings();
    // Use the default audio driver
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    synth = new_fluid_synth(settings);
    adriver = new_fluid_audio_driver(settings, synth);


    if (fluid_synth_sfload(synth, soundfont, 1) == FLUID_FAILED) {
        printf("Error: Failed to load the SoundFont file: %s\n", soundfont);
        goto cleanup;
    }
    printf("🎹 SoundFont loaded successfully.\n");


    player = new_fluid_player(synth);
    
    // Add the MIDI file to the player's playlist
    if (fluid_player_add(player, midi_file) == FLUID_FAILED) {
        printf("Error: Failed to add MIDI file: %s\n", midi_file);
        goto cleanup;
    }
    printf("🎵 MIDI file added to playlist.\n");

    printf("Playing... Press Enter to stop.\n");
    fluid_player_play(player);
    
    getchar();

    printf("Stopping playback.\n");
    fluid_player_stop(player);
    fluid_player_join(player); // Wait for playback to fully finish

cleanup:
    printf("Cleaning up resources.\n");
    delete_fluid_audio_driver(adriver);
    delete_fluid_player(player);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);

    return 0;
}