/*
 * StrobeHelper.cpp
 * Copyright (C) 2022 Moshe Braner
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
#include "../../SoftRF.h"

#if defined(EXCLUDE_STROBE)
void  Strobe_setup()       {}
void  Strobe_loop()        {}
void  Strobe_fini()        {}
#else

#include "Strobe.h"
#include "Buzzer.h"
#include "Settings.h"
#include "../TrafficHelper.h"

/* need this for the alarm levels enum: */
#include "../protocol/radio/Legacy.h"
/* - perhaps should move those to another location? */

#include "../protocol/data/NMEA.h"

static int StrobePin = SOC_UNUSED_PIN;;
static uint32_t StrobeTimeMarker = 0;
static uint32_t StrobePauseMarker = 0;
static int StrobeFlashes = 0;      /* how many flashes */
static bool StrobeState = false;   /* true = lit */
static int StrobeOnMS  = 0;        /* how long each flash */
static int StrobeOffMS = 0;        /* how long between flashes */
static int alarm_level = ALARM_LEVEL_NONE;

void Strobe_setup(void)
{
#if defined(ESP32)
  if (settings->strobe == STROBE_OFF)
      return;
  StrobePauseMarker = millis();
  StrobePin = SOC_GPIO_PIN_STROBE;
  if (settings->voice != VOICE_OFF) {
      if (hw_info.model == SOFTRF_MODEL_PRIME_MK2 && hw_info.revision < 8) {
          // (gpio15 not available on T-Beam v0.7)
          settings->strobe = STROBE_OFF;
          return;
      }
      StrobePin = SOC_GPIO_PIN_BUZZER2;   // use pin 15 instead
      if (settings->volume != BUZZER_EXT)
          settings->volume = BUZZER_OFF;
  }
  if (StrobePin == SOC_UNUSED_PIN)
      return;
  if (ESP32_pin_reserved(StrobePin, false, "Strobe"))
      return;
#else
  settings->strobe = STROBE_OFF;
  return;
#endif
  pinMode(StrobePin, OUTPUT);
  StrobeState = false;
  StrobeTimeMarker = 0;

  // one double flash to show it is working
  digitalWrite(StrobePin, HIGH);
  delay(50);
  digitalWrite(StrobePin, LOW);
  delay(50);
  digitalWrite(StrobePin, HIGH);
  delay(50);
  digitalWrite(StrobePin, LOW);
}

void Strobe_Start()
{
    if (StrobeTimeMarker != 0)   // flashing currently in progress
        return;

    int NMEAlevel = alarm_level;

    if (StrobePin != SOC_UNUSED_PIN) {

        if (alarm_level >= ALARM_LEVEL_LOW) {
          StrobeFlashes = STROBE_FLASHES_ALARM;
          StrobeOnMS = STROBE_MS_ALARM;
          --NMEAlevel;   // for the NMEA message below
        } else {
          StrobeFlashes = STROBE_FLASHES_NOALARM;
          StrobeOnMS = STROBE_MS_NOALARM;
        }

        digitalWrite(StrobePin, HIGH);
        StrobeState = true;
        StrobeTimeMarker = millis();

    } else {

        /* no hardware output, but pause before next NMEA output */
        StrobePauseMarker = millis();
    }

    //Serial.print("Strobe flash at alarm level: ");
    //Serial.println(alarm_level);

    snprintf_P(NMEABuffer, sizeof(NMEABuffer), PSTR("$PSRSF,%d*"), NMEAlevel);
    NMEAOutC(NMEA_E_OUTPUT);
}

void Strobe_Continue()
{
    if (StrobePin == SOC_UNUSED_PIN)
        return;

    if (StrobeFlashes > 1) {
      if (StrobeState == 1) {   /* a flash is ending */
         digitalWrite(StrobePin, LOW);
         StrobeState = false;
      } else {  /* Strobe is off, start another flash */
         digitalWrite(StrobePin, HIGH);
         StrobeState = true;
         --StrobeFlashes;
      }
      StrobeTimeMarker = millis();   /* reset timer for the next flash or gap */

    } else {
      /* pause for now */
      digitalWrite(StrobePin, LOW);
      StrobeTimeMarker = 0;
      StrobePauseMarker = millis();   /* reset timer for the next flash burst */
    }
}

void Strobe_loop(void)
{
  if (settings->strobe == STROBE_OFF)
      return;

  if (StrobeTimeMarker != 0) {

      /* currently flashing, complete the pattern */

      uint32_t duration = (StrobeState ? StrobeOnMS : STROBE_MS_GAP);

      if ((millis() - StrobeTimeMarker) > duration)
          Strobe_Continue();

  } else if (StrobePauseMarker != 0) {

    /* not currently flashing, flash again after pause */

      uint32_t t = millis() - SetupTimeMarker;
      bool self_test = (do_alarm_demo
                         && t < (1000*STROBE_INITIAL_RUN)
                         && t > (1000*(STROBE_INITIAL_RUN-3)));
      if (self_test)
          alarm_level = ALARM_LEVEL_LOW;
      else if (! alarm_ahead && settings->strobe != STROBE_ALARM)   // not within +-45 degrees of our track
          alarm_level = ALARM_LEVEL_NONE;   // strobe is facing forward, no use
      else
          alarm_level = max_alarm_level;
      uint32_t pause_ms = 0;
      if (self_test) {
            pause_ms = STROBE_MS_PAUSE_ALARM;
      } else {
        if (alarm_level > ALARM_LEVEL_CLOSE) {
            pause_ms = STROBE_MS_PAUSE_ALARM;
        } else if ((settings->strobe == STROBE_AIRBORNE && ThisAircraft.airborne)
                 || settings->strobe == STROBE_ALWAYS) {
            pause_ms = STROBE_MS_PAUSE_NOALARM;
        }
      }

      if (pause_ms != 0 && (millis() - StrobePauseMarker) > pause_ms)
            Strobe_Start();
  }
}

void Strobe_fini(void)
{
  if (StrobePin != SOC_UNUSED_PIN)
      digitalWrite(StrobePin, LOW);    // turns strobe off
  StrobePauseMarker = 0;               // prevents new flashes
}

#endif /* EXCLUDE_STROBE */
