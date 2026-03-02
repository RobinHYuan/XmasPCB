#ifndef SDCARD_H
#define SDCARD_H

#define MAX_STRING_SIZE 64

#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include <string>
#include <cstring>
#include "ssd1306.h"
#include "wav.h"

namespace Storage
{
    enum class SDCardSelftestResult
    {
        ALLPASS,
        READ_VALID_FAIL,
        READ_FAIL,
        WRITE_FAIL,
        RW_FAIL,
        FORMAT_FAIL,
        NOT_TESTED
    };
    class SDCard
    {
        public:
            int32_t totalBytesWritten;                    //Counter for total bytes written for the current-opened file
            osThreadAttr_t SDCardTaskHandleAttributes;   // Thread Attributes
            osThreadId_t sdCardTaskHandle;                  // Thread ID
            osMessageQueueId_t sdCardQueue;                 //FIFO for the SD Card
            uint8_t QueueSize;
            osMutexId SDCardMutexID;                        //Mutex for the SD Card
            osMutexAttr_t SDCardMutexAttr;                  //Mutex Attributes
            /**
             * @brief sd_card class constructor
             * @param void
             * @note Parameters have been decalred as global variables in "fafts.h"
             */
            SDCard();
            /**
             * @brief SD card write - thread. It would read out string from the
             * sd_card::SD1 -> sdCardQueue and write it into the sd card. Prior to the
             * infinite loop, it also performs, sdCard initialization and read/write tests
             * @param sd_card_pv pointer to the sd card obj
             */
            static void sdCardWriteThread(void* sd_card_pv);
            /**
             * @brief The higher level storage manager will call this method to store
             * a string into FIFO. Even though the input argument is a std::string, a
             * pointer to the string will be created and stored in the FIFO.
             * @param String to be written into the FIFO.
            */
            void logString(std::string msg);
            /**
              * @brief The higher level communication manager will call this method to
              * request for a sef diagnostic test on sd card
            */
            char* getSDCardSelftestResult();

        private:
            uint8_t readBuffer [_MAX_SS];          //Read Buffer Max Stack Size 512
            uint8_t writeBuffer[64];          //Write Buffer
            uint8_t fileIndex;                             //Counter for the number of existing log file
            std::string FILE_NAME_STR;                      //Log file name
            char msgReceive [MAX_STRING_SIZE] ;            //String to be received from the FIFO
            SDCardSelftestResult TestResult;
            /**
             * @brief Mount and Format the SD_Card
             * @param no parameter, SDFatFS, SDPath, and SDFile variables are global
             */
            void sdCardInit(void);
            /**
             * @brief Write the provided string into the specified file name
             * @param data String to be written, ideally has a size <MAX_STRING_SIZE bytes
             */
            FRESULT sdCardWrite( std::string data);
            /**
             * @brief Write the provided string specified by the character array into the specified file name
             * @param data_ptr Pointer to the char[] to be written with a size <MAX_STRING_SIZE bytes
             */
            FRESULT sdCardWrite(char* data_ptr);
            /**
             * @brief Perforem a read & write test on the sd card
             * @note  SDcard must be initialized. An exisitng VALID.TXT file with
             * the test string "The quick brown fox jumps over the lazy dog!\n"
             * needs to be present on the sd card root directory.
             *
             * If all test passed,there will be a TEST.txt with a "Read and Write Test Passed!\n"
             * string. Otherwise, the test will fail and a detailed message will be printed to the
             * sd card if the write function is working.
             *
             * In the absense of a valid SD card, the test will timeout according to the macro
             * "SD_TIMEOUT". f_mount will return a FA_DISK_ERR error code, whichh is non-recoverable.
             * A system reset would be needed to reset the SD card peripheral after it fails to mount
             * if one wish to use the sd card after it has been properly installed.
             *
             * Note the macro "SD_TIMEOUT" defined in FATFS/target/sd_diskio.c is set to 3ms(30)
             * This function will be called from the communication manager and the sd card thread.
             *@return Return Value Mapping
             *              ALLPASS = 0,
             *              READ_VALID_FAIL = 1,
             *              READ_FAIL = 2,
             *              WRITE_FAIL = 3,
             *              RW_FAIL = 4,
             *              FORMAT_FAIL = 5
             */
            void sdCardRWTest();
            /**
             * @brief Check to see if a new file needs to be opened due to file size limit
             * @note  FAT32 can only have a maximum size of 4GBytes.
             */
            void newFileCheck();
        public:
            uint32_t bytesWritten, bytesRead;             //Counter for bytes written/read

    };
    extern SDCard* sdCard;
}


struct aggregated_data_ptr
{
  Storage::SDCard* sd_card;
  Storage::WAV* wav;
  DAC_HandleTypeDef* hdac;
};


#endif