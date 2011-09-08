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

#include "player.h"
#include "recorder.h"

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
typedef struct {
  uint8_t count;
  uint8_t value;
} t_debouncer;

/*****************************************************************************
* Globals
******************************************************************************/
struct {
  uint8_t state;
} app;

/*****************************************************************************
* Function prototypes
******************************************************************************/
void debouncer_init(t_debouncer* debouncer);
uint8_t debouncer_update(t_debouncer* debouncer, uint8_t value);

#if 0
void shuffle_player_eof(void);
void play_file(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name);

uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry);
static struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name);
#endif

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

int application_main(void)
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

  /* Init the IO pins */
  DDRB &= ~_BV(PORTB4) | ~_BV(PORTB5);
  PORTB |= _BV(PORTB4) | _BV(PORTB5);  
  
  /* Init the debouncers */
  t_debouncer debouncer_sw0;
  t_debouncer debouncer_sw1;
  
  debouncer_init(&debouncer_sw0);
  debouncer_init(&debouncer_sw1);

  app.state = STATE_IDLE;

  /* Switches interaction loop */
  while(1)
  {
    delay_ms(10);

    if (debouncer_update(&debouncer_sw0, PINB & _BV(PORTB4)))
    {
      if (debouncer_sw0.value == 0)
      {
        printf("SW0 pressed\r\n");
        play(0, 64);
      }
      else
      {
        printf("SW0 released\r\n");
      }
    }
  
    if (debouncer_update(&debouncer_sw1, PINB & _BV(PORTB5)))
    {
      if (debouncer_sw1.value == 0)
      {
        printf("SW1 pressed\r\n");
        record(0, 64);
      }
      else
      {
        printf("SW1 released\r\n");
      }
    }
  }

  return 0;
}

void debouncer_init(t_debouncer* debouncer)
{
  memset(debouncer, 0x00, sizeof(t_debouncer));
  debouncer->count = 0;
  debouncer->value = 1;
}

uint8_t debouncer_update(t_debouncer* debouncer, uint8_t value)
{
  if (value != debouncer->value)
  {
    debouncer->count++;

    if (debouncer->count > 3)
    {
      debouncer->count = 0;
      debouncer->value = value;
      return 1;
    }
  }
  else
  {
    debouncer->count = 0;
  }

  return 0;
}

