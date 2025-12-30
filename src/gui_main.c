#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"
#include <pthread.h>
#include <math.h>

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

//Helper function to scale text
void DrawTextCentered(Font font, const char* text, float centerX, float y, float fontSize, Color color) {
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1.0f);
    // If we want the center to be at 'centerX', we start drawing at 'centerX - halfWidth'
    float textStartX = centerX - (textSize.x / 2.0f);

    DrawTextEx(font, text, (Vector2){ textStartX, y }, fontSize, 1.0f, color);
}

int main() {
    // --- INITIALIZATION ---
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Piano Buddy v1.0");
    SetTargetFPS(60);

    Font customFont = LoadFontEx("resources/Roboto-Bold.ttf", 96, 0, 0);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(customFont);

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
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        
        float center = w * 0.5f;

        float scale = w / 800.0f; 
        if (scale < 0.5f) 
            scale = 0.5f;
        // UPDATE LOGIC & DRAWING
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Draw a faint sine wave in the background if Recording or Playing
        if (currentState == STATE_RECORDING || currentState == STATE_PLAYING) {
            for (int i = 0; i < w; i+=5) {
                float time = GetTime();
                float amplitude = (currentState == STATE_RECORDING) ? 50 : 30;
                float frequency = (currentState == STATE_RECORDING) ? 0.05f : 0.02f;
                // Simple sine wave math
                DrawPixel(i, (h/2) + sin(i * frequency + time * 10) * amplitude, LIGHTGRAY);
                DrawPixel(i, (h/2) + sin(i * frequency + time * 10 + 100) * amplitude, Fade(MAROON, 0.3f));
            }
        }

        // Draw Title
        float titleSize = 70.0f * scale; 
        DrawTextCentered(customFont, "Piano Buddy", center, h * 0.1f, titleSize, DARKGRAY);
        DrawLine(w * 0.1f, h * 0.15f, w * 0.9f, h * 0.15f, GRAY);

        // Draw Status Message
        GuiStatusBar((Rectangle){0, h - (30 * scale), w, (30 * scale)}, statusMessage);


        // STATE MACHINE UI
        if (currentState == STATE_IDLE) {
            float btnW = 200 * scale;
            float btnH = 50 * scale;
            if (GuiButton((Rectangle){center - btnW/2, h * 0.4f, btnW, btnH}, 
                          GuiIconText(ICON_PLAYER_RECORD, "Record (15s)"))) {
                currentState = STATE_RECORDING;
                strcpy(statusMessage, "Recording audio...");
            }
            float subSize = 20.0f * scale;
            DrawTextCentered(customFont, "Press to listen", center, h * 0.55f, subSize, GRAY);
        }
        else if (currentState == STATE_RECORDING) {
            float subSize = 35.0f * scale;
            DrawTextCentered(customFont, "Listening...", center, h * 0.4f, subSize, MAROON);
            
            // Draw a pulsing circle
            float pulse = sin(GetTime() * 10) * 10;
            DrawCircleLines(center, h * 0.6f, 40 + pulse, RED);
            
            EndDrawing(); // Force draw

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
            // 1. Draw Error Message
            // Move it down to 30% of screen height (h * 0.3f) so it clears the title
            float errorMsgSize = 30.0f * scale;
            DrawTextCentered(customFont, "Could not identify song.", center, h * 0.3f, errorMsgSize, MAROON);
            
            if (showInputBox) {
                float boxWidth = 300 * scale;
                float boxHeight = 140 * scale;
                
                // Centered X: center - half_width
                // Y Position: 40% down the screen (h * 0.4f)
                Rectangle boxBounds = { center - (boxWidth / 2), h * 0.4f, boxWidth, boxHeight };
                
                GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * scale));

                int result = GuiTextInputBox(boxBounds, 
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
                
                // RUN PLAYBACK THREAD
                audioData.stop_flag = false;
                audioData.is_running = true; //set this to true before creating the thread to avoid race condition
                printf("DEBUG: Running playback thread");
                if (pthread_create(&playbackThread, NULL, audio_thread_func, &audioData) != 0) {
                     currentState = STATE_ERROR;
                     strcpy(statusMessage, "Failed to create audio thread.");
                }
            }
            free(found_url);
            continue;
        }
        else if (currentState == STATE_PLAYING) {
            float subSize = 35.0f * scale;
            float titleSize = 40.0f * scale;
            DrawTextCentered(customFont, "Now Playing:", center, h * 0.3f, subSize, DARKGRAY);
            DrawTextCentered(customFont, songTitle, center, h * 0.4f, titleSize, MAROON);
            // DrawTextCentered(customFont, "(Piano Track Muted)", center, h * 0.5f, 20, GRAY);

            // float time = GetTime();
            // DrawCircle(480, 390, 10 + (sin(time * 5) * 3), MAROON); 
            // DrawText("Playing...", 420, 380, 10, DARKGRAY);

            // Stop Button
            if (GuiButton((Rectangle){center - 100, h * 0.7f, 200, 50}, GuiIconText(ICON_PLAYER_STOP, "Stop"))) {
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
    UnloadFont(customFont);
    CloseWindow();

    return 0;
}