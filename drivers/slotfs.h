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

uint8_t slotfs_init(void);

uint32_t slotfs_get_nb_partitions(void);
uint32_t slotfs_get_partition_start(uint8_t partition);

void slotfs_get_partition_info(uint8_t partition, uint16_t *sampling_rate, uint8_t* nb_slots);
void slotfs_get_slot_info(uint8_t partition, uint8_t slot, uint32_t *start_block, uint16_t *max_content_blocks, uint16_t *nb_slot_blocks);

void slotfs_update_slot_content_size(uint8_t partition, uint8_t slot, uint16_t nb_content_blocks);
