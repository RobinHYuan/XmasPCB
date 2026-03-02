#include "wav.h"

Storage::WAV* Storage::wav;

namespace Storage
{
    WAV::WAV()
    {
    }

    void WAV::get_wav_header(FIL*file, char* wav_file_name)
    {
        uint8_t bytes_read  = 0;
        f_open(file, wav_file_name, FA_READ);
        f_read(file, this->wav_header.riff_header, 4, (UINT*)bytes_read);
        f_read(file, &this->wav_header.wav_size, 4, (UINT*)bytes_read);
        f_read(file, this->wav_header.wave_header, 4, (UINT*)bytes_read); //format header, should always be "WAVE"
        f_read(file, this->wav_header.fmt_header, 4, (UINT*)bytes_read); //format header, should always be "fmt "
        f_read(file, &this->wav_header.fmt_chunk_size, 4, (UINT*)bytes_read); //should be 16 for PCM
        f_read(file, &this->wav_header.audio_format, 2, (UINT*)bytes_read); //should be 1 for PCM. 3 for IEEE Float
        f_read(file, &this->wav_header.num_channels, 2, (UINT*)bytes_read); //1 for mono, 2 for stereo
        f_read(file, &this->wav_header.sample_rate, 4, (UINT*)bytes_read); //usually 44100
        f_read(file, &this->wav_header.byte_rate, 4, (UINT*)bytes_read); //Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
        f_read(file, &this->wav_header.sample_alignment, 2, (UINT*)bytes_read); //num_channels * Bytes Per Sample
        f_read(file, &this->wav_header.bit_per_sample, 2, (UINT*)bytes_read); //Number of bits per sample
        for(int8_t res = 1; res != 0;)
        {
            f_read(file, this->wav_header. data_header, 1, (UINT*)bytes_read); //Contains "data"
            FSIZE_t fptr = f_tell(file);
            if(wav_header. data_header[0] == 'd')
            {
                f_read(file, &(this->wav_header.data_header[1]), 3, (UINT*)bytes_read); //Contains "data"
                this ->wav_header. data_header[4] = '\0';
                res = strcmp(this->wav_header.data_header, this->wav_header.data_header_expected);
                if(res != 0) f_lseek(file, fptr);
            }
        }
        f_read(&SDFile, &wav_header.data_bytes, 4, (UINT*)bytes_read); 
    }

    void WAV::is_wav_file(char* file_name)
    {
        if(strstr(file_name, ".WAV") != NULL)
        {
            strcpy(&(this->wav_file_name[this->wav_file_count][0]), file_name);
            this->wav_file_count++;
        }
    }
}