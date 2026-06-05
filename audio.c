#include "include/lain_audio.h"
#include "include/lain_fs.h"
#include "include/lain_io.h"
#include <stdio.h>

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("use: ./audioinfo <path-to-wav>\n");
        return -1;
    }
    WavHeader wavheader;
    WavData wavdata;

    char *path = argv[1];
    i32 fd = lain_open(lain_string(path));

    // read wavheader
    lain_read(fd, &wavheader, sizeof(wavheader));
    // read wavdata
    lain_read(fd, &wavdata, sizeof(wavdata));

    bool isvalid = is_wav_valid(&wavheader);
    if (!isvalid)
    {
        lain_perror(lain_string("Incorret .wav datatype, unable to parse\n"));
        return -1;
    }

    printf("Num channels = %i\n", wavheader.num_channels);
    printf("Audio format = %i\n", wavheader.audio_format);
    printf("Format       = %.4s\n", wavheader.format);

    return 0;
}
