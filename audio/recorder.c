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
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "fat.h"
#include "sd_raw.h"

#include "recorder.h"
#include "interrupts.h"
#include "buffer.h"
#include "adc.h"
#include "delay.h"

/*****************************************************************************
* Globals
******************************************************************************/
struct {
  t_recorder_notify_eof notify_eof;
  uint32_t start_sector;
  uint32_t current_sector;
  uint32_t end_sector;
  uint8_t eof;
  uint8_t loop_mode;
} recorder;

/*****************************************************************************
* Local prototypes
******************************************************************************/
void buffer_full_handler(void);

/*****************************************************************************
* Functions
******************************************************************************/

void recorder_start(uint32_t start_sector, uint16_t nb_sectors, t_recorder_notify_eof notify_eof)
{
  /* Init the recorder context */
  recorder.start_sector = start_sector;
  recorder.current_sector = start_sector;
  recorder.end_sector = start_sector + nb_sectors;
  recorder.notify_eof = notify_eof;
  recorder.eof = 0;
  
  recorder.loop_mode = 0;
  
  /* Init the ADC */
  adc_init(1);

  /* Set the buffer event handler */
  set_buffer_event_handler(&buffer_full_handler);

  /* Start the ADC */
  adc_start(pcm_buffer, pcm_buffer + PCM_BUFFER_SIZE, PCM_BUFFER_SIZE);
}

void recorder_stop(void)
{
  /* Stop and shutdown the ADC */
  adc_stop();
  adc_shutdown();
  
  /* Pad the remaining sectors with silence */
  memset(pcm_buffer, 0x00, PCM_BUFFER_SIZE);
  for(uint32_t sector = recorder.current_sector; sector < recorder.end_sector; sector++)
  {
    uint32_t wr_address = sector << 9;
    sd_raw_write(wr_address, pcm_buffer, PCM_BUFFER_SIZE);
  }
  
  /* Reset the buffer event handler */
  set_buffer_event_handler(NULL);
  
  /* Reset the context */
  recorder.current_sector = 0;
  recorder.end_sector = 0;
}

void buffer_full_handler(void)
{
  uint8_t* p;
  uint32_t wr_address = recorder.current_sector << 9;
  
  if (buffer_full_flag)
  {
      //printf("F");
      
      /* Write data in raw sector */
      p = (uint8_t*)pcm_buffer + (buffer_full_flag & 0x1) * PCM_BUFFER_SIZE;
      sd_raw_write(wr_address, p, PCM_BUFFER_SIZE);
      recorder.current_sector++;
      
      /* Reset the flag */
      buffer_full_flag = 0;
      
      /* Detect end of file */
      if ((recorder.eof == 0) && (recorder.current_sector >= recorder.end_sector))
      {
        if (recorder.loop_mode)
        {
          recorder.current_sector = recorder.start_sector;
        }
        else
        {
          recorder.eof = 1;

          /* Stop the adc */
          adc_stop();

          /* Notify the client */
          if (recorder.notify_eof)
            recorder.notify_eof();
        }
      }
      
      //printf("\r\n");
  }
}