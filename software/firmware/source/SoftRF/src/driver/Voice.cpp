/*
 * Voice.cpp
 * edited by Moshe Braner from Sound.cpp & code from SkyView
 * and then a lot of trial and error to get the I2S-DAC parameters lined up
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

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "../system/SoC.h"

#if defined(EXCLUDE_VOICE)
void  Voice_setup()       {}
bool  Voice_Notify(ufo_t *fop) {return false;}
void  Voice_loop()        {}
void  Voice_fini()        {}
#else

#if !defined(ESP32)
void  Voice_setup()       {}
bool  Voice_Notify(ufo_t *fop) {return false;}
void  Voice_loop()        {}
void  Voice_fini()        {}
#else

#include "Voice.h"
#include "Buzzer.h"
#include "Strobe.h"
#include "EEPROM.h"

// #include "../platform/ESP32.h"  // included indirectly via SoC.h

/* need this for the alarm levels enum: */
#include "../protocol/radio/Legacy.h"
/* - perhaps should move those to another location? */

#include "../protocol/data/NMEA.h"

extern bool loopTaskWDTEnabled;

static uint32_t VoiceTimeMarker = 0;

bool invert = false;

// >>>>>>>>> external I2S functionality NOT TESTED <<<<<<<<<<<

#include "driver/i2s.h"

// The pin config for external I2S
//  - note this takes over pins 14 & 15 from the buzzer use
i2s_pin_config_t pin_config = {
  .bck_io_num = 14,    // Serial Clock (SCK)
  .ws_io_num = 15,     // Word Select (WS)
  //.data_in_num = I2S_PIN_NO_CHANGE  // not used
  .data_out_num = 25,  // Serial Data (SD)
};

// follow the code here:
// https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp

//i2s configuration

static bool i2s_installed = false;
 
#define i2s_num I2S_NUM_0    // static i2s_port_t i2s_num = I2S_NUM_0;

#define dmabufcount 16
#define dmabuflen   1024
#define dmabufsize 16*1024   // static int dmabufsize = 16*1024;

i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
  .sample_rate = 8000,                                  // assumed known
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,         // was ONLY_RIGHT
  .communication_format = I2S_COMM_FORMAT_STAND_MSB,    // requires ESP32 2.0.5, says schreibfaul1
//.intr_alloc_flags = 0, // default interrupt priority
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,   // high interrupt priority
  .dma_buf_count = dmabufcount,
  .dma_buf_len = dmabuflen,
  .use_apll = true,
  .tx_desc_auto_clear = true,  // was false
  .fixed_mclk = 0   // was I2S_PIN_NO_CHANGE
};

static void setup_i2s()
{
  //initialize i2s with configurations above
  if (settings->voice == VOICE_EXT) {  // I2S output to external device
      i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
      i2s_config.communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S; // Arduino v>2.0.0

      i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
      i2s_set_pin(i2s_num, &pin_config);
  } else {
    //i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
    //i2s_config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
      i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
//    i2s_set_pin(i2s_num, NULL);
      // Enable only I2S built-in DAC channel 1 on GPIO25:
//    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
      i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);         // no audio output yet
  }
}

static uint8_t sample[4] = {0,0,0,0};

// a wrapper for i2s_write() that handles buffer-overflow
static void i2s_writesample()
{
  char *data = (char *) sample;
  size_t written;
  i2s_write(i2s_num, (const char *)data, 4, &written, 100);
  if (written < 4) {
      //yield();
      delay(128);
      i2s_write(i2s_num, (const char *)(data+written), 4-written, &written, 100);
  }
}

// send silent "word" to I2S
static void silence()
{
  sample[1] = sample[3] = (settings->voice==VOICE_INT? 128 : 0);
  int size = 6*1024;  // 750 mS
  while (size-- > 0)
    i2s_writesample();
}

static void play_i2s(const uint8_t *data, int size)
{
  if (i2s_installed == false)
      return;

  bool internal_dac = (settings->voice == VOICE_INT);

  // ramp up to reduce click
  if (internal_dac) {
    for (int i=0; i<1024; i++) {
       sample[1] = sample[3] = (i >> 3);
       i2s_writesample();
    }
  }

  if (size == 0) {

      silence();

  } else {

    bool wdt_status = loopTaskWDTEnabled;
    if (wdt_status)
      disableLoopWDT();

    while (size-- > 0) {
      if (internal_dac) {
         // Need to shift 8-bit samples into MSB of 16-bit ints.
         sample[1] = sample[3] = *data++;
      } else {
         // For external I2S convert from 8-bit unsigned to 16-bit signed.
         sample[1] = sample[3] = (*data++) ^ 0x80;
      }
      i2s_writesample();
    }

    if (wdt_status)
      enableLoopWDT();
  }

  // ramp down to reduce click
  if (internal_dac) {
    for (int i=1024; i>0; i--) {
       sample[1] = sample[3] = (i >> 3);
       i2s_writesample();
    }
  }

  delay(dmabufsize/32);    // wait long enough for the buffer to be flushed
}

static void TTS(const char *msg)
{
    if (SOC_GPIO_PIN_VOICE == SOC_UNUSED_PIN)
      return;

    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);    // enable audio output via GPIO 25 DAC

    VoiceTimeMarker = millis();

    char message[80];
    strcpy(message, msg);

    char *word = strtok (message, " ");

    while (word != NULL)
    {
        int size;
        const uint8_t *wav = word2wav(word, size);
        play_i2s(wav, size);
        word = strtok (NULL, " ");
    }

    play_i2s(NULL, 0);      // silence "word"
    delay(dmabufsize/32);   // wait for the buffer to be flushed (again)
    i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
}

void Voice_test(int reason)
{
    if (settings->voice == VOICE_OFF)
        return;

    delay(800);
    if (reason == REASON_DEFAULT_RST ||
        reason == REASON_EXT_SYS_RST ||
        reason == REASON_SOFT_RESTART) {
         TTS("traffic eleven high");
         delay(3000);
         TTS("danger ahead level");
    } else if (reason == REASON_WDT_RST) {
         TTS("high low high");
    } else {
         TTS("low low low");
    }
}

static void Traffic_Voice_Msg(ufo_t *fop)
{
    int bearing = (int)(fop->bearing - ThisAircraft.course);   // relative bearing

    int oclock = bearing + 15;
    if (oclock < 0)    oclock += 360;
    if (oclock > 360)  oclock -= 360;
    oclock /= 30;

    const char *where;
    switch (oclock)
    {
    case 0:
      where = "ahead";
      break;
    case 1:
      where = "one";   // the audio file says "one o'clock"
      break;
    case 2:
      where = "two";
      break;
    case 3:
      where = "three";
      break;
    case 4:
      where = "four";
      break;
    case 5:
      where = "five";
      break;
    case 6:
      where = "six";
      break;
    case 7:
      where = "seven";
      break;
    case 8:
      where = "eight";
      break;
    case 9:
      where = "nine";
      break;
    case 10:
      where = "ten";
      break;
    case 11:
      where = "eleven";
      break;
    default:
      where = "traffic";
      break;
    }

    char message[80];
    snprintf(message, sizeof(message), "%s %s %s",
        (fop->alarm_level >= ALARM_LEVEL_URGENT? "danger" : "traffic"),
        where,
        (fop->adj_alt_diff > 0 ? "high" : fop->adj_alt_diff < 0 ? "low" : "level"));

    TTS(message);
}

void Voice_setup(void)
{
  if (settings->voice == VOICE_EXT) {
      Buzzer_fini();
      settings->volume = BUZZER_OFF;   // free up pins 14 & 15
  }
  setup_i2s();
  i2s_installed = true;
}

bool Voice_Notify(ufo_t *fop)
{
  if (settings->voice == VOICE_OFF)
      return false;

  if (VoiceTimeMarker != 0)   // voice notification (& subsequent delay) in progress
      return false;

  if (fop->alarm_level < ALARM_LEVEL_LOW)
      return false;

  Traffic_Voice_Msg(fop);

  if ((settings->nmea_l || settings->nmea2_l) && (settings->volume == BUZZER_OFF)){
      snprintf_P(NMEABuffer, sizeof(NMEABuffer),
        PSTR("$PSRAA,%d*"), fop->alarm_level-1);
      NMEA_add_checksum(NMEABuffer, sizeof(NMEABuffer)-10);
      NMEA_Outs(settings->nmea_l, settings->nmea2_l, (byte *) NMEABuffer, strlen(NMEABuffer), false);
  }

  return true;
}

void Voice_loop(void)
{
  if (VoiceTimeMarker != 0 && millis() - VoiceTimeMarker > VOICEMS) {
      VoiceTimeMarker = 0;
  }
}

void Voice_fini(void)
{
  VoiceTimeMarker = 0;
  // stop & destroy i2s driver
  i2s_driver_uninstall(i2s_num);
  i2s_installed = false;
}

#endif  /* ESP32 */

#endif  /* EXCLUDE_VOICE */