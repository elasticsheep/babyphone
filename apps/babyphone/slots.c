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
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "sd_raw.h"
#include "buffer.h"

/*****************************************************************************
* Constants
******************************************************************************/
#define NB_BANKS (3)

#define SLOT_NB_BLOCKS (64)
#define BANK_NB_SLOTS  (9)

#define BANK_SIZE      (1 + BANK_NB_SLOTS * SLOT_NB_BLOCKS)

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/

/*****************************************************************************
* Local prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

uint32_t get_nb_banks(void)
{
  return NB_BANKS;
}

uint32_t get_start_block(uint8_t bank, uint8_t slot)
{
  return BANK_SIZE * bank + 1 + (slot * SLOT_NB_BLOCKS);
}

uint32_t get_content_size(uint8_t bank, uint8_t slot)
{
  uint32_t content_size;
  uint32_t rd_address = (BANK_SIZE * (uint32_t)bank) * 512 + 4 * (uint32_t)slot;
  
  printf("0x%08lx\r\n", rd_address);
  sd_raw_read(rd_address, &content_size, 4);
  
  if ((content_size > 0) && (content_size <= SLOT_NB_BLOCKS))
    return content_size;
    
  return SLOT_NB_BLOCKS;
}

void write_content_size(uint8_t bank, uint8_t slot, uint32_t content_size)
{
  uint32_t wr_address = (BANK_SIZE * (uint32_t)bank) * 512 + 4 * (uint32_t)slot;
  
  sd_raw_write(wr_address, (const uint8_t*)&content_size, 4);
  sd_raw_sync();
}

void reset_banks(void)
{
  uint32_t i;
  
  memset(pcm_buffer, 0x00, PCM_BUFFER_SIZE);
  
  for(i= 0; i < (BANK_SIZE * NB_BANKS); i++)
  {
    uint32_t wr_address = i * 512;
    printf("0x%08lx\r\n", wr_address);
    sd_raw_write(wr_address, pcm_buffer, 512);
  }
}