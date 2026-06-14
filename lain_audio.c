#include "include/lain_audio.h"
#include "include/lain_io.h"
#include <string.h> // for memcpy
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
    if (memcmp(wav->chunk_id, "RIFF", 4) != 0)
        return false;

    if (memcmp(wav->format, "WAVE", 4) != 0)
        return false;

    if (memcmp(wav->subchunk1_id, "fmt ", 4) != 0)
        return false;

    if (memcmp(wav->subchunk2_id, "data", 4) != 0)
        return false;

    return true;
}

bool audio_validate_layout(const WavHeader *raw)
{
    i32 frame_size = raw->num_channels * (raw->bits_per_sample / 8);
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
    if (!audio_validate_layout(wheader)) 
    { 
        lain_perror(lain_string("ERROR: while convertindg Wav to proper Audio type\n")); 
        return false; 
    }
    if (!wheader || !a || !is_wav_valid(wheader)) 
    { 
        lain_perror(lain_string("ERROR: Invalid wav file while converting into audio")); 
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
