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
  STATE_RECORD_SELECT_SLOT,
  STATE_PLAYING,
  STATE_RECORDING,
  
  STATE_SAME = 0xFF,
};

#define NB_BANKS (3)

#define SLOT_NB_BLOCKS (64)
#define BANK_NB_SLOTS  (9)

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/
volatile struct {
  uint8_t state;
  uint8_t bank;
  
  uint8_t key_event_flag;
  uint8_t key_event;
} app;

/*****************************************************************************
* Function prototypes
******************************************************************************/
void set_next_state(uint8_t next_state);

/*****************************************************************************
* Functions
******************************************************************************/

uint32_t get_start_block(uint8_t bank, uint8_t slot)
{
  return (bank * BANK_NB_SLOTS * SLOT_NB_BLOCKS) + (slot * SLOT_NB_BLOCKS);
}

uint32_t get_slot_size(uint8_t bank, uint8_t slot)
{
  return SLOT_NB_BLOCKS;
}

void stop_all(void)
{
  switch(app.state)
  {
    case STATE_PLAYING:
      player_stop();
      break;
      
    case STATE_RECORDING:
      recorder_stop();
      break;
  }
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
  //onboard_led_toggle();
  
  if (!app.key_event_flag)
  {
    /* Capture a new keyboard event if the previous one has
       been acknowledged */
    keyboard_update(&app.key_event);
    app.key_event_flag = 1;
  }
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

void action_next_bank(void)
{
  if (app.bank >= NB_BANKS)
    app.bank = 0;
  else
    app.bank++;
    
  printf("Switch to bank %u\r\n", app.bank);
}

void end_of_record(void)
{
  printf_P(PSTR("End of record\r\n"));
  
  recorder_stop();
  set_next_state(STATE_IDLE);
}

void action_start_record(uint8_t slot)
{
  printf_P(PSTR("Start recording...\r\n"));
  
  if (app.state != STATE_IDLE)
    stop_all();
  
  /* Start the recording */
  recorder_start(get_start_block(app.bank, slot), get_slot_size(app.bank, slot), &end_of_record);
}

void action_stop_record(void)
{
  recorder_stop();
  
  printf("Record stopped.\r\n");
}

void end_of_playback(void)
{
  printf_P(PSTR("End of playback\r\n"));
  
  player_stop();
  set_next_state(STATE_IDLE);
}

void action_start_play(uint8_t slot)
{
  printf_P(PSTR("Start playing...\r\n"));

  if (app.state != STATE_IDLE)
    stop_all();

  /* Start the playback */
  player_start(get_start_block(app.bank, slot), get_slot_size(app.bank, slot), &end_of_playback);
}

uint8_t handle_idle_state(void)
{
  uint8_t next_state = STATE_SAME;
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_PRESSED)
    {
      uint8_t keycode = app.key_event & KEYCODE_MASK;
      
      printf("R %i\r\n", keycode);

      switch(keycode)
      {
        case KEYCODE_SW0:
          action_next_bank();
          next_state = STATE_SAME;
          break;
          
        case KEYCODE_SW1:
          printf("Wait for slot selection...\r\n");
          PORTD |= _BV(PORTD6);
          next_state = STATE_RECORD_SELECT_SLOT;
          break;
          
        case KEYCODE_1:
        case KEYCODE_2:
        case KEYCODE_3:
          action_start_play(keycode - KEYCODE_1);
          next_state = STATE_PLAYING;
          break;
      }
    }
    
    /* Acknowledged the event */
    app.key_event_flag = 0;
  }
  
  return next_state;
}

uint8_t handle_record_select_slot(void)
{
  uint8_t next_state = STATE_SAME;
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_PRESSED)
    {
      uint8_t keycode = app.key_event & KEYCODE_MASK;
      
      printf("R %i\r\n", keycode);

      switch(keycode)
      {
        case KEYCODE_1:
        case KEYCODE_2:
        case KEYCODE_3:
        case KEYCODE_4:
        case KEYCODE_5:
        case KEYCODE_6:
        case KEYCODE_7:
        case KEYCODE_8:
        case KEYCODE_9:
          PORTD &= ~_BV(PORTD6);
          action_start_record(keycode - KEYCODE_1);
          next_state = STATE_RECORDING;
          break;
          
        default:
          printf("Record cancelled\r\n");
          next_state = STATE_IDLE;
          break;
      }
    }
    
    /* Acknowledged the event */
    app.key_event_flag = 0;
  }
  
  return next_state;
}

uint8_t handle_recording(void)
{
  uint8_t next_state = STATE_SAME;
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_RELEASED)
    {
      action_stop_record();
      next_state = STATE_IDLE;
    }
    
    /* Acknowledged the event */
    app.key_event_flag = 0;
  }
  
  return next_state;
}

void set_next_state(uint8_t next_state)
{
  if (next_state != STATE_SAME)
    app.state = next_state;
}

int application_main(void)
{
  hardware_init();

  /* Init the application state */
  app.state = STATE_IDLE;
  app.bank = 0;
  app.key_event_flag = 0;
  app.key_event = 0;

  /* Mainloop */
  while(1)
  {
    uint8_t next_state;
    
    switch(app.state)
    {
      case STATE_IDLE:
        next_state = handle_idle_state();
        set_next_state(next_state);
        break;
        
      case STATE_RECORD_SELECT_SLOT:
        next_state = handle_record_select_slot();
        set_next_state(next_state);
        break;
        
      case STATE_RECORDING:
        next_state = handle_recording();
        set_next_state(next_state);
        break;
    }

  }

  return 0;
}


