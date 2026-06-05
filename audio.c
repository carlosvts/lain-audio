#include "include/lain_audio.h"
#include "include/lain_fs.h"
#include "include/lain_io.h"
#include <stdio.h>

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
