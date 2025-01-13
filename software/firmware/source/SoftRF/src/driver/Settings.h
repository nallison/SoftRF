/*
 * Settings.h - formerly EEPROMHelper.h
 * Copyright (C) 2016-2021 Linar Yusupov
 * Changed by Moshe Braner 2024 to storing settings in a text file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../SoftRF.h"

//one of the following needs to be defined in SoftRF.h:  (not used any more)
//#define DEFAULT_REGION_EU
//#define DEFAULT_REGION_US

//#if !defined(DEFAULT_REGION_EU) && !defined(DEFAULT_REGION_US)
//#error No default region defined
//#endif

//#if defined(DEFAULT_REGION_EU) && defined(DEFAULT_REGION_US)
//#error Multiple default regions defined
//#endif

#include "../system/SoC.h"

#if !defined(EXCLUDE_EEPROM)
#if defined(ENERGIA_ARCH_CC13XX) || defined(ENERGIA_ARCH_CC13X2)
#include <EEPROM_CC13XX.h>
#else
#include <EEPROM.h>
#endif /* CC13XX or CC13X2 */
#endif /* EXCLUDE_EEPROM */

#define SOFTRF_EEPROM_MAGIC   0xBABADEDA
#define SOFTRF_EEPROM_VERSION 0xACAC0B0D
#define SOFTRF_SETTINGS_VERSION 1

#if defined(ESP32)
#define ABANDON_EEPROM
#endif

#if defined(ARDUINO_ARCH_NRF52)
#define ABANDON_EEPROM
#endif

#if defined(ABANDON_EEPROM)
// the "ui" settings have been appended into "settings"
#define ui settings
#endif

enum
{
	EEPROM_EXT_LOAD,
	EEPROM_EXT_DEFAULTS,
	EEPROM_EXT_STORE
};

enum
{
	STROBE_OFF = 0,
	STROBE_ALARM,
	STROBE_AIRBORNE,
	STROBE_ALWAYS
};

//#if !defined(EXCLUDE_VOICE)
//#if defined(ESP32)
enum
{
	VOICE_OFF = 0,
	VOICE_INT,
	VOICE_EXT
};
//#endif
//#endif

enum
{
	DEST_NONE,
	DEST_UART,
	DEST_UDP,
	DEST_TCP,
	DEST_USB,
	DEST_BLUETOOTH,
	DEST_UART2
};

enum
{
    ADSB_RX_NONE = 0,
    ADSB_RX_GNS5892 = 1,
    ADSB_RX_2,
    ADSB_RX_3
};

enum
{
	BAUD_DEFAULT = 0,
	BAUD_4800 = 1,
	BAUD_9600 = 2,
	BAUD_19200 = 3,
	BAUD_38400 = 4,
	BAUD_57600 = 5,
	BAUD_115200 = 6,
	BAUD_2000000 = 7
};

enum
{
	TCP_MODE_SERVER=0,
	TCP_MODE_CLIENT
};

enum
{
	RELAY_OFF=0,
	RELAY_LANDED,
	RELAY_ALL,
	RELAY_ONLY
};

enum
{
	EXT_GNSS_NONE=0,
	EXT_GNSS_39_4,
	EXT_GNSS_13_2,
	EXT_GNSS_15_14
};

enum
{
	SD_CARD_NONE=0,   // SCK, MISO, MOSI, SS
	SD_CARD_13_25,    // SD card on 13,25,2,0
	SD_CARD_13_VP,    // SD card on 13,VP,2,0
	SD_CARD_LORA      // SD card on 5,19,27,0
};

enum
{
	FLIGHT_LOG_NONE=0,
	FLIGHT_LOG_ALWAYS,
	FLIGHT_LOG_AIRBORNE,
	FLIGHT_LOG_TRAFFIC
};

enum
{
	LOG_INTERVAL_1S = 0,
	LOG_INTERVAL_2S = 1,
	LOG_INTERVAL_4S = 2,
	LOG_INTERVAL_8S = 3
};

enum stgidx {
    STG_NONE,
    STG_VERSION,
    STG_MODE,
    STG_PROTOCOL,
    STG_BAND,
    STG_ACFT_TYPE,
    STG_ID_METHOD,
    STG_AIRCRAFT_ID,
    STG_IGNORE_ID,
    STG_FOLLOW_ID,
    STG_ALARM,
    STG_HRANGE,
    STG_VRANGE,
    STG_TXPOWER,
    STG_VOLUME,
    STG_POINTER,
//#if defined(ESP32)
    STG_STROBE,
    STG_VOICE,
    STG_OWNSSID,
    STG_EXTSSID,
    STG_PSK,
    STG_HOST_IP,
    STG_TCPMODE,
    STG_TCPPORT,
//#endif
    STG_BLUETOOTH,
    STG_BAUD_RATE,
    STG_NMEA_OUT,
    STG_NMEA_G,
    STG_NMEA_P,
    STG_NMEA_L,
    STG_NMEA_S,
    STG_NMEA_D,
    STG_NMEA_E,
//#if defined(ESP32)
    STG_NMEA_OUT2,
    STG_NMEA2_G,
    STG_NMEA2_P,
    STG_NMEA2_L,
    STG_NMEA2_S,
    STG_NMEA2_D,
    STG_NMEA2_E,
    STG_ALTPIN0,
    STG_BAUDRATE2,
    STG_INVERT2,
    STG_ALT_UDP,
    STG_RX1090,
    STG_RX1090X,
    STG_MODE_S,
    STG_HRANGE1090,
    STG_VRANGE1090,
    STG_GDL90_IN,
//#endif
    STG_GDL90,
    STG_D1090,
    STG_RELAY,
    STG_STEALTH,
    STG_NO_TRACK,
    STG_POWER_SAVE,
    STG_POWER_EXT,
    STG_RFC,
    STG_ALARMLOG,
//#if defined(ESP32)
    STG_GNSS_PINS,
    STG_PPSWIRE,
    STG_SD_CARD,
//#endif
    STG_LOGFLIGHT,
    STG_LOGINTERVAL,
    STG_IGC_PILOT,
    STG_IGC_TYPE,
    STG_IGC_REG,
    STG_IGC_CS,
    STG_GEOID,
    STG_JSON,
//#if defined(USE_EPAPER)
    STG_EPD_UNITS,
    STG_EPD_ZOOM,
    STG_EPD_ROTATE,
    STG_EPD_ORIENT,
    STG_EPD_ADB,
    STG_EPD_IDPREF,
    STG_EPD_VMODE,
    STG_EPD_AGHOST,
    STG_EPD_TEAM,
//#endif
    STG_DEBUG_FLAGS,
    STG_END
};

enum stgtyp {
    STG_HEX6  = -4,
    STG_HEX2  = -3,
    STG_UINT1 = -2,
    STG_INT1  = -1,
    STG_VOID  = 0,
    STG_STR    // strings' "type" equals their length
};

struct setting_struct {
    const char *label;
    char *value;
    int8_t type;
};

typedef struct __attribute__((packed)) PackedSettings {

    uint8_t  mode:4;            // do not move
    uint8_t  rf_protocol:4;     // do not move
    uint8_t  band:4;            // do not move
    uint8_t  txpower:2;         // do not move
    uint8_t  volume:2;
    uint8_t  acft_type;         // can be reduced to 5 bits (not 4!)

    uint8_t  geoid:7;
    bool     resvd1:1;

    bool     nmea_g:1;       // do not move
    bool     nmea_p:1;
    bool     nmea_l:1;
    bool     nmea_s:1;
    bool     nmea_d:1;
    uint8_t  nmea_out:3;     // do not move

    uint8_t  bluetooth:3; // ESP32 built-in Bluetooth  // do not move
    uint8_t  alarm:3;        // do not move
    bool     stealth:1;      // do not move
    bool     no_track:1;     // do not move

    uint8_t  gdl90:3;      // output destination
    uint8_t  d1090:3;
    uint8_t  json:1;       // was 2 bits
    bool     resvd2:1;

    uint8_t  pointer:2;
    uint8_t  power_save:2;
    uint8_t  power_ext:1;    /* if nonzero, shuts down if battery is not full */
    uint8_t  rx1090:2;       // attached ADS-B receiver module    // do not move
    bool     mode_s:1;

    uint8_t  gnss_pins:2;    // external GNSS added to T-Beam     // do not move
    uint8_t  sd_card:2;      // gpio pins for SD card adapter
    uint8_t  logflight:2;
    uint8_t  loginterval:2;

    int8_t   freq_corr; /* +/-, kHz */   // <<< limited to +-30, so can liberate two bits
    uint8_t  relay:2;
    uint8_t  gdl90_in:3;    // data from this port will be interpreted as GDL90
    uint8_t  alt_udp:1;     // if 1 then use 10111 instead of 10110
    bool     nmea_e:1;
    bool     nmea2_e:1;     // whether to send bridged data
    uint8_t  baud_rate:3;   // for serial UART0    // do not move
    uint8_t  baudrate2:3;   // for aux UART2       // do not move
    bool     invert2:1;     // whether to invert the logic levels on UART2
    bool     altpin0:1;     // whether to use a different pin for UART0 RX

    /* encryption key provided by gliding contest organizer */
    uint32_t igc_key[4];

    /* added to allow setting aircraft ID and also an ID to ignore */
    uint32_t aircraft_id:24;  // do not move
    uint8_t  id_method:2;     // device ID, ICAO ID, or random    // do not move
    uint8_t  debug_flags:6;   /* each bit activates output of some debug info */
    uint32_t ignore_id:24;    // do not move
    uint8_t  strobe:2;
    bool    logalarms:1;
    uint8_t  voice:2;
    uint8_t  tcpport:1;       // 0=2000, 1=8880   // do not move
    uint8_t  tcpmode:1;       // do not move
    bool     ppswire:1;       // whether PPS wire added  // do not move
    uint32_t follow_id:24;    // do not move

    bool     nmea2_g:1;       // do not move
    bool     nmea2_p:1;
    bool     nmea2_l:1;
    bool     nmea2_s:1;
    bool     nmea2_d:1;
    uint8_t  nmea_out2:3;     // second NMEA output route    // do not move

    char    ssid[19];         // do not move
    char    psk[17];
    char    host_ip[16];

} settingb_t;

typedef struct Settings {

    uint8_t  version;
    uint8_t  mode;
    uint8_t  rf_protocol;
    uint8_t  band;
    uint8_t  acft_type;
    uint8_t  alarm;
    uint8_t  hrange;
    uint8_t  vrange;
    uint8_t  txpower;
    uint8_t  id_method;
    uint32_t aircraft_id;
    uint32_t ignore_id;
    uint32_t follow_id;
    uint8_t  volume;
    uint8_t  pointer;
    uint8_t  bluetooth;     // no effect on T-Echo?  (always BLE, always active?)
    uint8_t  baud_rate;
    uint8_t  nmea_out;
    bool     nmea_g;
    bool     nmea_p;
    bool     nmea_l;
    bool     nmea_s;
    bool     nmea_d;
    bool     nmea_e;
    uint8_t  baudrate2;
    uint8_t  nmea_out2;
    bool     nmea2_g;
    bool     nmea2_p;
    bool     nmea2_l;
    bool     nmea2_s;
    bool     nmea2_d;
    bool     nmea2_e;     // whether to send bridged data
    bool     stealth;
    bool     no_track;
    uint8_t  gdl90;      // output destination
    uint8_t  d1090;
    uint8_t  json;
    int8_t   geoid;
    int8_t   freq_corr; /* +/-, kHz */   // <<< limited to +-30, so can liberate two bits
    uint8_t  relay;
    bool    logalarms;
    uint8_t  debug_flags;   /* each bit activates output of some debug info */

//#if defined(ESP32)
    uint8_t  strobe;
    uint8_t  voice;
    char    myssid[20];    // if using AP mode
    char    ssid[20];      // if connecting to external network
    char    psk[20];
    char    host_ip[16];
    uint8_t  alt_udp;     // if 1 then use 10111 instead of 10110
    uint8_t  tcpmode;
    uint8_t  tcpport;
    uint8_t  power_save;
    uint8_t  power_ext;  /* if nonzero, shuts down if battery is not full */
    uint8_t  rx1090;
    uint8_t  rx1090x;    // settings for the ADS-B receiver module
    bool     mode_s;
    uint8_t  hrange1090;  // km
    uint8_t  vrange1090;  // hundreds of meters
    uint8_t  gdl90_in;    // data from this port will be interpreted as GDL90
    uint8_t  gnss_pins;
    bool     ppswire;
    uint8_t  sd_card;      // gpio pins for SD card adapter
    bool     invert2;     // whether to invert the logic levels on UART2
    bool     altpin0;     // whether to use a different pin for UART0 RX
//#endif
    uint8_t  logflight;
    uint8_t  loginterval;
    char     igc_pilot[32];
    char     igc_type[20];
    char     igc_reg[12];
    char     igc_cs[8];

    /* encryption key provided by contest organizers */
    uint32_t igc_key[4];

//#if defined(USE_EPAPER)
    // EPD UI settings
    uint8_t  units;
    uint8_t  zoom;
    uint8_t  rotate;
    uint8_t  orientation;
    uint8_t  adb;
    uint8_t  epdidpref;
    uint8_t  viewmode;
    uint8_t  antighost;
    uint32_t team;
//#endif

} settings_t;

typedef struct EEPROM_S {
    uint32_t  magic;
    uint32_t  version;
    settingb_t settings;
    uint32_t  version2;    // guard from both ends
} eeprom_struct_t;

typedef union EEPROM_U {
   eeprom_struct_t field;
   uint8_t raw[sizeof(eeprom_struct_t)];
} eeprom_t;

#define DEBUG_WIND 0x01
#define DEBUG_PROJECTION 0x02
#define DEBUG_ALARM 0x04
#define DEBUG_LEGACY 0x08
#define DEBUG_DEEPER 0x10
#define DEBUG_SIMULATE 0x20

void Adjust_Settings(void);
void Settings_setup(void);
void Settings_defaults(bool keepsome);
void EEPROM_store(void);
int  find_setting(const char *label);
bool load_setting(const int index, const char *value);
bool format_setting(const int index, const bool comment);
void show_settings_serial(void);
void save_settings_to_file(void);
bool load_settings_from_file(void);
void do_test_mode(void);

enum stg_default {
    STG_DEFAULT = 0,
    STG_EEPROM  = 1,
    STG_FILE    = 2
};
extern uint8_t settings_used;

extern settings_t *settings;
extern setting_struct stgdesc[STG_END];
extern uint32_t baudrates[];
extern bool do_alarm_demo;
extern bool test_mode;
extern bool landed_out_mode;
extern int8_t geoid_from_setting;

#endif /* SETTINGS_H */
