#ifndef LAIN_AUDIO_H
#define LAIN_AUDIO_H

#include "lain.h"

#define PCM 1
#define IEEE_FLOAT 3

enum Format
{
    UNKNOWN = 0,
    U8 = 1,
    S16 = 2,
    S32 = 3,
    F32 = 4,
};

typedef struct
{
    char chunk_id[4]; // RIFF
    i32 chunk_size;   // chunksize
    char format[4];

    char subchunk1_id[4]; // "fmt "
    i32 subchunk1_size;
    i16 audio_format;
    i16 num_channels;
    i32 sample_rate;
    i32 byte_rate;
    i16 block_align;
    i16 bits_per_sample;

    char subchunk2_id[4]; // "data"
    i32 subchunk2_size;
} WavHeader;

typedef struct
{
    i8 *data;
    usize data_size;
} WavData;

typedef struct
{
    enum Format format;

    i16 channels;
    i32 sample_rate;
    i64 sample_count;

    WavData rawdata;
} Audio;

// utils
enum Format audio_detect_format(const WavHeader *raw);
bool is_wav_valid(WavHeader *wav);
bool audio_validate_layout(const WavHeader *raw);
i64 get_num_samples(const WavHeader *raw);
// converting into proper Audio format
bool audio_from_wav(WavHeader* wheader, Audio* a);
#endif // LAIN_AUDIO_H
