#include "sdcard.h"

Storage::SDCard* Storage::sdCard;

Storage::SDCard::SDCard()
{   // Initialize everything to zero
    this -> bytesWritten = 0;
    this -> bytesRead = 0;
    this -> fileIndex  = 0;
    this -> totalBytesWritten = 0;
    this -> SDCardTaskHandleAttributes =
    {
        .name = "SD_Card_TaskHandle",
        .stack_size = 0x1200,
        .priority = (osPriority_t) osPriorityHigh2
    };
    this->TestResult = SDCardSelftestResult::NOT_TESTED;
}


void Storage::SDCard::sdCardInit(void)
{
    if(f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) == FR_OK) return; // Force Mount the SD Card
    //When the operations fails, there is either no card/file system
    f_mount(&SDFatFS, (TCHAR const*)SDPath, 0); //soft mount
    FRESULT sdFormatRes = f_mkfs((TCHAR const*)SDPath, FM_ANY, 0, this->readBuffer, sizeof(readBuffer)) ;// Format the sd card
    if(sdFormatRes!= FR_OK) 
    {   
        //FR_MKFS_ABORTED: Check your readBuffer size should be 512 bytes
        //FR_DISK_ERR: Check the timeout SD_TIMEOUT in sd_disio.c
        this->TestResult = SDCardSelftestResult::FORMAT_FAIL;
    }
    return; // FILE_NAME will stay at "LOG0.TXT" since the sd card is freshly formatted.
}

FRESULT Storage::SDCard::sdCardWrite(std::string data)
{   // write a string, data into the currently opened file
    FRESULT result = f_write(&SDFile, (void*)data.c_str(), data.length(), (UINT *)(&(this->bytesWritten)));
    if((this->bytesWritten == 0) || (result != FR_OK)) this->TestResult = SDCardSelftestResult::WRITE_FAIL;
    if(f_sync(&SDFile)!= FR_OK) this->TestResult = SDCardSelftestResult::WRITE_FAIL;//flushes the caches into the sd card
    this-> totalBytesWritten +=this->bytesWritten;
    return result;
}


void Storage::SDCard::sdCardRWTest()
{
    if(this -> TestResult == SDCardSelftestResult::FORMAT_FAIL) return;// if format fails, no need to test read and write
    bool read_test_flag = true;// inidcate write test result
    //First perform a read test
    if(f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) != FR_OK)
    {
        this -> TestResult = SDCardSelftestResult::RW_FAIL;
        return;
    }
    if(f_open(&SDFile, "VALID.TXT", FA_READ) == FR_OK)//open and read exisiting file only
    {
        std::string read_test = "The quick brown fox jumps over the lazy dog!\n";//str to be read
        f_read(&SDFile, this->readBuffer, read_test.length(), (UINT *)(&(this->bytesRead)));
        std::string return_str ((char*)readBuffer, this->bytesRead);
        if(read_test.compare(return_str)) read_test_flag = false;//if comparison fails, set flag to 0
        f_close(&SDFile);
    }
    else read_test_flag = false;//failed to open file -> set flag to false
    //Then, a write & read test is perforemd
    if(f_open(&SDFile, "TEST.TXT",  FA_CREATE_ALWAYS | FA_WRITE | FA_READ) == FR_OK) // new file, read & write
    {
        std::string write_test = "Hello World!\n";// string to be written
        FRESULT write_result = Storage::SDCard::sdCardWrite(write_test);// write str

        f_rewind(&SDFile);//reset the file ptr
        f_read(&SDFile, this->readBuffer, write_test.length(), (UINT *)(&(this->bytesRead)));// read str into buffer

        std::string return_str ((char*)readBuffer, this->bytesRead); //copy from buffer to std::string

        if(!read_test_flag)
        {   
            //failed to read the validation file
            if(!write_test.compare(return_str)) //check if what we read is what we wrote
            {
                // if they match, then having no validation file is the only issue. 
                this -> TestResult = SDCardSelftestResult::READ_VALID_FAIL;
                Storage::SDCard::sdCardWrite((std::string)"Read Validation File Test Failed!\n");
            }
            else
            {
                if (write_result == FR_OK)
                {   // if they dont match, then the write function is working since the read validation file 
                    // is read correctly. 
                    this -> TestResult = SDCardSelftestResult::READ_FAIL;
                    Storage::SDCard::sdCardWrite((std::string)"Read Test Failed!\n");
                }
                else
                {   //otherwise, neither read nor write is working
                    this -> TestResult = SDCardSelftestResult::RW_FAIL;
                    Storage::SDCard::sdCardWrite((std::string)"Read and Write Test Failed!\n");
                }
            }
        }
        else
        {
            if(write_test.compare(return_str))// if validation file is read correctly, check if what we read is what we wrote
            {
                this -> TestResult = SDCardSelftestResult::WRITE_FAIL;
                Storage::SDCard::sdCardWrite((std::string)"Write Test Failed!\n");
            }
            else
            {   // if they match, then both read and write are working
                this -> TestResult = SDCardSelftestResult::ALLPASS;
                Storage::SDCard::sdCardWrite((std::string)"Read and Write Test Passed!\n");
            }
        }
        f_close(&SDFile);
    }
    return;
}

void Storage::SDCard::sdCardWriteThread(void* data_ptr)
{
    DIR dir;
    FILINFO fno;

    aggregated_data_ptr* volatile ptr = (aggregated_data_ptr*)data_ptr;
    Storage::SDCard* sd = ptr -> sd_card;// get object
    Storage::WAV* volatile wav = ptr -> wav;// get object
    DAC_HandleTypeDef*  hdac = ptr -> hdac;// get object

    sd  -> sdCardInit();  // initialize the sd card
    sd  -> sdCardRWTest();

    if(sd -> TestResult == SDCardSelftestResult::ALLPASS || sd -> TestResult == SDCardSelftestResult::READ_VALID_FAIL)
    {
        f_opendir(&dir, SDPath);  
        for(uint8_t i = 0 ;f_readdir(&dir, &fno) == FR_OK && fno.fname[0]; i+= 9)
        {
            i = (i > 64) ? 0 : i;
            ssd1306_SetCursor(0, i);
            ssd1306_WriteString(fno.fname,Font_6x8 , White);
            wav->is_wav_file(fno.fname);
        }
        ssd1306_UpdateScreen();
    }




    wav -> get_wav_header(&SDFile, wav->wav_file_name[0]);
    f_read(&SDFile, &(wav->read_buffer1[0]), 512, (UINT *)(&(sd->bytesRead))); 
    HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)wav->read_buffer1, 512, DAC_ALIGN_8B_R);
    bool flag = 0;

    while(1)
    {
        if(osSemaphoreAcquire( wav->dac_dma_lock, 1000) == osOK)
        {
            if(flag == 0)
            {
                f_read(&SDFile, &(wav->read_buffer1[0]), 512, (UINT *)(&(sd->bytesRead)));
                flag = 1;
            }
            else
            {
                f_read(&SDFile, &(wav->read_buffer1[512]), 512, (UINT *)(&(sd->bytesRead)));
                flag = 0;
            }

            if(wav->wav_header.wav_size < 1024)
            {
                f_close(&SDFile);
                while (1);
                
            }

            wav->wav_header.wav_size -= 512;
        }
    }


}

char* Storage::SDCard::getSDCardSelftestResult()
{
    switch(this -> TestResult)
    {
        case SDCardSelftestResult::ALLPASS:
            return (char*)"ALL PASS";
        case SDCardSelftestResult::READ_VALID_FAIL:
            return (char*)"Pass, No Valid.txt";
        case SDCardSelftestResult::READ_FAIL:
            return (char*)"READ FAIL";
        case SDCardSelftestResult::WRITE_FAIL:
            return (char*)"WRITE FAIL";
        case SDCardSelftestResult::RW_FAIL:
            return (char*)"READ & WRITE FAIL";
        case SDCardSelftestResult::FORMAT_FAIL:
            return (char*)"FORMAT FAIL";
        default:
            return (char*)"UNKNOWN";
    }
}