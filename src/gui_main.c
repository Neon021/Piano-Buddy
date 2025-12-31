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
    STATE_PROCESSING,
    STATE_PLAYING,
    STATE_ERROR
} AppState;

// THREADING DATA
typedef struct {
    char songname[256];
    bool success;
    bool is_done;
} ProcessorThreadData;

//Global instance for v1
ProcessorThreadData procData;
char** librarySongs = NULL;
int libraryCount = 0;
int selectedSongIndex = -1;
int scrollIndex = 0;

typedef struct {
    Music vocals;
    Music drums;
    Music bass;
    Music other;
    
    float volVocals;
    float volDrums;
    float volBass;
    float volOther;
    
    bool isLoaded;
} StemMixer;

StemMixer globalMixer = {0};
// The function that runs in the background thread
void* proc_thread_func(void* arg) {
    printf("DEBUG: proc_thread_func function start");
    ProcessorThreadData* data = (ProcessorThreadData*)arg;
    
    int result = process_song_with_python(data->songname);
    
    printf("DEBUG: proc_thread_func function ended");
    data->is_done = true;
    data->success = result == 0;
    return NULL;
}
void RefreshLibrary() {
    if (librarySongs) 
        free_song_list(librarySongs, libraryCount);
    librarySongs = get_library_songs(&libraryCount);
}

//Helper function to scale text
void DrawTextCentered(Font font, const char* text, float centerX, float y, float fontSize, Color color) {
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1.0f);
    // If we want the center to be at 'centerX', we start drawing at 'centerX - halfWidth'
    float textStartX = centerX - (textSize.x / 2.0f);

    DrawTextEx(font, text, (Vector2){ textStartX, y }, fontSize, 1.0f, color);
}

int LoadStemMixer(const char* songFolder) {
    if (globalMixer.isLoaded) {
        UnloadMusicStream(globalMixer.vocals);
        UnloadMusicStream(globalMixer.drums);
        UnloadMusicStream(globalMixer.bass);
        UnloadMusicStream(globalMixer.other);
    }

    char path[512];
    
    // Note: In a real app, we might need a "silence.wav" fallback, 
    snprintf(path, sizeof(path), "library/%s/vocals.wav", songFolder);
    globalMixer.vocals = LoadMusicStream(path);
    
    snprintf(path, sizeof(path), "library/%s/drums.wav", songFolder);
    globalMixer.drums = LoadMusicStream(path);
    
    snprintf(path, sizeof(path), "library/%s/bass.wav", songFolder);
    globalMixer.bass = LoadMusicStream(path);
    
    snprintf(path, sizeof(path), "library/%s/other.wav", songFolder);
    globalMixer.other = LoadMusicStream(path);

    // Default Volumes
    globalMixer.volVocals = 1.0f;
    globalMixer.volDrums = 1.0f;
    globalMixer.volBass = 1.0f;
    globalMixer.volOther = 1.0f;

    // Start all
    PlayMusicStream(globalMixer.vocals);
    PlayMusicStream(globalMixer.drums);
    PlayMusicStream(globalMixer.bass);
    PlayMusicStream(globalMixer.other);

    globalMixer.isLoaded = true;

    return 1;
}

void UpdateStemMixer() {
    if (!globalMixer.isLoaded) return;
    
    UpdateMusicStream(globalMixer.vocals);
    UpdateMusicStream(globalMixer.drums);
    UpdateMusicStream(globalMixer.bass);
    UpdateMusicStream(globalMixer.other);
    
    SetMusicVolume(globalMixer.vocals, globalMixer.volVocals);
    SetMusicVolume(globalMixer.drums, globalMixer.volDrums);
    SetMusicVolume(globalMixer.bass, globalMixer.volBass);
    SetMusicVolume(globalMixer.other, globalMixer.volOther);
}

void UnloadStemMixer(){
    if (!globalMixer.isLoaded) return;

    UnloadMusicStream(globalMixer.vocals);
    UnloadMusicStream(globalMixer.drums);
    UnloadMusicStream(globalMixer.bass);
    UnloadMusicStream(globalMixer.other);
}

void StopStemMixer() {
    if (!globalMixer.isLoaded) return;
    StopMusicStream(globalMixer.vocals);
    StopMusicStream(globalMixer.drums);
    StopMusicStream(globalMixer.bass);
    StopMusicStream(globalMixer.other);
}

int main() {
    // --- INITIALIZATION ---
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Piano Buddy v1.0");
    SetTargetFPS(60);
    RefreshLibrary();

    InitAudioDevice();

    Font customFont = LoadFontEx("resources/Roboto-Bold.ttf", 96, 0, 0);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(customFont);

    // App Variables
    AppState currentState = STATE_IDLE;
    char statusMessage[256] = "Ready to start!";
    char songTitle[256] = "Unknown Song";

    char manualInputBuffer[256] = {0};
    bool showInputBox = false;
    bool isManualModeSelected = false;
    pthread_t playback_thread;
    // APP LOOP
    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        float center = w * 0.5f;

        float scale = w / 800.0f; 
        if (scale < 0.5f) 
            scale = 0.5f;
        GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * scale));

        if (currentState == STATE_PLAYING && globalMixer.isLoaded){
            UpdateStemMixer();
        }

        // UPDATE LOGIC & DRAWING
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Draw Title
        DrawTextCentered(customFont, "Piano Buddy", center, h * 0.1f, 70.0f * scale, DARKGRAY);
        DrawLine(w * 0.1f, h * 0.30f, w * 0.9f, h * 0.30f, GRAY);

        // Draw Status Message
        GuiStatusBar((Rectangle){0, h - (30 * scale), w, (30 * scale)}, statusMessage);

        float subSize = 35.0f * scale;
        float titleSize = 40.0f * scale;
        // STATE MACHINE UI
        if (currentState == STATE_IDLE) {
            float btnW = 200 * scale;
            float btnH = 60 * scale;
            float gap = 20 * scale;
            if (GuiButton((Rectangle){(w * 0.25f) - btnW/2, h * 0.4f, btnW, btnH}, 
                          GuiIconText(ICON_PLAYER_RECORD, "Record New"))) {
                currentState = STATE_RECORDING;
                strcpy(statusMessage, "Recording audio...");
            }

            if (GuiButton((Rectangle){(w * 0.25f) - btnW/2, h * 0.4f + btnH + gap, btnW, btnH}, 
                          GuiIconText(ICON_HAND_POINTER, "Manual Search"))) {
                currentState = STATE_MANUAL_INPUT;
                showInputBox = true;
                isManualModeSelected = true;
                memset(manualInputBuffer, 0, 256);
                strcpy(statusMessage, "Enter song name manually.");
            }

            //Library List
            float listX = w * 0.6f;
            float listY = h * 0.2f;
            float listW = w * 0.35f;
            float listH = h * 0.6f;

            DrawText("Your Library:", listX, listY - 30*scale, 20*scale, DARKGRAY);
            DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);

            // Draw List Items
            float itemHeight = 40 * scale;
            GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(16 * scale));
            
            for (int i = 0; i < libraryCount; i++) {
                Rectangle itemRect = { listX + 5, listY + 5 + (i * itemHeight), listW - 10, itemHeight - 5 };
                
                // If clicked, load this song directly
                if (GuiButton(itemRect, librarySongs[i])) {
                    snprintf(songTitle, sizeof(songTitle), "%s", librarySongs[i]);
                    
                    strcpy(statusMessage, "Mixing audio stems...");
                    // Force a draw so user sees the message
                    BeginDrawing(); 
                        ClearBackground(RAYWHITE);
                        DrawTextCentered(customFont, "Mixing...", w/2, h/2, 40*scale, DARKGRAY);
                    EndDrawing();

                    // 1. Generate the Mix on the fly
                    if (LoadStemMixer(songTitle)) {
                        currentState = STATE_PLAYING;
                        strcpy(statusMessage, "Playing custom mix.");
                    } else {
                        strcpy(statusMessage, "Error: Could not mix stems.");
                    }
                }
            }
        }
        else if (currentState == STATE_RECORDING) {
            DrawTextCentered(customFont, "Listening...", center, h * 0.4f, subSize, MAROON);
            
            // Draw a pulsing circle
            float pulse = sin(GetTime() * 10) * 10;
            DrawCircleLines(center, h * 0.6f, 40 + pulse, RED);
            
            EndDrawing(); // Force draw

            if (record_audio("recording.wav", 15) != 0) {
                currentState = STATE_ERROR;
                strcpy(statusMessage, "Recording Failed.");
            } else {
                currentState = STATE_IDENTIFYING;
                strcpy(statusMessage, "Identifying song...");
            }
            
            continue; // Skip the standard EndDrawing since we forced it
        }
        else if (currentState == STATE_IDENTIFYING) {
            DrawTextCentered(customFont, "Identifying...",  center, h * 0.3f, titleSize, BLUE);
            EndDrawing();

            char* result = identify_song("recording.wav");
            if (result == NULL) {
                // FALLBACK
                currentState = STATE_MANUAL_INPUT;
                strcpy(statusMessage, "Identification failed. Enter name manually.");
                showInputBox = true;
                memset(manualInputBuffer, 0, 256);
            } else {
                snprintf(songTitle, sizeof(songTitle), "%s", result);
                free(result);
                currentState = STATE_PROCESSING;
                strcpy(statusMessage, "AI Processing (This takes 1-2 mins)...");

                strcpy(procData.songname, songTitle);
                procData.is_done = false;
                pthread_create(&playback_thread, NULL, proc_thread_func, &procData);
            }
            continue;
        }
        else if (currentState == STATE_MANUAL_INPUT) {
            if(isManualModeSelected){
                DrawTextCentered(customFont, "Could not identify song.", center, h * 0.3f, titleSize, MAROON);
                isManualModeSelected = false;
            }
            
            float boxWidth = 300 * scale;
             Rectangle boxBounds = { center - (boxWidth / 2), h * 0.4f, boxWidth, 160 * scale };
             GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(14 * scale));
             if (showInputBox) {
                 if (GuiTextInputBox(boxBounds, "Enter Song", "Title:", "Go", manualInputBuffer, 256, NULL) == 1) {
                     snprintf(songTitle, sizeof(songTitle), "%s", manualInputBuffer);
                     currentState = STATE_PROCESSING;
                     strcpy(statusMessage, "AI Processing...");
                     strcpy(procData.songname, songTitle);
                     procData.is_done = false;
                     pthread_create(&playback_thread, NULL, proc_thread_func, &procData);
                 }
             }
        }
        else if (currentState == STATE_PROCESSING) {
            DrawTextCentered(customFont, "Generating Backing Track...", center, h * 0.35f, 30 * scale, DARKGRAY);
            DrawTextCentered(customFont, "Please Wait", center, h * 0.45f, 25 * scale, GRAY);
             
            // Simple Spinner Animation
            float time = GetTime();
            DrawCircleLines(center, h * 0.6f, (40 * scale), Fade(BLUE, 0.5f));
            DrawCircle(center + cos(time*5)*30*scale, h * 0.6f + sin(time*5)*30*scale, 10*scale, BLUE);

            // Check Thread
                if (procData.is_done) {
                    pthread_join(playback_thread, NULL);
                    if (procData.success) {
                        LoadStemMixer(songTitle);                     
                        
                        currentState = STATE_PLAYING;
                        strcpy(statusMessage, "Playing Backing Track!");
                     
                        RefreshLibrary();
                 }
            }
        }
        else if (currentState == STATE_PLAYING) {
            DrawTextCentered(customFont, "Now Playing:", center, h * 0.3f, 20 * scale, DARKGRAY);
            DrawTextCentered(customFont, songTitle, center, h * 0.4f, 40 * scale, MAROON);
            
            float time = GetTime();
            DrawCircle((int)(w - 50*scale), (int)(h - 60*scale), (10 + sin(time*5)*3)*scale, MAROON);

            // STEM TOGGLES
            float sliderX = center - (180 * scale);
            float startY = h * 0.5f;
            float gap = 30 * scale;
            
            GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(16 * scale));

            GuiSlider((Rectangle){sliderX, startY, 200*scale, 20*scale}, 
                      "Vocals", TextFormat("%d%%", (int)(globalMixer.volVocals*100)), 
                      &globalMixer.volVocals, 0.0f, 1.0f);
                      
            GuiSlider((Rectangle){sliderX, startY + gap, 200*scale, 20*scale}, 
                      "Drums", TextFormat("%d%%", (int)(globalMixer.volDrums*100)), 
                      &globalMixer.volDrums, 0.0f, 1.0f);
                      
            GuiSlider((Rectangle){sliderX, startY + gap*2, 200*scale, 20*scale}, 
                      "Bass", TextFormat("%d%%", (int)(globalMixer.volBass*100)), 
                      &globalMixer.volBass, 0.0f, 1.0f);

            // "Other" usually contains the Piano, so we label it "Piano/Other"
            GuiSlider((Rectangle){sliderX, startY + gap*3, 200*scale, 20*scale}, 
                      "Piano/Other", TextFormat("%d%%", (int)(globalMixer.volOther*100)), 
                      &globalMixer.volOther, 0.0f, 1.0f);

            if (GuiButton((Rectangle){center - 100*scale, h * 0.7f, 200*scale, 50*scale}, 
                          GuiIconText(ICON_PLAYER_STOP, "Stop"))) {
                StopStemMixer();
                currentState = STATE_IDLE;
            }
        }
        else if (currentState == STATE_ERROR) {
            DrawTextCentered(customFont, "Error Occurred", center, h * 0.3f, subSize, RED);
            DrawTextCentered(customFont, statusMessage, center, h * 0.4f, titleSize, BLACK);
            
            if (GuiButton((Rectangle){150, 250, 200, 40}, "Try Again")) {
                currentState = STATE_IDLE;
                strcpy(statusMessage, "Ready.");
            }
        }

        EndDrawing();
    }

    if (globalMixer.isLoaded) 
        UnloadStemMixer();

    UnloadFont(customFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}