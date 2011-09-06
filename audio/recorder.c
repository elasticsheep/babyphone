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

/* sd-reader API */
#include "fat.h"

#include "recorder.h"
#include "buffer.h"
#include "adc.h"
#include "delay.h"

/*****************************************************************************
* Constants
******************************************************************************/

/*****************************************************************************
* Definitions
******************************************************************************/
typedef struct {
  struct fat_file_struct* fd;
} t_recorder_context;

/*****************************************************************************
* Globals
******************************************************************************/
t_recorder_context recorder;

/*****************************************************************************
* Local prototypes
******************************************************************************/
void buffer_emptying_handler(void);

/*****************************************************************************
* Functions
******************************************************************************/

void recorder_start(struct fat_file_struct* fd)
{
  uint16_t size;

  /* Init the recorder context */
  recorder.fd = fd;
  fat_write_file_prologue(recorder.fd);

  /* Init the ADC */
  adc_init();

  /* Set the buffer event handler */
  set_buffer_event_handler(&buffer_emptying_handler);

  /* Start the ADC */
  adc_start(pcm_buffer, pcm_buffer + PCM_BUFFER_SIZE, PCM_BUFFER_SIZE);
}

void recorder_stop(void)
{
  /* Stop and shutdown the ADC */
  adc_stop();
  adc_shutdown();
  
  /* Reset the buffer event handler */
  set_buffer_event_handler(NULL);
  
  /* End the write operation */
  fat_write_file_epilogue(recorder.fd);
  
  /* Reset the context */
  recorder.fd = NULL;
}

/* Buffer emptying handler */
void buffer_emptying_handler(void)
{
  uint16_t size;
  uint8_t* p;
  
  if (buffer_full_flag)
  {
    if (recorder.fd)
    {
      printf("F");
      
      /* Write data to the filesystem */
      p = (uint8_t*)pcm_buffer + (buffer_full_flag & 0x1) * PCM_BUFFER_SIZE;
      //size = fat_write_file_write(recorder.fd, p, PCM_BUFFER_SIZE);
      sd_raw_write(112000, p, PCM_BUFFER_SIZE);
      
      /* Reset the flag */
      buffer_full_flag = 0;
      
      printf("\r\n");
    }
  }
}