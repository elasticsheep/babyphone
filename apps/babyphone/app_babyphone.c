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
#include <avr/eeprom.h> 

#include "player.h"
#include "recorder.h"

#include "sd_raw.h"
#include "keyboard.h"
#include "leds.h"
#include "slotfs.h"

#include "delay.h"
#include "LUFA/Drivers/Peripheral/SerialStream.h"

/*****************************************************************************
* Constants
******************************************************************************/
enum state {
  STATE_IDLE,
  STATE_RECORD_SELECT_SLOT,
  STATE_PLAYING,
  STATE_RECORDING,
  
  STATE_SAME = 0xFF,
};

enum event {
  EVENT_NONE,
  EVENT_END_OF_RECORD,
  EVENT_END_OF_PLAYBACK,
};

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/
volatile struct {
  uint8_t state;
  uint8_t partition;
  
  uint8_t key_event_flag;
  uint8_t key_event;
  
  uint8_t media_event_flag;
  uint8_t media_event;
} app;

uint8_t EEMEM NonVolatilePartition; 

/*****************************************************************************
* Function prototypes
******************************************************************************/
void set_next_state(uint8_t next_state);

/*****************************************************************************
* Functions
******************************************************************************/

void stop_all(void)
{
  switch(app.state)
  {
    case STATE_PLAYING:
      player_stop();
      break;
      
    case STATE_RECORDING:
      recorder_stop(NULL);
      break;
  }
}

void init_keyboard_polling(void)
{
  /* Init the keyboard polling timer */
  TCCR1B = _BV(WGM12); /* CTC mode */

#if (F_CPU == 16000000)
  TCCR1B |= _BV(CS12); /* Fclk / 256 */
  OCR1A = (uint16_t)(625 - 1); /* 100 Hz */
#else
#error F_CPU not supported
#endif

  /* Enable interrupt on channel A */
  TIMSK1 |= _BV(OCIE1A); 
}

/* Keyboard polling interrupt */
ISR(TIMER1_COMPA_vect)
{
  uint8_t key_event;
  
  if (!app.key_event_flag)
  {
    PORTD |= _BV(PORTD6);
    
    /* Capture a new keyboard event if the previous one has
       been acknowledged */
    keyboard_update(&key_event);
    
    if (key_event)
    {
      printf("K");
      app.key_event = key_event;
      app.key_event_flag = 1;
    }
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
  
  leds_init();
  keyboard_init(2);
  init_keyboard_polling();
  
  /* Enable interrupts */
  sei();
}

void action_select_partition(uint8_t partition)
{
  if (app.partition >= slotfs_get_nb_partitions())
    app.partition = 0;

  /* Use the new value */
  app.partition = partition;
  
  /* Update the EEPROM */
  eeprom_write_byte(&NonVolatilePartition, app.partition);
    
  printf_P(PSTR("Switch to partition %u\r\n"), app.partition);
}

void end_of_record(void)
{
  printf_P(PSTR("End of record\r\n"));

  app.media_event = EVENT_END_OF_RECORD;
  app.media_event_flag = 1;
}

void action_start_record(uint8_t slot)
{
  printf_P(PSTR("Start recording...\r\n"));
  
  if (app.state != STATE_IDLE)
    stop_all();
  
  /* Start the recording */
  //printf("start block = %lu\r\n", get_start_block(app.bank, slot));
  //recorder_start(get_start_block(app.bank, slot), 64, &end_of_record);
}

void action_stop_record(void)
{
  //recorder_stop();
  
  printf("Record stopped.\r\n");
}

void end_of_playback(void)
{
  printf_P(PSTR("End of playback\r\n"));
  
  app.media_event = EVENT_END_OF_PLAYBACK;
  app.media_event_flag = 1;
}

void action_start_play(uint8_t partition, uint8_t slot)
{
  uint32_t start_block;
  uint16_t content_blocks;
  uint16_t sampling_rate = 0;
  
  if (app.state != STATE_IDLE)
    stop_all();

  printf_P(PSTR("Play partition %u slot %u\r\n"), partition, slot);

  /* Read content info */
  slotfs_get_partition_info(partition, &sampling_rate, NULL);
  slotfs_get_slot_info(partition, slot, &start_block, NULL, &content_blocks);

  printf_P(PSTR("Sampling rate = %u\r\n"), sampling_rate);
  printf_P(PSTR("Start block = %lu\r\n"), start_block);
  printf_P(PSTR("Content blocks = %u\r\n"), content_blocks);

  if (content_blocks > 0)
  {
    /* Start the playback */
    printf_P(PSTR("Start playing...\r\n"));
    
    player_set_option(PLAYER_OPTION_SAMPLING_RATE, sampling_rate);
    player_set_option(PLAYER_OPTION_LOOP_MODE, 0);
    
    player_start(start_block, content_blocks, &end_of_playback);
  }
  else
  {
    printf_P(PSTR("Empty slot\r\n"));
  }
}

uint8_t handle_idle_state(void)
{
  uint8_t next_state = STATE_SAME;
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_PRESSED)
    {
      uint8_t keycode = app.key_event & KEYCODE_MASK;
      printf("P %i\r\n", keycode);

      switch(keycode)
      {
        case KEYCODE_M1:
          action_select_partition(0);
          next_state = STATE_SAME;
          break;
          
        case KEYCODE_M2:
          action_select_partition(1);
          next_state = STATE_SAME;
          break;
          
        case KEYCODE_1:
        case KEYCODE_2:
        case KEYCODE_3:
        case KEYCODE_4:
        case KEYCODE_5:
        case KEYCODE_6:
        case KEYCODE_7:
        case KEYCODE_8:
        case KEYCODE_9:
        case KEYCODE_STAR:
        case KEYCODE_0:
        case KEYCODE_SHARP:
          action_start_play(app.partition, keycode - KEYCODE_1);
          next_state = STATE_PLAYING;
          break;
      }
    }
  }
  
  return next_state;
}

uint8_t handle_playing_state(void)
{
  uint8_t next_state;
  
  if (app.media_event_flag)
  {
    player_stop();
    return STATE_IDLE;
  }
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_PRESSED)
    {
      player_stop();
      
      next_state = handle_idle_state();
      if (next_state == STATE_SAME)
        return STATE_IDLE;
    }
  }
  
  return STATE_SAME;
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
  }
  
  return next_state;
}

uint8_t handle_recording(void)
{
  uint8_t next_state = STATE_SAME;
  
  if (app.media_event_flag)
  {
    recorder_stop(NULL);
    return STATE_IDLE;
  }
  
  if (app.key_event_flag)
  {
    if (app.key_event & EVENT_KEY_RELEASED)
    {
      action_stop_record();
      next_state = STATE_IDLE;
    }
  }
  
  return next_state;
}

void set_next_state(uint8_t next_state)
{
  if (next_state != STATE_SAME)
  {
    printf("State %i => %i\r\n", app.state, next_state);
    app.state = next_state;
  }
}

void init_from_eeprom(void)
{
  uint8_t partition = eeprom_read_byte(&NonVolatilePartition);
  
  if (partition > 3)
    app.partition = 0;
  else
    app.partition = partition;
}

int application_main(void)
{
  hardware_init();

  /* Init the filesystem */
  slotfs_init();

  /* Init the player */
  player_init();

  /* Switch on the green led */
  leds_set(LED_GREEN, 1);

  /* Init the application state */
  app.state = STATE_IDLE;
  app.partition = 0;

  app.key_event_flag = 0;
  app.key_event = 0;
  app.media_event_flag = 0;
  app.media_event = 0;
  
  init_from_eeprom();

  /* Mainloop */
  while(1)
  {
    uint8_t next_state;
    
    if ((app.key_event_flag) || (app.media_event_flag))
    {
      /* Handle events */
      switch(app.state)
      {
        case STATE_IDLE:
          next_state = handle_idle_state();
          set_next_state(next_state);
          break;

        case STATE_PLAYING:
          next_state = handle_playing_state();
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

      /* Acknowledge the events */
      app.key_event = 0;
      app.key_event_flag = 0;
      
      app.media_event = 0;
      app.media_event_flag = 0;
    }
  }

  return 0;
}


