#ifndef WAV_H
#define WAV_H

#include "fatfs.h"
#include <string.h>

namespace Storage
{
    typedef struct wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint64_t wav_size = 0; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    short num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint32_t sample_alignment; // num_channels * Bytes Per Sample
    uint32_t bit_per_sample ; // Number of bits per sample
    
    // Data
    char data_header_expected[5] = "data"; // Contains "data"
    uint64_t data_bytes = 0; // Number of bytes in data. Number of samples * num_channels * sample byte size
    char data_header[5]; // Contains "data"

    } wav_header_t;

    class WAV
    {
        public:
            wav_header_t wav_header = {.data_header_expected = "data"} ;
            uint8_t read_buffer1[1024];
            //uint8_t read_buffer2[512];
            char wav_file_name [16][16]; //Array of file names
            WAV();
            void get_wav_header(FIL* file, char* wav_file_name);
            void is_wav_file(char* file_name); 
            osSemaphoreId_t  dac_dma_lock;


        private:
            uint8_t wav_file_count = 0;
    };
    extern WAV* wav;

}

#endif