#include "include/lain_audio.h"
#include "include/lain_fs.h"
#include "include/lain_io.h"

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 500 

#define BAR_COUNT 250
#define BAR_GAP 1.0f
#define VISUALIZER_MAX_HEIGHT ((f32)SCREEN_HEIGHT * 0.4f)
#define VISUALIZER_SECONDS 3.0f

static bool read_full(i32 fd, void *buffer, usize size)
{
    u8 *cursor = (u8 *)buffer;
    usize total_read = 0;

    while (total_read < size)
    {
        isize bytes_read = lain_read(fd, cursor + total_read, size - total_read);

        if (bytes_read <= 0)
            return false;

        total_read += (usize)bytes_read;
    }

    return true;
}

static void draw_visualizer(const Audio *audio, f32 played_seconds)
{
    const f32 center_y = (f32)SCREEN_HEIGHT / 2.0f;
    const f32 bar_width = ((f32)SCREEN_WIDTH / (f32)BAR_COUNT) - BAR_GAP;
    const i64 visible_frames = (i64)((f32)audio->sample_rate * VISUALIZER_SECONDS);
    const i64 playhead_frame = (i64)(played_seconds * (f32)audio->sample_rate);

    DrawLine(0, (i32)center_y, SCREEN_WIDTH, (i32)center_y, (Color){64, 64, 72, 255});

    for (i32 i = 0; i < BAR_COUNT; i++)
    {
        i64 start_frame = playhead_frame + ((visible_frames * i) / BAR_COUNT);
        i64 end_frame = playhead_frame + ((visible_frames * (i + 1)) / BAR_COUNT);
        i64 frame_count = end_frame - start_frame;

        if (start_frame >= audio->sample_count)
            break;

        if (frame_count <= 0)
            frame_count = 1;

        f32 peak = 0.0f;
        for (i32 channel = 0; channel < audio->channels; channel++)
        {
            f32 channel_peak = audio_peak(audio, start_frame, frame_count, channel);

            if (channel_peak > peak)
                peak = channel_peak;
        }

        if (peak > 1.0f)
            peak = 1.0f;

        f32 height = peak * VISUALIZER_MAX_HEIGHT;
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
    i32 fd = lain_open(lain_string(path));

    if (fd == -1)
        return -1;

    if (!read_full(fd, &wavheader, sizeof(wavheader)))
    {
        lain_perror(lain_string("ERROR: unable to read wav header"));
        lain_close(fd);
        return -1;
    }

    bool isvalid = is_wav_valid(&wavheader);
    if (!isvalid)
    {
        lain_perror(lain_string("Incorret .wav datatype, unable to parse\n"));
        lain_close(fd);
        return -1;
    }

    if (!audio_from_wav(&wavheader, &audio))
    {
        lain_close(fd);
        return -1;
    }

    audio.rawdata.data = malloc(audio.rawdata.data_size);
    if (!audio.rawdata.data)
    {
        lain_perror(lain_string("ERROR: unable to allocate wav data"));
        lain_close(fd);
        return -1;
    }

    if (!read_full(fd, audio.rawdata.data, audio.rawdata.data_size))
    {
        lain_perror(lain_string("ERROR: unable to read wav data"));
        free(audio.rawdata.data);
        lain_close(fd);
        return -1;
    }

    lain_close(fd);

    printf("Num channels = %i\n", wavheader.num_channels);
    printf("Audio format = %i\n", wavheader.audio_format);
    printf("Format       = %.4s\n", wavheader.format);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "lain audio visualizer");
    InitAudioDevice();
    SetTargetFPS(60);

    Music music = LoadMusicStream(path);

    while (!WindowShouldClose())
    {
        SeekMusicStream(music, 0.0f);
        PlayMusicStream(music);

        while (!WindowShouldClose() && IsMusicStreamPlaying(music))
        {
            UpdateMusicStream(music);

            BeginDrawing();
            ClearBackground((Color){18, 18, 22, 255});

            draw_visualizer(&audio, GetMusicTimePlayed(music));

            EndDrawing();
        }
    }

    StopMusicStream(music);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    free(audio.rawdata.data);

    return 0;
}
