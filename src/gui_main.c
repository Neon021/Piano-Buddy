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
    char input_song_name[256];
    char output_folder_name[256]; 
    bool success;
    bool is_done;
} ProcessorThreadData;

ProcessorThreadData procData;

// LIBRARY GLOBALS
char** librarySongs = NULL;
int libraryCount = 0;
int selectedSongIndex = -1;
int scrollIndex = 0; // Reserved for scrolling later

// MIXER
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

// THREAD FUNCTION
void* processing_thread_func(void* arg) {
    ProcessorThreadData* data = (ProcessorThreadData*)arg;
    int result = process_song_with_python(data->input_song_name, data->output_folder_name);
    data->success = (result == 0);
    data->is_done = true;
    return NULL;
}

// REFRESH LIBRARY
void RefreshLibrary() {
    if (librarySongs) 
        free_song_list(librarySongs, libraryCount);
    librarySongs = get_library_songs(&libraryCount);
}

// HELPER: DRAW CENTERED TEXT
void DrawTextCentered(Font font, const char* text, float centerX, float y, float fontSize, Color color) {
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1.0f);
    float textStartX = centerX - (textSize.x / 2.0f);
    DrawTextEx(font, text, (Vector2){ textStartX, y }, fontSize, 1.0f, color);
}

// MIXER FUNCTIONS
int LoadStemMixer(const char* songFolder) {
    if (globalMixer.isLoaded) {
        UnloadMusicStream(globalMixer.vocals);
        UnloadMusicStream(globalMixer.drums);
        UnloadMusicStream(globalMixer.bass);
        UnloadMusicStream(globalMixer.other);
    }

    char path[512];
    #define LOAD_STEM(stem, file) \
        snprintf(path, sizeof(path), "library/%s/" file, songFolder); \
        globalMixer.stem = LoadMusicStream(path); \
        PlayMusicStream(globalMixer.stem);

    LOAD_STEM(vocals, "vocals.wav");
    LOAD_STEM(drums, "drums.wav");
    LOAD_STEM(bass, "bass.wav");
    LOAD_STEM(other, "other.wav");

    globalMixer.volVocals = 1.0f;
    globalMixer.volDrums = 1.0f;
    globalMixer.volBass = 1.0f;
    globalMixer.volOther = 0.0f;
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

// --- MAIN ---
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Piano Buddy v1.0");
    SetTargetFPS(60);
    
    InitAudioDevice();
    RefreshLibrary(); // Initial Load

    Font customFont = LoadFontEx("resources/Roboto-Bold.ttf", 96, 0, 0);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(customFont);

    AppState currentState = STATE_IDLE;
    char statusMessage[256] = "Ready to start!";
    char songTitle[256] = "Unknown Song";
    char manualInputBuffer[256] = {0};
    bool editMode = true; 
    pthread_t workerThread;

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        float center = w * 0.5f;
        float scale = w / 800.0f; 
        if (scale < 0.5f) scale = 0.5f;
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * scale));

        if (currentState == STATE_PLAYING && globalMixer.isLoaded){
            UpdateStemMixer();
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // HEADER
        DrawTextCentered(customFont, "Piano Buddy", center, h * 0.1f, 70.0f * scale, DARKGRAY);
        DrawLine(w * 0.1f, h * 0.30f, w * 0.9f, h * 0.30f, GRAY);
        GuiStatusBar((Rectangle){0, h - (30 * scale), w, (30 * scale)}, statusMessage);

        float subSize = 35.0f * scale;
        float titleSize = 40.0f * scale;

        if (currentState == STATE_IDLE) {
            float btnW = 200 * scale;
            float btnH = 60 * scale;
            float gap = 20 * scale;
            
            // Buttons
            if (GuiButton((Rectangle){(w * 0.25f) - btnW/2, h * 0.4f, btnW, btnH}, 
                          GuiIconText(ICON_PLAYER_RECORD, "Record New"))) {
                currentState = STATE_RECORDING;
                strcpy(statusMessage, "Recording audio...");
            }

            if (GuiButton((Rectangle){(w * 0.25f) - btnW/2, h * 0.4f + btnH + gap, btnW, btnH}, 
                          GuiIconText(ICON_HAND_POINTER, "Manual Search"))) {
                currentState = STATE_MANUAL_INPUT;
                editMode = true;
                memset(manualInputBuffer, 0, 256);
                strcpy(statusMessage, "Enter song name manually.");
            }

            // --- LIBRARY LIST (Fixed) ---
            float listX = w * 0.6f;
            float listY = h * 0.2f;
            float listW = w * 0.35f;
            float listH = h * 0.6f;

            // FIX: Use DrawTextEx/customFont instead of DrawText (which used tiny default font)
            DrawTextEx(customFont, "Your Library:", (Vector2){listX, listY - 35*scale}, 20*scale, 1.0f, DARKGRAY);
            DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);

            float itemHeight = 40 * scale;
            GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(16 * scale));
            
            for (int i = 0; i < libraryCount; i++) {
                Rectangle itemRect = { listX + 5, listY + 5 + (i * itemHeight), listW - 10, itemHeight - 5 };
                
                // Only draw songs that fit in the box (simple clipping)
                if ((i + 1) * itemHeight > listH) break; 

                if (GuiButton(itemRect, librarySongs[i])) {
                    snprintf(songTitle, sizeof(songTitle), "%s", librarySongs[i]);
                    
                    // Visual feedback
                    BeginDrawing(); 
                        ClearBackground(RAYWHITE);
                        DrawTextCentered(customFont, "Loading Stems...", w/2, h/2, 40*scale, DARKGRAY);
                    EndDrawing();

                    if (LoadStemMixer(songTitle)) {
                        currentState = STATE_PLAYING;
                        strcpy(statusMessage, "Playing custom mix.");
                    } else {
                        strcpy(statusMessage, "Error: Could not load stems.");
                    }
                }
            }
        }
        else if (currentState == STATE_RECORDING) {
            DrawTextCentered(customFont, "Listening...", center, h * 0.4f, subSize, MAROON);
            float pulse = sin(GetTime() * 10) * 10;
            DrawCircleLines(center, h * 0.6f, 40 + pulse, RED);
            EndDrawing(); 

            if (record_audio("recording.wav", 15) != 0) {
                currentState = STATE_ERROR;
            } else {
                currentState = STATE_IDENTIFYING;
                strcpy(statusMessage, "Identifying song...");
            }
            continue; 
        }
        else if (currentState == STATE_IDENTIFYING) {
            DrawTextCentered(customFont, "Identifying...",  center, h * 0.3f, titleSize, BLUE);
            EndDrawing();

            char* result = identify_song("recording.wav");
            if (result == NULL) {
                currentState = STATE_MANUAL_INPUT;
                strcpy(statusMessage, "Identification failed.");
                editMode = true;
                memset(manualInputBuffer, 0, 256);
            } else {
                currentState = STATE_PROCESSING;
                strcpy(statusMessage, "AI Processing...");
                strcpy(procData.input_song_name, result);
                free(result);
                
                procData.is_done = false;
                pthread_create(&workerThread, NULL, processing_thread_func, &procData);
            }
            continue;
        }
        else if (currentState == STATE_MANUAL_INPUT) {
            DrawTextCentered(customFont, "Enter Song Name:", center, h * 0.35f, 30 * scale, DARKGRAY);
            
            float boxW = 400 * scale;
            float boxH = 50 * scale;
            Rectangle boxRect = {center - boxW/2, h * 0.45f, boxW, boxH};
            
            GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(24 * scale));
            
            if (GuiTextBox(boxRect, manualInputBuffer, 256, editMode)) {
                editMode = !editMode;
            }
            
            if (GuiButton((Rectangle){center - 100*scale, h * 0.6f, 200*scale, 50*scale}, "Search")) {
                if (strlen(manualInputBuffer) > 0) {
                    currentState = STATE_PROCESSING;
                    strcpy(statusMessage, "AI Processing...");
                    strcpy(procData.input_song_name, manualInputBuffer);
                    
                    procData.is_done = false;
                    pthread_create(&workerThread, NULL, processing_thread_func, &procData);
                }
            }
            
            if (GuiButton((Rectangle){20*scale, 20*scale, 80*scale, 30*scale}, "Back")) {
                currentState = STATE_IDLE;
            }
        }
        else if (currentState == STATE_PROCESSING) {
            DrawTextCentered(customFont, "Generating Backing Track...", center, h * 0.35f, 30 * scale, DARKGRAY);
            DrawTextCentered(customFont, "This may take 3-4 minutes.", center, h * 0.6f, 15 * scale, GRAY);
            
            GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(18 * scale));
            
            // GuiProgressBar((Rectangle){center - 150*scale, h * 0.5f, 300*scale, 30*scale}, 
            //                "0%", "100%", (float*)processingProgress, 0, 100);
            float time = GetTime();
            DrawCircleLines(center, h * 0.6f, (40 * scale), Fade(BLUE, 0.5f));
            DrawCircle(center + cos(time*5)*30*scale, h * 0.6f + sin(time*5)*30*scale, 10*scale, BLUE);

            if (procData.is_done) {
                pthread_join(workerThread, NULL);
                if (procData.success) {
                    currentState = STATE_PLAYING;
                    strcpy(statusMessage, "Playing Multi-Stem Audio!");
                    
                    // --- FIX: Refresh Library so the new song shows up later ---
                    RefreshLibrary();

                    LoadStemMixer(procData.output_folder_name);
                } else {
                    currentState = STATE_ERROR;
                    strcpy(statusMessage, "Processing Failed.");
                }
            }
        }
        else if (currentState == STATE_PLAYING) {
            DrawTextCentered(customFont, "Now Playing:", center, h * 0.3f, 20 * scale, DARKGRAY);
            DrawTextCentered(customFont, songTitle, center, h * 0.4f, 40 * scale, MAROON);
            
            float time = GetTime();
            DrawCircle((int)(w - 50*scale), (int)(h - 60*scale), (10 + sin(time*5)*3)*scale, MAROON);

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
            
            if (GuiButton((Rectangle){center - 100*scale, h * 0.6f, 200*scale, 40*scale}, "Try Again")) {
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