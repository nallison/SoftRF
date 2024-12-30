#ifndef SDCARD_H
#define SDCARD_H

#if defined(ESP32)

char *getline(File file, char *buf, int limit);

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

void SD_setup();
void SD_log(const char * message);
void closeSDlog();

extern bool SDfileOpen;
extern File SIMfile;
extern File TARGETfile;
extern bool SIMfileOpen;
extern bool TARGETfileOpen;

#endif // USE_SD_CARD

#endif // ESP32

#endif // SDCARD_H