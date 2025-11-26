#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"
#include <pthread.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "piano_buddy.h"

// APP STATE
typedef enum {
    STATE_IDLE,
    STATE_RECORDING,
    STATE_IDENTIFYING,
    STATE_MANUAL_INPUT,
    STATE_SEARCHING,
    STATE_DOWNLOADING,
    STATE_PLAYING,
    STATE_ERROR
} AppState;

// THREADING DATA
typedef struct {
    char soundfont[256];
    char midi_file[256];
    volatile bool stop_flag;
    bool is_running;
} AudioThreadData;

//Global instance for v1
AudioThreadData audioData;

// The function that runs in the background thread
void* audio_thread_func(void* arg) {
    printf("DEBUG: audio_thread_func function start");
    AudioThreadData* data = (AudioThreadData*)arg;
    data->is_running = true;
    
    printf("DEBUG: audio_thread_func play_midi_muted function started");
    play_midi_muted(data->soundfont, data->midi_file, &data->stop_flag);
    
    printf("DEBUG: audio_thread_func function ended");
    data->is_running = false;
    return NULL;
}

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

    char manualInputBuffer[256] = {0};
    bool showInputBox = false;

    const char* RECORDING_FILE = "recording.wav";
    const char* SOUNDFONT_FILE = "FluidR3Mono_GM.sf3";
    const char* DOWNLOADED_MIDI = "song.mid";

    char* found_title = NULL;
    char* found_url = NULL;
    pthread_t playbackThread;

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
                // FALLBACK
                currentState = STATE_MANUAL_INPUT;
                strcpy(statusMessage, "Identification failed. Enter name manually.");
                showInputBox = true;
                memset(manualInputBuffer, 0, 256);
            } else {
                snprintf(songTitle, sizeof(songTitle), "%s", found_title);
                free(found_title);
                currentState = STATE_SEARCHING;
                strcpy(statusMessage, "Searching for MIDI...");
            }
            continue;
        }
        else if (currentState == STATE_MANUAL_INPUT) {
            DrawText("Could not identify song.", 140, 120, 20, MAROON);
            
            if (showInputBox) {
                int result = GuiTextInputBox((Rectangle){100, 160, 300, 120}, 
                                             "Enter Song Name", 
                                             "Type the song name to search:", 
                                             "Search", 
                                             manualInputBuffer, 
                                             256, 
                                             NULL);
                
                if (result == 1) {
                    snprintf(songTitle, sizeof(songTitle), "%s", manualInputBuffer);
                    showInputBox = false;
                    currentState = STATE_SEARCHING;
                    strcpy(statusMessage, "Searching manual entry...");
                }
            }
        }
        else if (currentState == STATE_SEARCHING) {
            DrawText("Searching Web...", 160, 150, 30, ORANGE);
            EndDrawing();

            found_url = get_midi_url_from_python(songTitle);
            if (found_url == NULL) {
                currentState = STATE_MANUAL_INPUT;
                strcpy(statusMessage, "MIDI not found. Try a different name.");
                showInputBox = true;
            } else {
                currentState = STATE_DOWNLOADING;
                strcpy(statusMessage, "Downloading MIDI...");
            }
            continue;
        }
        else if (currentState == STATE_DOWNLOADING) {
            DrawText("Downloading...", 160, 150, 30, ORANGE);
            EndDrawing();

            printf("DEBUG: Downloading Midi file");
            if (download_file(found_url, DOWNLOADED_MIDI) != 0) {
                currentState = STATE_ERROR;
                strcpy(statusMessage, "Download Failed.");
            } 
            else {
                currentState = STATE_PLAYING;
                strcpy(statusMessage, "Starting Playback...");
                
                // CONFIGURE PLAYBACK THREAD VARS
                printf("DEBUG: Configuring playback thread");
                strcpy(audioData.soundfont, SOUNDFONT_FILE);
                strcpy(audioData.midi_file, DOWNLOADED_MIDI);
                audioData.stop_flag = false;
                
                // CREATE PLAYBACK THREAD
                printf("DEBUG: Creating playback thread");
                if (pthread_create(&playbackThread, NULL, audio_thread_func, &audioData) != 0) {
                     currentState = STATE_ERROR;
                     strcpy(statusMessage, "Failed to create audio thread.");
                }
            }
            free(found_url);
            continue;
        }
        else if (currentState == STATE_PLAYING) {
            DrawText("Now Playing:", 50, 120, 20, DARKGRAY);
            DrawText(songTitle, 50, 150, 30, MAROON);
            // DrawText("(Piano Track Muted)", 150, 200, 20, GRAY);
            
            float time = GetTime();
            DrawCircle(480, 390, 10 + (sin(time * 5) * 3), MAROON); 
            DrawText("Playing...", 420, 380, 10, DARKGRAY);

            if (GuiButton((Rectangle){150, 300, 200, 50}, GuiIconText(ICON_PLAYER_STOP, "Stop Playback"))) {
                audioData.stop_flag = true; // Tell thread to stop
                currentState = STATE_IDLE;
                strcpy(statusMessage, "Stopping...");
            }
            
            // Cleanup thread if not already
            if (!audioData.is_running) {
                pthread_join(playbackThread, NULL);
                currentState = STATE_IDLE;
                strcpy(statusMessage, "Ready.");
            }
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

    if (audioData.is_running) {
        audioData.stop_flag = true;
        pthread_join(playbackThread, NULL);
    }
    CloseWindow();

    return 0;
}