/*
 * EEPROMHelper.cpp
 * Copyright (C) 2016-2021 Linar Yusupov
 * Settings scheme redesigned by Moshe Braner 2024
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

#include "../system/SoC.h"

#if defined(EXCLUDE_EEPROM)
void EEPROM_setup()    {}
void EEPROM_store()    {}
#else

#if defined(ESP32)
#include "SPIFFS.h"
#include "SDcard.h"
#define ABANDON_EEPROM
#endif

#if defined(ABANDON_EEPROM)
// read the EEPROM one more time, copy to settings.txt file, then mark EEPROM as obsolete
#else
#define settings_t settingb_t
// keep using the packed settings structure
#endif

#include "EEPROM.h"
#include "RF.h"
#include "LED.h"
#include "Buzzer.h"
#include "Bluetooth.h"
#include "../TrafficHelper.h"
#include "../protocol/radio/Legacy.h"
#include "../protocol/data/NMEA.h"
#include "../protocol/data/GDL90.h"
#include "../protocol/data/D1090.h"
#include "../protocol/data/JSON.h"
#include "Battery.h"

uint8_t default_settings_used;
int settings_file_version = 0;

settings_t settings_stored;
settings_t *settings;

bool do_alarm_demo = false;        // activated by middle button on T-Beam
bool landed_out_mode = false;      // activated by button in status web page

bool test_mode = false;            // activated by double-clicking middle button on T-Beam
                                    // - or via web interface, or via $PSRFT
// Upon receiving a $PSRFT NMEA command,
// first the variable test_mode is toggled, then
// this is called, whether test_mode is on or off
// put custom code here for debugging, for example:
#include "../protocol/data/GNS5892.h"
void do_test_mode()
{
#if defined(ESP32)
    if (settings->rx1090)
        gns5892_test_mode();
#endif
}

uint32_t baudrates[8] = 
{
    0,
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    0
};

setting_struct stgdesc[STG_END];

static void init_stgdesc()
{
  stgdesc[STG_NONE]       = { "none",       (char*)&settings->version,    STG_VOID };
  stgdesc[STG_VERSION]    = { "SoftRF",     (char*)&settings->version,    STG_UINT1 };
  stgdesc[STG_MODE]       = { "mode",       (char*)&settings->mode,       STG_UINT1 };
  stgdesc[STG_PROTOCOL]   = { "protocol",   (char*)&settings->rf_protocol,STG_UINT1 };
  stgdesc[STG_BAND]       = { "band",       (char*)&settings->band,       STG_UINT1 };
  stgdesc[STG_ACFT_TYPE]  = { "acft_type",  (char*)&settings->acft_type,  STG_UINT1 };
  stgdesc[STG_ID_METHOD]  = { "id_method",  (char*)&settings->id_method,  STG_UINT1 };
  stgdesc[STG_AIRCRAFT_ID]= { "aircraft_id",(char*)&settings->aircraft_id,STG_HEX6 };
  stgdesc[STG_IGNORE_ID]  = { "ignore_id",  (char*)&settings->ignore_id,  STG_HEX6 };
  stgdesc[STG_FOLLOW_ID]  = { "follow_id",  (char*)&settings->follow_id,  STG_HEX6 };
  stgdesc[STG_ALARM]      = { "alarm",      (char*)&settings->alarm,      STG_UINT1 };
  stgdesc[STG_TXPOWER]    = { "txpower",    (char*)&settings->txpower,    STG_UINT1 };
  stgdesc[STG_VOLUME]     = { "volume",     (char*)&settings->volume,     STG_UINT1 };
  stgdesc[STG_STROBE]     = { "strobe",     (char*)&settings->strobe,     STG_UINT1 };
  stgdesc[STG_POINTER]    = { "pointer",    (char*)&settings->pointer,    STG_UINT1 };
  stgdesc[STG_VOICE]      = { "voice",      (char*)&settings->voice,      STG_UINT1 };
  stgdesc[STG_BLUETOOTH]  = { "bluetooth",  (char*)&settings->bluetooth,  STG_UINT1 };
  stgdesc[STG_TCPMODE]    = { "tcpmode",    (char*)&settings->tcpmode,    STG_UINT1 };
  stgdesc[STG_TCPPORT]    = { "tcpport",    (char*)&settings->tcpport,    STG_UINT1 };
  stgdesc[STG_OWNSSID]    = { "myssid",     settings->myssid,      sizeof(settings->myssid) };
  stgdesc[STG_EXTSSID]    = { "ssid",       settings->ssid,        sizeof(settings->ssid) };
  stgdesc[STG_PSK]        = { "psk",        settings->psk,         sizeof(settings->psk) };
  stgdesc[STG_HOST_IP]    = { "host_ip",    settings->host_ip,     sizeof(settings->host_ip) };
  stgdesc[STG_NMEA_OUT]   = { "nmea_out",   (char*)&settings->nmea_out,   STG_UINT1 };
  stgdesc[STG_NMEA_G]     = { "nmea_g",     (char*)&settings->nmea_g,     STG_UINT1 };
  stgdesc[STG_NMEA_P]     = { "nmea_p",     (char*)&settings->nmea_p,     STG_UINT1 };
  stgdesc[STG_NMEA_L]     = { "nmea_l",     (char*)&settings->nmea_l,     STG_UINT1 };
  stgdesc[STG_NMEA_S]     = { "nmea_s",     (char*)&settings->nmea_s,     STG_UINT1 };
  stgdesc[STG_NMEA_D]     = { "nmea_d",     (char*)&settings->nmea_d,     STG_UINT1 };
  stgdesc[STG_NMEA_E]     = { "nmea_e",     (char*)&settings->nmea_e,     STG_UINT1 };
  stgdesc[STG_NMEA_OUT2]  = { "nmea_out2",  (char*)&settings->nmea_out2,  STG_UINT1 };
  stgdesc[STG_NMEA2_G]    = { "nmea2_g",    (char*)&settings->nmea2_g,    STG_UINT1 };
  stgdesc[STG_NMEA2_P]    = { "nmea2_p",    (char*)&settings->nmea2_p,    STG_UINT1 };
  stgdesc[STG_NMEA2_L]    = { "nmea2_l",    (char*)&settings->nmea2_l,    STG_UINT1 };
  stgdesc[STG_NMEA2_S]    = { "nmea2_s",    (char*)&settings->nmea2_s,    STG_UINT1 };
  stgdesc[STG_NMEA2_D]    = { "nmea2_d",    (char*)&settings->nmea2_d,    STG_UINT1 };
  stgdesc[STG_NMEA2_E]    = { "nmea2_e",    (char*)&settings->nmea2_e,    STG_UINT1 };
  stgdesc[STG_BAUD_RATE]  = { "baud_rate",  (char*)&settings->baud_rate,  STG_UINT1 };
  stgdesc[STG_ALTPIN0]    = { "altpin0",    (char*)&settings->altpin0,    STG_UINT1 };
  stgdesc[STG_BAUDRATE2]  = { "baudrate2",  (char*)&settings->baudrate2,  STG_UINT1 };
  stgdesc[STG_INVERT2]    = { "invert2",    (char*)&settings->invert2,    STG_UINT1 };
  stgdesc[STG_ALT_UDP]    = { "alt_udp",    (char*)&settings->alt_udp,    STG_UINT1 };
  stgdesc[STG_RX1090]     = { "rx1090",     (char*)&settings->rx1090,     STG_UINT1 };
  stgdesc[STG_RX1090X]    = { "rx1090x",    (char*)&settings->rx1090x,    STG_UINT1 };
  stgdesc[STG_MODE_S]     = { "mode_s",     (char*)&settings->mode_s,     STG_UINT1 };
  stgdesc[STG_GDL90_IN]   = { "gdl90_in",   (char*)&settings->gdl90_in,   STG_UINT1 };
  stgdesc[STG_GDL90]      = { "gdl90",      (char*)&settings->gdl90,      STG_UINT1 };
  stgdesc[STG_D1090]      = { "d1090",      (char*)&settings->d1090,      STG_UINT1 };
  stgdesc[STG_RELAY]      = { "relay",      (char*)&settings->relay,      STG_UINT1 };
  stgdesc[STG_STEALTH]    = { "stealth",    (char*)&settings->stealth,    STG_UINT1 };
  stgdesc[STG_NO_TRACK]   = { "no_track",   (char*)&settings->no_track,   STG_UINT1 };
  stgdesc[STG_POWER_SAVE] = { "power_save", (char*)&settings->power_save, STG_UINT1 };
  stgdesc[STG_POWER_EXT]  = { "power_ext",  (char*)&settings->power_ext,  STG_UINT1 };
  stgdesc[STG_RFC]        = { "rfc",        (char*)&settings->freq_corr,  STG_INT1 };
  stgdesc[STG_ALARMLOG]   = { "alarmlog",   (char*)&settings->logalarms,  STG_UINT1 };
  stgdesc[STG_GNSS_PINS]  = { "gnss_pins",  (char*)&settings->gnss_pins,  STG_UINT1 };
  stgdesc[STG_PPSWIRE]    = { "ppswire",    (char*)&settings->ppswire,    STG_UINT1 };
  stgdesc[STG_SD_CARD]    = { "sd_card",    (char*)&settings->sd_card,    STG_UINT1 };
  stgdesc[STG_LOGFLIGHT]  = { "logflight",  (char*)&settings->logflight,  STG_UINT1 };
  stgdesc[STG_LOGINTERVAL]= { "loginterval",(char*)&settings->loginterval,STG_UINT1 };
  stgdesc[STG_GEOID]      = { "geoid",      (char*)&settings->geoid,      STG_INT1 };
  stgdesc[STG_JSON]       = { "json",       (char*)&settings->json,       STG_UINT1 };
  stgdesc[STG_DEBUG_FLAGS]= { "debug_flags",(char*)&settings->debug_flags,STG_HEX2 };
// >>> for nRF52 also need UI settings

  // ensure no null pointers in the array
  for (int i=0; i<STG_END; i++) {
     if (!stgdesc[i].label) {
         stgdesc[i].label = stgdesc[STG_NONE].label;
         stgdesc[i].value = stgdesc[STG_VERSION].value;
         stgdesc[i].type  = STG_VOID;
     }
  }
}

settingb_t *settingb;

#if defined(ABANDON_EEPROM)
// copy all the settings from settingb to settings
static void copy_settingb()
{
    settings->mode = settingb->mode;
    settings->rf_protocol = settingb->rf_protocol;
    settings->band = settingb->band;
    settings->txpower = settingb->txpower;
    settings->volume = settingb->volume;
    settings->acft_type = settingb->acft_type;
    settings->geoid = 0;
    settings->nmea_g = settingb->nmea_g;
    settings->nmea_p = settingb->nmea_p;
    settings->nmea_l = settingb->nmea_l;
    settings->nmea_s = settingb->nmea_s;
    settings->nmea_d = settingb->nmea_d;
    settings->nmea_out = settingb->nmea_out;
    settings->bluetooth = settingb->bluetooth;
    settings->alarm = settingb->alarm;
    settings->stealth = settingb->stealth;
    settings->no_track = settingb->no_track;
    settings->gdl90 = settingb->gdl90;
    settings->d1090 = settingb->d1090;
    settings->json = settingb->json;
    settings->pointer = settingb->pointer;
    settings->power_save = settingb->power_save;
    settings->power_ext = settingb->power_ext;
    settings->rx1090 = settingb->rx1090;
    settings->rx1090x = 100;
    settings->mode_s = settingb->mode_s;
    settings->gnss_pins = settingb->gnss_pins;
    settings->sd_card = settingb->sd_card;
    settings->logflight = settingb->logflight;
    settings->loginterval = settingb->loginterval;
    settings->freq_corr = settingb->freq_corr;
    settings->relay = settingb->relay;
    settings->gdl90_in = settingb->gdl90_in;
    settings->alt_udp = settingb->alt_udp;
    settings->nmea_e = settingb->nmea_e;
    settings->nmea2_e = settingb->nmea2_e;
    settings->baud_rate = settingb->baud_rate;
    settings->baudrate2 = settingb->baudrate2;
    settings->invert2 = settingb->invert2;
    settings->altpin0 = settingb->altpin0;
    settings->aircraft_id = settingb->aircraft_id;
    settings->id_method = settingb->id_method;
    settings->debug_flags = settingb->debug_flags;
    settings->ignore_id = settingb->ignore_id;
    settings->strobe = settingb->strobe;
    settings->logalarms = settingb->logalarms;
    settings->voice = settingb->voice;
    settings->tcpport = settingb->tcpport;
    settings->tcpmode = settingb->tcpmode;
    settings->ppswire = settingb->ppswire;
    settings->follow_id = settingb->follow_id;
    settings->nmea2_g = settingb->nmea2_g;
    settings->nmea2_p = settingb->nmea2_p;
    settings->nmea2_l = settingb->nmea2_l;
    settings->nmea2_s = settingb->nmea2_s;
    settings->nmea2_d = settingb->nmea2_d;
    settings->nmea_out2 = settingb->nmea_out2;
    settings->myssid[0] = '\0';
    strcpy(settings->ssid,settingb->ssid);
    strcpy(settings->psk,settingb->psk);
    strcpy(settings->host_ip,settingb->host_ip);
}
#endif

// start reading from the first byte (address 0) of the EEPROM
eeprom_t eeprom_block;

// whether to save some settings from the previous version
bool keepsome = false;

void EEPROM_setup()
{
  settings = &settings_stored;
  init_stgdesc();

#if defined(ESP32)
  if (load_settings_from_file()) {
// >>> for nRF52 need to also read UI settings
      //Serial.println(F("Current settings:"));
      //show_settings_serial();
      return;
  }
#endif

  // settings file not found, read the old EEPROM block

  int cmd = EEPROM_EXT_LOAD;

  if (!SoC->EEPROM_begin(sizeof(eeprom_t)))
  {
    Serial.print(F("ERROR: Failed to initialize "));
    Serial.print(sizeof(eeprom_t));
    Serial.println(F(" bytes of EEPROM!"));
    Serial.flush();
    delay(1000000);
  }

  for (int i=0; i<sizeof(eeprom_t); i++) {
    eeprom_block.raw[i] = EEPROM.read(i);
  }

  settingb = &eeprom_block.field.settings;

//Serial.print("sizeof(eeprom_t): ");
//Serial.println(sizeof(eeprom_t));
//Serial.print("sizeof(eeprom_block.field.settings): ");
//Serial.println(sizeof(eeprom_block.field.settings));

  if (eeprom_block.field.magic != SOFTRF_EEPROM_MAGIC) {
    Serial.println(F("WARNING! User defined settings are not initialized yet. Loading defaults..."));

    EEPROM_defaults();
    cmd = EEPROM_EXT_DEFAULTS;
  } else {
    Serial.print(F("EEPROM version: "));
    Serial.println(eeprom_block.field.version, HEX);

    if (eeprom_block.field.version  != SOFTRF_EEPROM_VERSION
    ||  settingb->ssid[sizeof(settingb->ssid)-1] != '\0'
    ||  settingb->psk[sizeof(settingb->psk)-1] != '\0'
    ||  eeprom_block.field.version2 != SOFTRF_EEPROM_VERSION) {
      Serial.println(F("WARNING! EEPROM Version mismatch. Loading defaults..."));

        keepsome = (eeprom_block.field.version  == SOFTRF_EEPROM_VERSION - 1
                 && eeprom_block.field.version2 == SOFTRF_EEPROM_VERSION - 1);
        // keep some settings from the previous version

        EEPROM_defaults();
        cmd = EEPROM_EXT_DEFAULTS;

    } else {
      Serial.println(F("Loaded existing user settings from EEPROM block"));
      default_settings_used = STG_EEPROM;
    }
  }

  SoC->EEPROM_extension(cmd);

  // in case re-writing to EEPROM
  eeprom_block.field.magic    = SOFTRF_EEPROM_MAGIC;
  eeprom_block.field.version  = SOFTRF_EEPROM_VERSION;
  eeprom_block.field.version2 = SOFTRF_EEPROM_VERSION;

#if defined(ABANDON_EEPROM)
  copy_settingb();          // copy all the settings from packed-bits settingb to settings
  //Serial.println(F("Settings copied from EEPROM"));
  //show_settings_serial();
  save_settings_to_file();  // save to a file, next boot will read from the file
#else
  settings = settingb;
  Serial.println(F("Settings read from EEPROM:"));
  show_settings_serial();
#endif
}

void EEPROM_defaults()
{
  default_settings_used = STG_DEFAULT;
  settings->version = SOFTRF_SETTINGS_VERSION;

  if (keepsome == false) {    // the following may be kept from previous version

    settings->mode          = SOFTRF_MODE_NORMAL;
    settings->rf_protocol   = hw_info.model == SOFTRF_MODEL_BRACELET ?
                                RF_PROTOCOL_FANET : RF_PROTOCOL_LATEST;
#if defined(DEFAULT_REGION_US)
    settings->band        = RF_BAND_US;
#else
    settings->band        = RF_BAND_EU;
#endif
    settings->acft_type   = hw_info.model == SOFTRF_MODEL_BRACELET ?
                                                AIRCRAFT_TYPE_STATIC :
                                                AIRCRAFT_TYPE_GLIDER;
    settings->id_method   = ADDR_TYPE_FLARM;
    settings->aircraft_id = 0;
    settings->txpower     = RF_TX_POWER_FULL;

#if defined(USBD_USE_CDC) && !defined(DISABLE_GENERIC_SERIALUSB)
    settings->nmea_out    = DEST_USB;
    settings->nmea_out2   = DEST_NONE;
#else
    settings->nmea_out    = hw_info.model == SOFTRF_MODEL_BADGE ?
                                             DEST_BLUETOOTH :
                                          (hw_info.model == SOFTRF_MODEL_PRIME ?
                                             DEST_UDP :
                                          (hw_info.model == SOFTRF_MODEL_PRIME_MK2 ?
                                             DEST_UDP :
                                           DEST_UART));
    settings->nmea_out2   = hw_info.model == SOFTRF_MODEL_BADGE ?
                                             DEST_USB :
                                          (hw_info.model == SOFTRF_MODEL_PRIME ?
                                             DEST_UART :
                                          (hw_info.model == SOFTRF_MODEL_PRIME_MK2 ?
                                             DEST_UART :
                                           DEST_NONE));
#endif

    settings->nmea_g  = true;
    settings->nmea_p  = false;
    settings->nmea_l  = true;
    settings->nmea_s  = true;
    settings->nmea_d  = false;
    settings->nmea_e  = false;

    settings->nmea2_g = true;
    settings->nmea2_p = false;
    settings->nmea2_l = true;
    settings->nmea2_s = true;
    settings->nmea2_d = false;
    settings->nmea2_e = false;

    settings->bluetooth  = BLUETOOTH_OFF;
    settings->alarm      = TRAFFIC_ALARM_LEGACY;
    settings->stealth    = false;
    settings->no_track   = false;

    settings->baud_rate  = BAUD_DEFAULT;      // Serial  - meaning 38400
    settings->baudrate2  = BAUD_DEFAULT;      // Serial2 - meaning disabled
    settings->invert2    = false;
    settings->freq_corr  = 0;

    if (hw_info.model == SOFTRF_MODEL_STANDALONE
     || hw_info.model == SOFTRF_MODEL_PRIME) {
      //settings->volume  = BUZZER_OFF;
      settings->strobe  = STROBE_OFF;
      settings->pointer = DIRECTION_NORTH_UP;
    } else if (hw_info.model == SOFTRF_MODEL_PRIME_MK2) {
      //settings->volume  = BUZZER_VOLUME_FULL;
      settings->strobe  = STROBE_OFF;
      settings->pointer = LED_OFF;
    } else {
      //settings->volume  = BUZZER_OFF;
      settings->strobe  = STROBE_OFF;
      settings->pointer = LED_OFF;
    }
    settings->voice = VOICE_OFF;

    settings->ignore_id = 0;
    settings->follow_id = 0;

    settings->tcpmode = TCP_MODE_SERVER;
    strncpy(settings->host_ip, NMEA_TCP_IP, sizeof(settings->host_ip)-1);
    settings->host_ip[sizeof(settings->host_ip)-1] = '\0';
    settings->tcpport = 0;   // 2000
    settings->alt_udp    = false;

    settings->gdl90_in   = DEST_NONE;
    settings->gdl90      = DEST_NONE;
#if !defined(EXCLUDE_D1090)
    settings->d1090      = DEST_NONE;
#endif

    settings->logalarms  = false;
  }
  // otherwise keep those settings from the previous version

  // move volume back above next time
    if (hw_info.model == SOFTRF_MODEL_STANDALONE
     || hw_info.model == SOFTRF_MODEL_PRIME) {
      settings->volume  = BUZZER_OFF;
    } else if (hw_info.model == SOFTRF_MODEL_PRIME_MK2) {
      settings->volume  = BUZZER_VOLUME_FULL;
    } else {
      settings->volume  = BUZZER_OFF;
    }

    settings->gnss_pins = EXT_GNSS_NONE;   // whether an external GNSS module was added to a T-Beam
    settings->ppswire   = false;       // whether T-Beam v0.7 or external GNSS has PPS wire connected
    settings->sd_card   = SD_CARD_NONE;
    settings->logflight = FLIGHT_LOG_NONE;
    settings->loginterval = LOG_INTERVAL_4S;
    settings->rx1090    = ADSB_RX_NONE;
    settings->rx1090x   = 100;
    settings->mode_s    = false;
    settings->geoid     = 0;

    settings->myssid[0] = '\0';
    settings->myssid[sizeof(settings->ssid)-1] = '\0';
    settings->ssid[0] = '\0';   // default is empty string - speeds up booting
    settings->ssid[sizeof(settings->ssid)-1] = '\0';
    settings->psk[0] = '\0';
    settings->psk[sizeof(settings->psk)-1] = '\0';

  // the settings below get reset:

  settings->relay = RELAY_OFF;   // >>> revert to RELAY_LANDED as default eventually

  settings->json        = JSON_OFF;
  settings->power_save  = (hw_info.model == SOFTRF_MODEL_BRACELET ? POWER_SAVE_NORECEIVE : POWER_SAVE_NONE);
  settings->power_ext   = 0;
  settings->altpin0     = false;
  settings->debug_flags = 0;      // if and when debug output will be turned on - 0x3F for all

  settings->igc_key[0] = 0;
  settings->igc_key[1] = 0;
  settings->igc_key[2] = 0;
  settings->igc_key[3] = 0;
}

void EEPROM_store()
{
  Serial.println("Writing EEPROM...");

  for (int i=0; i<sizeof(eeprom_t); i++) {
    EEPROM.write(i, eeprom_block.raw[i]);
  }

  SoC->EEPROM_extension(EEPROM_EXT_STORE);

  EEPROM_commit();
}


void show_settings_serial()
{
  for (int i=STG_MODE; i<STG_END; i++) {
     int t = stgdesc[i].type;
     if (t == STG_VOID)
         continue;
     Serial.print(stgdesc[i].label);
     Serial.print(",");
     char *v = stgdesc[i].value;
     switch (t) {
     case STG_INT1:
        Serial.println((int)(*(int8_t*)v));
        break;
     case STG_UINT1:
        Serial.println((int)(*(uint8_t*)v));
        break;
     case STG_HEX2:
        Serial.printf("%02X\r\n", (int)(*(uint8_t*)v));
        break;
     case STG_HEX6:
        Serial.printf("%06X\r\n", (int)(*(uint32_t*)v));
        break;
     default:
        Serial.println((t > STG_VOID)? v : "?");
        break;
     }
  }
#if defined(USE_OGN_ENCRYPTION)
    if (settings->rf_protocol == RF_PROTOCOL_OGNTP) {
        Serial.print("IGC key");
        Serial.printf(" %08X", (settings->igc_key[0]? 0x88888888 : 0));
        Serial.printf(" %08X", (settings->igc_key[1]? 0x88888888 : 0));
        Serial.printf(" %08X", (settings->igc_key[2]? 0x88888888 : 0));
        Serial.printf(" %08X\r\n", (settings->igc_key[3]? 0x88888888 : 0));
    }
#endif
}

#if defined(ESP32)

void save_settings_to_file()
{
  Serial.println(F("Saving settings to SPIFFS/settings.txt ..."));
  if (SPIFFS.exists("/settings.txt"))
      SPIFFS.remove("/settings.txt");
  File SettingsFile = SPIFFS.open("/settings.txt", FILE_WRITE);
  if (!SettingsFile) {
      Serial.println(F("Failed to open settings.txt"));
      return;
  }
  settings->version = SOFTRF_SETTINGS_VERSION;
  bool write_error = false;
  for (int i=STG_VERSION; i<STG_END; i++) {
     int t = stgdesc[i].type;
     if (t != STG_VOID && write_error == false) {
        int n = sizeof(NMEABuffer);
        char *v = stgdesc[i].value;
        switch (t) {
        case STG_INT1:
           snprintf(NMEABuffer,n,"%s,%d\r\n", stgdesc[i].label, (int)(*(int8_t*)v));
           break;
        case STG_UINT1:
           snprintf(NMEABuffer,n,"%s,%d\r\n", stgdesc[i].label, (int)(*(uint8_t*)v));
           break;
        case STG_HEX2:
           snprintf(NMEABuffer,n,"%s,%02X\r\n", stgdesc[i].label, (int)(*(uint8_t*)v));
           break;
        case STG_HEX6:
           snprintf(NMEABuffer,n,"%s,%06X\r\n", stgdesc[i].label, (int)(*(uint32_t*)v));
           break;
        default:
           snprintf(NMEABuffer,n,"%s,%s\r\n", stgdesc[i].label, (t > STG_VOID)? v : "?");
           break;
        }
        int len = strlen(NMEABuffer);
        if (SettingsFile.write((const uint8_t *)NMEABuffer, len) == len) {
            Serial.print(NMEABuffer);
        } else {
            Serial.println(F("Error writing to settings.txt"));
            write_error = true;
            break;
        }
     }
  }
  SettingsFile.close();
  if (write_error)
      SPIFFS.remove("/settings.txt");
  else
      Serial.println(F("... OK"));
}

bool try_match_setting(const char *p, const char *q, const int idx)
{
    if (strcmp(p,stgdesc[idx].label)!=0)
        return false;
    int t = stgdesc[idx].type;
    if (t == STG_VOID)
        return false;
    if (t < STG_VOID && *q == '\0')
            return false; 
    char *v = stgdesc[idx].value;
    switch(t) {
    case STG_INT1:
        *(int8_t *)v = (int8_t) atoi(q);
        break;
    case STG_UINT1:
        *(uint8_t *)v = (uint8_t) atoi(q);
        break;
    case STG_HEX2:
        *(uint8_t *)v = (uint8_t) strtol(q,NULL,16);
        break;
    case STG_HEX6:
        *(uint32_t *)v = (uint32_t) strtol(q,NULL,16);
        break;
    default:
        if (t < STG_VOID)
            return false;
        strncpy(v, q, t);
        v[t-1] = '\0';
        break;
    }
    return true;
}

static bool interpretSetting()
{
    char *p = NMEABuffer;
    char *q = p;
    while (*q != ',') {
        q++;
        if (*q == '\0')
            return false;
    }
    *q = '\0';  // overwrites the comma
    ++q;
    char *r = q;
    while (*r != '\0') {
        if (*r == ' ') {
            *r = '\0';       // treat anything after a space as a comment
            break;
        }
        r++;
    }
    //if (*q == '\0')          // nothing after the comma - leave it as is
    //    return false;

    for (int i=STG_VERSION; i<STG_END; i++) {
        if (try_match_setting(p, q, i))
            return true;
    }
    return false;
}

bool load_settings_from_file()
{
    if (! SPIFFS.exists("/settings.txt")) {
        Serial.println(F("File settings.txt does not exist"));
        return false;
    }
    File SettingsFile = SPIFFS.open("/settings.txt", FILE_READ);
    if (!SettingsFile) {
        Serial.println(F("Failed to open settings.txt"));
        return false;
    }
    Serial.println(F("Loading settings from file..."));
    EEPROM_defaults();    // defaults used for any setting not mentioned in the file
    //settings->version = 0;
    int nsettings = 0;
    while (getline(SettingsFile, NMEABuffer, sizeof(NMEABuffer))) {
        // allow blank or comment lines
        if (NMEABuffer[0] == '#')  continue;
        if (NMEABuffer[0] == '*')  continue;
        if (NMEABuffer[0] == ';')  continue;
        if (NMEABuffer[0] == '/')  continue;
        if (NMEABuffer[0] == ' ')  continue;
        if (NMEABuffer[0] == '\0')  continue;
        Serial.print(NMEABuffer);
        if (interpretSetting()) {
            //Serial.println(" - loaded OK");
            Serial.println("");
        } else {
            SettingsFile.close();
            Serial.println(F(" - invalid setting label, erased file"));
            SPIFFS.remove("/settings.txt");
            EEPROM_defaults();
            return false;
        }
        ++nsettings;
    }
    SettingsFile.close();
    if (settings->version != SOFTRF_SETTINGS_VERSION) {
        // all labels valid, but version number was wrong or missing
        Serial.println(F("bad settings.txt version, erased file"));
        SPIFFS.remove("/settings.txt");
        EEPROM_defaults();
        return false;
    }
    Serial.print("... Loaded ");
    Serial.print(nsettings);
    Serial.println(F(" settings from file"));
    default_settings_used = STG_FILE;    // may be a mix of defaults and values from file
    if (settings->geoid >   84)  settings->geoid =   84;
    if (settings->geoid < -104)  settings->geoid = -104;
    return true;
}

#endif /* ESP32 */

#endif /* ! EXCLUDE_EEPROM */
