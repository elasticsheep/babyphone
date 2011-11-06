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

/*****************************************************************************
* Constants
******************************************************************************/
#define MAX_PARTITIONS (3)  /* Max in partition table = 62 */
#define MAX_SLOTS      (12) /* Max in partition header = 62 */

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/

struct {
  uint8_t no_partitions;
  uint8_t nb_partitions;
} slotfs;

/*****************************************************************************
* Local prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

uint8_t slotfs_init(void)
{
  char buffer[10 + 1];
  uint32_t nb_bytes = 0;
  
  memset(&slotfs, 0x00, sizeof(slotfs));
  
  printf_P(PSTR("slotfs_init\r\n"));
  
  nb_bytes = sd_raw_read(0, (uint8_t*)buffer, 10);
  if (strncmp(buffer, "SLOTFS", 6) == 0)
  {
    printf_P(PSTR("SLOTFS\r\n"));
    
    /* Only one slotfs, without partition */
    slotfs.no_partitions = 1;
  }
  else if (strncmp(buffer, "PARTITIONS", 10) == 0)
  {
    uint8_t i;
    uint32_t start_block;

    printf_P(PSTR("PARTITIONS\r\n"));
        
    slotfs.no_partitions = 0;
        
    /* Read the partition table */
    for(i = 0; i < MAX_PARTITIONS; i++)
    {
      sd_raw_read(16 + i * 8, (uint8_t*)&start_block, 4);
      
      if(start_block == 0)
      {
        slotfs.nb_partitions = i;
        break;
      }
      else
      {
        printf_P(PSTR("Partition %i start %li\r\n"), i, start_block);
      }
    }
  }
  else
  {
    return 0;
  }
  
  return 1;
}

uint32_t slotfs_get_nb_partitions(void)
{
  if (slotfs.no_partitions == 1)
  {
    return 1;
  }
  
  return slotfs.nb_partitions;
}

uint32_t slotfs_get_partition_start(uint8_t partition)
{
  uint32_t start_block;
  
  if (slotfs.no_partitions == 1)
  {
    return 0;
  }

  /* Read the partition table */
  sd_raw_read(16 + partition * 8, (uint8_t*)&start_block, 4);
  
  return start_block;
}

void slotfs_get_partition_info(uint8_t partition, uint16_t *sampling_rate, uint8_t* nb_slots)
{
  uint8_t i;
  uint32_t partition_address = slotfs_get_partition_start(partition) * 512;
  uint32_t start_block;
  
  if (sampling_rate)
  {
    sd_raw_read(partition_address + 8, (uint8_t*)sampling_rate, 2);
  }
  
  if (nb_slots)
  {
    for(i = 0; i < MAX_SLOTS; i++)
    {
      sd_raw_read(partition_address + 16 + i * 8, (uint8_t*)&start_block, 4);
      
      if(start_block == 0)
      {
        *nb_slots = i;
        break;
      }
#ifdef DEBUG
      else
      {
        printf_P(PSTR("Slot %i start %li\r\n"), i, start_block);
      }
#endif
    }
  }
}

void slotfs_get_slot_info(uint8_t partition, uint8_t slot, uint32_t *start_block, uint16_t *max_content_blocks, uint16_t *nb_slot_blocks)
{
  uint32_t partition_address = slotfs_get_partition_start(partition) * 512;
  uint32_t slot_entry_address = partition_address + 16 + slot * 8;
  uint32_t slot_offset = 0;
  
  if (start_block)
  {
    sd_raw_read(slot_entry_address, (uint8_t*)&slot_offset, 4);
    *start_block = slotfs_get_partition_start(partition) + slot_offset;
  }
  
  if (max_content_blocks)
    sd_raw_read(slot_entry_address + 4, (uint8_t*)max_content_blocks, 2);
  
  if (nb_slot_blocks)
    sd_raw_read(slot_entry_address + 6, (uint8_t*)nb_slot_blocks, 2);
}

void slotfs_update_slot_content_size(uint8_t partition, uint8_t slot, uint16_t nb_content_blocks)
{
  uint32_t partition_address = slotfs_get_partition_start(partition) * 512;
  uint32_t slot_entry_address = partition_address + 16 + slot * 8;

  sd_raw_write(slot_entry_address + 6, (uint8_t*)&nb_content_blocks, 2);
  sd_raw_sync();
}