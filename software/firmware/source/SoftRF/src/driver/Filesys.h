#ifndef FILESYS_H
#define FILESYS_H

#include "../system/SoC.h"

#if defined(ESP32)

#if defined(USE_SD_CARD)

// micro SD card SPI pins

// on a separate SPI bus - two choices

#define SD1_SCK     13
#define SD1_MISO    25
#define SD1_MOSI     2
#define SD1_SS       0

#define SD2_SCK     13
#define SD2_MISO    36   // VP
#define SD2_MISO_V7  4   // for T-Beam v0.7
#define SD2_MOSI     2
#define SD2_SS       0

// on same SPI bus as LORA chip
#define SD3_SCK      5
#define SD3_MISO    19
#define SD3_MOSI    27
#define SD3_SS       0

void SD_log(const char * message);
void closeSDlog();

extern bool SD_is_mounted;
extern bool SDlogOpen;
extern File SIMfile;
extern File TARGETfile;
extern bool SIMfileOpen;
extern bool TARGETfileOpen;

#endif // USE_SD_CARD

#define FILESYS SPIFFS
#define FS_is_mounted SPIFFS_is_mounted
#define IGCFILESYS SD
#define IGCFS_is_mounted SD_is_mounted

#endif // ESP32

#if defined(ARDUINO_ARCH_NRF52)

#define FILESYS fatfs
#define FS_is_mounted FATFS_is_mounted
#define IGCFILESYS fatfs
#define IGCFS_is_mounted FATFS_is_mounted

#endif

void Filesys_setup();

bool getline(File &infile, char *buf, int limit);

uint32_t FILESYS_free_kb();    // on SPIFFS (T-Beam) or FATFS (T-Echo)
uint32_t IGCFS_free_kb();      // on SD (T-Beam) or FATFS (T-Echo)

#endif // FILESYS_H
