/*
  Copyright 2011  Mathieu SONET (contact [at] elasticsheep [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "player.h"
#include "recorder.h"

#include "keyboard.h"
#include "delay.h"
#include "sd_raw.h"
#include "LUFA/Drivers/Peripheral/SerialStream.h"

/*****************************************************************************
* Constants
******************************************************************************/
enum {
  STATE_IDLE,
  STATE_IS_PLAYING,
  STATE_IS_RECORDING
};

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/
struct {
  uint8_t state;
} app;

/*****************************************************************************
* Function prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

void stop_all(void)
{
  switch(app.state)
  {
    case STATE_IS_PLAYING:
      player_stop();
      break;
      
    case STATE_IS_RECORDING:
      recorder_stop();
      break;
  }
}

void end_of_playback(void)
{
  printf_P(PSTR("End of playback\r\n"));
  
  player_stop();
  app.state = STATE_IDLE;
}

void play(uint32_t start_sector, uint16_t nb_sectors)
{
  printf_P(PSTR("Start playing...\r\n"));

  if (app.state != STATE_IDLE)
    stop_all();
  
  app.state = STATE_IS_PLAYING;

  /* Start the playback */
  player_start(start_sector, nb_sectors, &end_of_playback);
}

void end_of_record(void)
{
  printf_P(PSTR("End of record\r\n"));
  
  recorder_stop();
  app.state = STATE_IDLE;
}

void record(uint32_t start_sector, uint16_t nb_sectors)
{
  printf_P(PSTR("Start recording...\r\n"));
  
  if (app.state != STATE_IDLE)
    stop_all();
  
  app.state = STATE_IS_RECORDING;
  
  /* Start the recording */
  recorder_start(start_sector, nb_sectors, &end_of_record);
}

void init_leds(void)
{
  /* Teensy onboard led */
  DDRD |= _BV(PORTD6);
  PORTD &= ~_BV(PORTD6);
}

void onboard_led_toggle(void)
{
  PORTD ^= _BV(PORTD6);
}

void init_keyboard_polling(void)
{
  /* Init the keyboard polling timer */
  TCCR1B = _BV(WGM12); /* CTC mode */

#if (F_CPU == 16000000)
  TCCR1B |= _BV(CS12); /* Fclk / 256 */
  OCR1A = (uint16_t)(1250 - 1); /* 50 Hz */
#else
#error F_CPU not supported
#endif

  /* Enable interrupt on channel A */
  TIMSK1 |= _BV(OCIE1A); 
}

/* Keyboard polling interrupt */
ISR(TIMER1_COMPA_vect)
{
  uint8_t event;
  
  onboard_led_toggle();
  
  keyboard_update(&event);
  
  if (event & EVENT_KEY_PRESSED)
    printf("P %i\r\n", event & KEYCODE_MASK);
    
  if (event & EVENT_KEY_RELEASED)
    printf(" R %i\r\n", event & KEYCODE_MASK);
}

void hardware_init(void)
{
  set_sleep_mode(SLEEP_MODE_IDLE);

  SerialStream_Init(38400, false);
  
  /* Init the sd card */
  while(1)
  {
    if (sd_raw_init())
       break;

    printf_P(PSTR("MMC/SD initialization failed\n"));
  }
  
  init_leds();
  
  keyboard_init(2);
  init_keyboard_polling();
  
  /* Enable interrupts */
  sei();
}

int application_main(void)
{
  hardware_init();

  /* Init the application state */
  app.state = STATE_IDLE;

  /* Mainloop */
  while(1)
  {

  }

  return 0;
}


