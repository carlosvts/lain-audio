#include "include/lain_audio.h"
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
