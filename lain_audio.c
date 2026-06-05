#include "include/lain_audio.h"

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
