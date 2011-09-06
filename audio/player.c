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

/* sd-reader API */
#include "fat.h"

#include "player.h"
#include "wavparser.h"
#include "buffer.h"
#include "dac.h"
#include "delay.h"

/*****************************************************************************
* Constants
******************************************************************************/

/*****************************************************************************
* Definitions
******************************************************************************/
typedef struct {
  struct fat_file_struct* fd;
  uint8_t format;
  uint8_t eof;
  t_notify_eof notify_eof;
} t_player_context;

/*****************************************************************************
* Globals
******************************************************************************/
t_player_context player;
uint8_t scratch[64];

/*****************************************************************************
* Local prototypes
******************************************************************************/
void buffer_refill_handler(void);

/*****************************************************************************
* Functions
******************************************************************************/

inline intptr_t read_samples(struct fat_file_struct* fd, uint8_t* buffer, uintptr_t buffer_len, uint8_t format)
{
  if (format == FORMAT_16_BITS_LE)
  {
    uint16_t i;
    uint8_t* p = buffer;
    
    i = buffer_len / 32;
    do
    {
      if (fat_read_file(fd, (uint8_t*)&scratch, 64) < 64)
        break;

      *p++ = scratch[1] + 128;
      *p++ = scratch[3] + 128;
      *p++ = scratch[5] + 128;
      *p++ = scratch[7] + 128;
      *p++ = scratch[9] + 128;
      *p++ = scratch[11] + 128;
      *p++ = scratch[13] + 128;
      *p++ = scratch[15] + 128;
      *p++ = scratch[17] + 128;
      *p++ = scratch[19] + 128;
      *p++ = scratch[21] + 128;
      *p++ = scratch[23] + 128;
      *p++ = scratch[25] + 128;
      *p++ = scratch[27] + 128;
      *p++ = scratch[29] + 128;
      *p++ = scratch[31] + 128;
      
      *p++ = scratch[33] + 128;
      *p++ = scratch[35] + 128;
      *p++ = scratch[37] + 128;
      *p++ = scratch[39] + 128;
      *p++ = scratch[41] + 128;
      *p++ = scratch[43] + 128;
      *p++ = scratch[45] + 128;
      *p++ = scratch[47] + 128;
      *p++ = scratch[49] + 128;
      *p++ = scratch[51] + 128;
      *p++ = scratch[53] + 128;
      *p++ = scratch[55] + 128;
      *p++ = scratch[57] + 128;
      *p++ = scratch[59] + 128;
      *p++ = scratch[61] + 128;
      *p++ = scratch[63] + 128;
    } while (--i);
    
    return (p - buffer);
  }
  else
  {
    return fat_read_file(fd, buffer, buffer_len);
  }
  
  return 0;
}

void player_start(struct fat_file_struct* fd, t_notify_eof notify_eof)
{
  uint16_t size;

  /* Init the player context */
  player.fd = fd;
  player.eof = 0;
  player.notify_eof = notify_eof;

  /* Parse the file header */
  t_wavparser_status status;
  t_wavparser_header header;
  status = wavparser_parse_header(fd, &header);

  /* Init the DAC */
  uint16_t rate;
  uint8_t channels;
  
  switch(header.sample_rate)
  {
    case 8000:
    case 16000:
    case 22050:
    case 44100:
      rate = header.sample_rate;
      break;
      
    default:
      rate = 8000;
      break;
  }
  
  if (header.num_channels == 2)
    channels = CHANNELS_STEREO;
  else
    channels = CHANNELS_MONO;

  if (header.bits_per_sample == 16)
    player.format = FORMAT_16_BITS_LE;
  else
    player.format = FORMAT_8_BITS;

  dac_init(rate, channels);

  /* Go to the start of the data */
  int32_t offset = header.data_start;
  fat_seek_file(fd, &offset, FAT_SEEK_SET);

  /* Do some pre-buffering */
  if ((size = read_samples(player.fd, (uint8_t*)pcm_buffer, 2 * PCM_BUFFER_SIZE, player.format)) < (2 * PCM_BUFFER_SIZE))
  {
    printf_P(PSTR("Buffering error %i\r\n "), size);
    return; 
  }

  /* Set the buffer event handler */
  set_buffer_event_handler(&buffer_refill_handler);

  /* Start the DAC */
  dac_start(pcm_buffer, pcm_buffer + PCM_BUFFER_SIZE, PCM_BUFFER_SIZE);
}

void player_stop(void)
{
  dac_stop();
  
  /* Reset the buffer event handler */
  set_buffer_event_handler(NULL);
  
  /* Reset the context */
  player.fd = NULL;
}

void player_pause(void)
{
  dac_pause();
}

void player_resume(void)
{
  dac_resume();
}

/* Buffer refill handler */
void buffer_refill_handler(void)
{
  uint16_t size;
  uint8_t* p;
  
  if (empty_buffer_flag)
  {
    if (player.fd)
    {
      p = (uint8_t*)pcm_buffer + (empty_buffer_flag & 0x1) * PCM_BUFFER_SIZE;

      if ((size = read_samples(player.fd, p, PCM_BUFFER_SIZE, player.format)) < PCM_BUFFER_SIZE)
      {
        /* Stop the dac */
        dac_stop();
        
        /* Notify the client */
        player.eof = 1;
        if (player.notify_eof)
          player.notify_eof();
      }

      empty_buffer_flag = 0;
    }
  }
}