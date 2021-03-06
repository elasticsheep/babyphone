/*
  Copyright 2010  Mathieu SONET (contact [at] elasticsheep [dot] com)

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
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "sd_raw.h"

#include "player.h"
#include "interrupts.h"
#include "buffer.h"
#include "dac.h"

/*****************************************************************************
* Globals
******************************************************************************/
struct {
  t_notify_eof notify_eof;
  uint32_t start_sector;
  uint32_t current_sector;
  uint32_t end_sector;
  uint8_t eof;
} player;

struct {
  uint16_t sampling_rate;
  uint8_t loop_mode;
} player_options;

/*****************************************************************************
* Local prototypes
******************************************************************************/
void buffer_empty_handler(void);

/*****************************************************************************
* Functions
******************************************************************************/

void player_init(void)
{
  /* Reset the player context and options */
  memset(&player, 0x00, sizeof(player));
  memset(&player_options, 0x00, sizeof(player_options));
}

void player_set_option(uint8_t option, uint32_t value)
{
  switch(option)
  {
    case PLAYER_OPTION_SAMPLING_RATE:
      player_options.sampling_rate = (uint16_t)value;
      break;
      
    case PLAYER_OPTION_LOOP_MODE:
      player_options.loop_mode = (value > 0 ? 1 : 0);
      break;
  }
}

void player_start(uint32_t start_sector, uint16_t nb_sectors, t_notify_eof notify_eof)
{
  /* Init the player context */
  player.start_sector = start_sector;
  player.current_sector = start_sector;
  player.end_sector = start_sector + nb_sectors;
  player.eof = 0;
  player.notify_eof = notify_eof;

  /* Init the DAC */
  if (player_options.sampling_rate == 0)
    dac_init(8000);
  else
    dac_init(player_options.sampling_rate);

  /* Do some pre-buffering */
  sd_raw_read(player.current_sector << 9, pcm_buffer, PCM_BUFFER_SIZE * 2);
  player.current_sector += 2;

  /* Set the buffer event handler */
  set_buffer_event_handler(&buffer_empty_handler);

  /* Start the DAC */
  dac_start(pcm_buffer, pcm_buffer + PCM_BUFFER_SIZE, PCM_BUFFER_SIZE);
}

void player_stop(void)
{
  dac_stop();
  
  /* Reset the buffer event handler */
  set_buffer_event_handler(NULL);
  
  /* Reset the context */
  player.current_sector = 0;
  player.end_sector = 0;
  player.eof = 0;
  player.notify_eof = NULL;
}

void buffer_empty_handler(void)
{
  uint8_t* p;
  uint32_t rd_address = player.current_sector << 9;
  
  if (empty_buffer_flag)
  {
      //printf("E");
    
      p = (uint8_t*)pcm_buffer + (empty_buffer_flag & 0x1) * PCM_BUFFER_SIZE;
      sd_raw_read(rd_address, p, PCM_BUFFER_SIZE);
      player.current_sector++;
      
      /* Reset the flag */
      empty_buffer_flag = 0;
      
      /* Detect end of file */
      if ((player.eof == 0) && (player.current_sector >= player.end_sector))
      {
        if (player_options.loop_mode)
        {
          player.current_sector = player.start_sector;
        }
        else
        {
          player.eof = 1;

          /* Stop the dac */
          dac_stop();

          /* Notify the client */
          if (player.notify_eof)
            player.notify_eof();
        }
      }
      
      //printf("\r\n");
  }
}