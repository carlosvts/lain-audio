#include "include/lain_audio.h"

#include <raylib.h>
#include <stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 500 

#define BAR_COUNT 250
#define BAR_GAP 1.0f
#define VISUALIZER_MAX_HEIGHT ((f32)SCREEN_HEIGHT * 0.4f)
#define SAMPLES_PER_BAR 256

static void draw_visualizer(const f32 bars[BAR_COUNT])
{
    const f32 center_y = (f32)SCREEN_HEIGHT / 2.0f;
    const f32 bar_width = ((f32)SCREEN_WIDTH / (f32)BAR_COUNT) - BAR_GAP;

    DrawLine(0, (i32)center_y, SCREEN_WIDTH, (i32)center_y, (Color){64, 64, 72, 255});

    for (i32 i = 0; i < BAR_COUNT; i++)
    {
        f32 height = bars[i] * VISUALIZER_MAX_HEIGHT;
        f32 x = (f32)i * ((f32)SCREEN_WIDTH / (f32)BAR_COUNT);
        f32 y = center_y - height;

        DrawRectangleRec(
            (Rectangle){x, y, bar_width, height},
            (Color){91, 214, 167, 255});
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("use: ./audioinfo <path-to-wav>\n");
        return -1;
    }

    WavHeader wavheader;
    Audio audio;

    char *path = argv[1];

    if (!audio_load_wav(path, &wavheader, &audio))
        return -1;

    printf("Num channels = %i\n", wavheader.num_channels);
    printf("Audio format = %i\n", wavheader.audio_format);
    printf("Format       = %.4s\n", wavheader.format);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "lain audio visualizer");
    InitAudioDevice();
    SetTargetFPS(600);

    Music music = LoadMusicStream(path);
    f32 visualizer_bars[BAR_COUNT];
    AudioVisualizer visualizer = {
        .bars = visualizer_bars,
        .bar_count = BAR_COUNT,
        .samples_per_bar = SAMPLES_PER_BAR,
    };

    while (!WindowShouldClose())
    {
        SeekMusicStream(music, 0.0f);
        PlayMusicStream(music);
        audio_visualizer_reset(&visualizer);

        while (!WindowShouldClose() && IsMusicStreamPlaying(music))
        {
            UpdateMusicStream(music);
            audio_visualizer_update(&visualizer, &audio, GetMusicTimePlayed(music));

            BeginDrawing();
            ClearBackground((Color){18, 18, 22, 255});

            draw_visualizer(visualizer_bars);

            EndDrawing();
        }
    }

    StopMusicStream(music);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    audio_free(&audio);

    return 0;
}
