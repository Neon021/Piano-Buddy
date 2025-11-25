#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"
#if defined(__linux__)
    #include <pthread.h>
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "piano_buddy.h"


typedef enum {
    STATE_IDLE,
    STATE_RECORDING,
    STATE_IDENTIFYING,
    STATE_SEARCHING,
    STATE_DOWNLOADING,
    STATE_PLAYING,
    STATE_ERROR
} AppState;

int main() {
    // --- INITIALIZATION ---
    const int screenWidth = 500;
    const int screenHeight = 400;

    InitWindow(screenWidth, screenHeight, "Piano Buddy v1.0");
    SetTargetFPS(60);

    // App Variables
    AppState currentState = STATE_IDLE;
    char statusMessage[256] = "Ready to start!";
    char songTitle[256] = "Unknown Song";

    const char* RECORDING_FILE = "recording.wav";
    const char* SOUNDFONT_FILE = "FluidR3Mono_GM.sf3";
    const char* DOWNLOADED_MIDI = "song.mid";

    char* found_title = NULL;
    char* found_url = NULL;

    // Check for SoundFont
    if (access(SOUNDFONT_FILE, F_OK) != 0) {
        currentState = STATE_ERROR;
        strcpy(statusMessage, "Error: SoundFont file not found!");
    }

    // APP LOOP
    while (!WindowShouldClose()) {
        
        // UPDATE LOGIC & DRAWING
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Draw Title
        DrawText("Piano Buddy", 150, 20, 40, DARKGRAY);
        DrawLine(20, 70, 480, 70, GRAY);

        // Draw Status Message
        GuiStatusBar((Rectangle){0, screenHeight - 30, screenWidth, 30}, statusMessage);


        // STATE MACHINE UI
        if (currentState == STATE_IDLE) {
            if (GuiButton((Rectangle){150, 150, 200, 50}, GuiIconText(ICON_PLAYER_RECORD, "Record (15s)"))) {
                currentState = STATE_RECORDING;
                strcpy(statusMessage, "Recording audio...");
            }
            DrawText("Press the button to listen.", 140, 220, 20, GRAY);
        }
        else if (currentState == STATE_RECORDING) {
            DrawText("Listening...", 190, 150, 30, RED);
            
            // FORCE DRAW to screen before blocking
            EndDrawing(); 

            if (record_audio(RECORDING_FILE, 15) != 0) {
                currentState = STATE_ERROR;
                strcpy(statusMessage, "Recording Failed.");
            } else {
                currentState = STATE_IDENTIFYING;
                strcpy(statusMessage, "Identifying song...");
            }
            
            continue; // Skip the standard EndDrawing since we forced it
        }
        else if (currentState == STATE_IDENTIFYING) {
            DrawText("Identifying...", 180, 150, 30, BLUE);
            EndDrawing();

            found_title = identify_song(RECORDING_FILE);
            if (found_title == NULL) {
                currentState = STATE_ERROR;
                strcpy(statusMessage, "Could not identify song.");
            } else {
                snprintf(songTitle, sizeof(songTitle), "%s", found_title);
                currentState = STATE_SEARCHING;
                strcpy(statusMessage, "Searching for MIDI...");
                free(found_title); // copied itnto songTitle free this
            }
            continue;
        }
        else if (currentState == STATE_SEARCHING) {
            DrawText("Searching Web...", 160, 150, 30, ORANGE);
            EndDrawing();

            found_url = get_midi_url_from_python(songTitle);
            if (found_url == NULL) {
                //TODO: Add a text box for user to manually type the song name
                currentState = STATE_ERROR;
                strcpy(statusMessage, "MIDI not found online.");
            } else {
                currentState = STATE_DOWNLOADING;
                strcpy(statusMessage, "Downloading MIDI...");
            }
            continue;
        }
        else if (currentState == STATE_DOWNLOADING) {
            DrawText("Downloading...", 160, 150, 30, ORANGE);
            EndDrawing();

            if (download_file(found_url, DOWNLOADED_MIDI) != 0) {
                currentState = STATE_ERROR;
                strcpy(statusMessage, "Download Failed.");
            } else {
                currentState = STATE_PLAYING;
                strcpy(statusMessage, "Playing...");
            }
            free(found_url);
            continue;
        }
        else if (currentState == STATE_PLAYING) {
            DrawText("Now Playing:", 50, 120, 20, DARKGRAY);
            DrawText(songTitle, 50, 150, 30, MAROON);
            
            DrawText("(Piano Track Muted)", 150, 200, 20, GRAY);

            if (GuiButton((Rectangle){150, 300, 200, 50}, GuiIconText(ICON_PLAYER_STOP, "Stop Playback"))) {
                currentState = STATE_IDLE;
                strcpy(statusMessage, "Ready.");
            }
            
            EndDrawing();
            //TODO: Implement threading for midi_player to eliminate blocking here
            play_midi_muted(SOUNDFONT_FILE, DOWNLOADED_MIDI);

            currentState = STATE_IDLE;
            strcpy(statusMessage, "Playback finished.");
            continue;
        }
        else if (currentState == STATE_ERROR) {
            DrawText("Error Occurred", 160, 120, 30, RED);
            DrawText(statusMessage, 50, 180, 20, BLACK);
            
            if (GuiButton((Rectangle){150, 250, 200, 40}, "Try Again")) {
                currentState = STATE_IDLE;
                strcpy(statusMessage, "Ready.");
            }
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}