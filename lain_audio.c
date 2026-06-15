#include "include/lain_audio.h"
#include "include/lain_fs.h"
#include "include/lain_io.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool audio_read_full(i32 fd, void *buffer, usize size)
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

static bool audio_skip_bytes(i32 fd, i32 size)
{
    if (size <= 0)
        return true;

    return lseek(fd, size, SEEK_CUR) != -1;
}

bool audio_read_wav_header(i32 fd, WavHeader *wavheader)
{
    struct
    {
        char id[4];
        i32 size;
    } chunk;

    if (!wavheader)
        return false;

    memset(wavheader, 0, sizeof(*wavheader));

    if (!audio_read_full(fd, wavheader->chunk_id, sizeof(wavheader->chunk_id)) ||
        !audio_read_full(fd, &wavheader->chunk_size, sizeof(wavheader->chunk_size)) ||
        !audio_read_full(fd, wavheader->format, sizeof(wavheader->format)))
    {
        return false;
    }

    if (memcmp(wavheader->chunk_id, "RIFF", 4) != 0 ||
        memcmp(wavheader->format, "WAVE", 4) != 0)
    {
        return false;
    }

    while (audio_read_full(fd, &chunk, sizeof(chunk)))
    {
        if (chunk.size < 0)
            return false;

        i32 padded_size = chunk.size + (chunk.size % 2);

        if (memcmp(chunk.id, "fmt ", 4) == 0)
        {
            memcpy(wavheader->subchunk1_id, chunk.id, sizeof(wavheader->subchunk1_id));
            wavheader->subchunk1_size = chunk.size;

            if (chunk.size < 16)
                return false;

            if (!audio_read_full(fd, &wavheader->audio_format, sizeof(wavheader->audio_format)) ||
                !audio_read_full(fd, &wavheader->num_channels, sizeof(wavheader->num_channels)) ||
                !audio_read_full(fd, &wavheader->sample_rate, sizeof(wavheader->sample_rate)) ||
                !audio_read_full(fd, &wavheader->byte_rate, sizeof(wavheader->byte_rate)) ||
                !audio_read_full(fd, &wavheader->block_align, sizeof(wavheader->block_align)) ||
                !audio_read_full(fd, &wavheader->bits_per_sample, sizeof(wavheader->bits_per_sample)))
            {
                return false;
            }

            if (!audio_skip_bytes(fd, padded_size - 16))
                return false;

            continue;
        }

        if (memcmp(chunk.id, "data", 4) == 0)
        {
            memcpy(wavheader->subchunk2_id, chunk.id, sizeof(wavheader->subchunk2_id));
            wavheader->subchunk2_size = chunk.size;
            return true;
        }

        if (!audio_skip_bytes(fd, padded_size))
            return false;
    }

    return false;
}

enum Format audio_detect_format(const WavHeader *wh)
{
    switch (wh->audio_format)
    {
        case PCM:
        {
            switch (wh->bits_per_sample)
            {
                case 8:
                    return U8;
                case 16:
                    return S16;
                case 24:
                    return UNKNOWN;
                case 32:
                    return S32;
            }
            break;
        }
        case IEEE_FLOAT:
            return F32;
    }
    return UNKNOWN;
}

bool is_wav_valid(WavHeader *wav)
{
    if (!wav)
        return false;

    if (memcmp(wav->chunk_id, "RIFF", 4) != 0)
        return false;

    if (memcmp(wav->format, "WAVE", 4) != 0)
        return false;

    if (memcmp(wav->subchunk1_id, "fmt ", 4) != 0)
        return false;

    if (memcmp(wav->subchunk2_id, "data", 4) != 0)
        return false;

    if (wav->subchunk1_size < 16)
        return false;

    if (wav->audio_format != PCM && wav->audio_format != IEEE_FLOAT)
        return false;

    if (audio_detect_format(wav) == UNKNOWN)
        return false;

    if (wav->num_channels <= 0 || wav->sample_rate <= 0)
        return false;

    if (wav->bits_per_sample <= 0 || wav->subchunk2_size <= 0)
        return false;

    return true;
}

bool audio_validate_layout(const WavHeader *raw)
{
    if (!raw)
        return false;

    i32 frame_size = raw->num_channels * (raw->bits_per_sample / 8);

    if (frame_size <= 0)
        return false;

    // that means that chunk size isnt a multiple of frame size, so something
    // went wrong
    if (raw->subchunk2_size % frame_size != 0)
        return false;

    return true;
}

i64 get_num_samples(const WavHeader *raw)
{
    // assuming audio is valid  and layout is validated
    i64 frame_size = raw->num_channels * (raw->bits_per_sample / 8);
    i64 num_samples = raw->subchunk2_size / frame_size;
    return num_samples;
}

bool audio_from_wav(WavHeader* wheader, Audio* a)
{
    if (!wheader || !a || !is_wav_valid(wheader)) 
    { 
        lain_perror(lain_string("ERROR: Invalid wav file while converting into audio")); 
        return false; 
    }
    if (!audio_validate_layout(wheader))
    {
        lain_perror(lain_string("ERROR: while convertindg Wav to proper Audio type\n"));
        return false;
    }

    a->format = audio_detect_format(wheader);
    a->channels = wheader->num_channels;
    a->sample_rate = wheader->sample_rate;
    a->sample_count = get_num_samples(wheader);

    a->rawdata.data = NULL;
    a->rawdata.data_size = wheader->subchunk2_size;

   return true; 
}

bool audio_load_wav(char *path, WavHeader *wavheader, Audio *audio)
{
    i32 fd = lain_open(lain_string(path));

    if (fd == -1)
        return false;

    if (!audio_read_wav_header(fd, wavheader))
    {
        lain_perror(lain_string("ERROR: unable to read wav header"));
        lain_close(fd);
        return false;
    }

    if (!is_wav_valid(wavheader))
    {
        lain_perror(lain_string("Incorret .wav datatype, unable to parse\n"));
        lain_close(fd);
        return false;
    }

    if (!audio_from_wav(wavheader, audio))
    {
        lain_close(fd);
        return false;
    }

    audio->rawdata.data = malloc(audio->rawdata.data_size);
    if (!audio->rawdata.data)
    {
        lain_perror(lain_string("ERROR: unable to allocate wav data"));
        lain_close(fd);
        return false;
    }

    if (!audio_read_full(fd, audio->rawdata.data, audio->rawdata.data_size))
    {
        lain_perror(lain_string("ERROR: unable to read wav data"));
        audio_free(audio);
        lain_close(fd);
        return false;
    }

    lain_close(fd);
    return true;
}

void audio_free(Audio *audio)
{
    if (!audio)
        return;

    free(audio->rawdata.data);
    audio->rawdata.data = NULL;
    audio->rawdata.data_size = 0;
}

f32 audio_get_sample(const Audio *audio, i64 frame, i32 channel)
{
    if (!audio || !audio->rawdata.data)
        return 0.0f;

    if (channel < 0 || channel >= audio->channels)
        return 0.0f;

    if (frame < 0 || frame >= audio->sample_count)
        return 0.0f;

    i64 index = frame * audio->channels + channel;

    switch (audio->format)
    {
        // Normalizing each case of format in the interval [0,1]
        case U8:
        {
            u8 *samples = (u8 *)audio->rawdata.data;
            return ((f32)samples[index] - 128.0f) / 128.0f;
        }

        case S16:
        {
            i16 *samples = (i16 *)audio->rawdata.data;
            return (f32)samples[index] / 32768.0f;
        }

        case S32:
        {
            i32 *samples = (i32 *)audio->rawdata.data;
            return (f32)samples[index] / 2147483648.0f;
        }

        case F32:
        {
            f32 *samples = (f32 *)audio->rawdata.data;
            return samples[index];
        }

        default:
            return 0.0f;
    }
}
// get the highest peak of amplitude in audio
f32 audio_peak(const Audio *audio, i64 start_frame, i64 frame_count, i32 channel)
{
    f32 peak = 0.0f;

    if (!audio || frame_count <= 0)
        return 0.0f;

    i64 end = start_frame + frame_count;

    if (end > audio->sample_count)
        end = audio->sample_count;

    for (i64 frame = start_frame; frame < end; frame++)
    {
        f32 sample = audio_get_sample(audio, frame, channel);

        if (sample < 0.0f)
            sample = -sample;

        if (sample > peak)
            peak = sample;
    }

    return peak;
}

void audio_visualizer_reset(AudioVisualizer *visualizer)
{
    if (!visualizer || !visualizer->bars)
        return;

    for (i32 i = 0; i < visualizer->bar_count; i++)
        visualizer->bars[i] = 0.0f;

    visualizer->last_frame = 0;
    visualizer->bar_index = 0;
    visualizer->bar_peak = 0.0f;
    visualizer->samples_in_bar = 0;
}

void audio_visualizer_update(AudioVisualizer *visualizer, const Audio *audio, f32 played_seconds)
{
    if (!visualizer || !visualizer->bars || !audio || visualizer->bar_count <= 0 ||
        visualizer->samples_per_bar <= 0)
    {
        return;
    }

    i64 playhead_frame = (i64)(played_seconds * (f32)audio->sample_rate);
    i64 frame_count = (i64)visualizer->bar_count * visualizer->samples_per_bar;

    if (playhead_frame < visualizer->last_frame)
        audio_visualizer_reset(visualizer);

    for (i32 i = 0; i < visualizer->bar_count; i++)
        visualizer->bars[i] = 0.0f;

    if (playhead_frame > audio->sample_count)
        playhead_frame = audio->sample_count;

    i64 start_frame = playhead_frame - frame_count;
    i64 end_frame = playhead_frame;

    if (start_frame < 0)
        start_frame = 0;

    if (end_frame <= start_frame)
        return;

    for (i32 i = 0; i < visualizer->bar_count; i++)
    {
        i64 bar_start = start_frame + ((i64)i * visualizer->samples_per_bar);
        i64 bar_end = bar_start + visualizer->samples_per_bar;
        f32 peak = 0.0f;

        if (bar_start >= end_frame)
            break;

        if (bar_end > end_frame)
            bar_end = end_frame;

        for (i64 frame = bar_start; frame < bar_end; frame++)
        {
            for (i32 channel = 0; channel < audio->channels; channel++)
            {
                f32 sample = audio_get_sample(audio, frame, channel);

                if (sample < 0.0f)
                    sample = -sample;

                if (sample > peak)
                    peak = sample;
            }
        }

        if (peak > 1.0f)
            peak = 1.0f;
        
        // adds the sum of amplitude of a slice of the sample into the visualizer
        visualizer->bars[i] = peak;
    }
    
    // resets the bar index
    visualizer->last_frame = playhead_frame;
    visualizer->bar_index = 0;
    visualizer->bar_peak = 0.0f;
    visualizer->samples_in_bar = 0;
}
