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

/* Description of the WAVE soundfile format on:
      https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
*/

#include <string.h>
#include <avr/pgmspace.h>

#include "wavparser.h"
#include "fat.h"

#if defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)
#define DEBUG_TRACE(sTRING, pARAM)  printf_P(PSTR(sTRING), pARAM);
#else
#define DEBUG_TRACE(sTRING, pARAM)
#endif

t_wavparser_status wavparser_parse_header(struct fat_file_struct* fd, t_wavparser_header* header)
{
  char buffer[5] = { 0, 0, 0, 0, 0};
  uint32_t dword;
  uint16_t word;

  /* Check the header pointer */
  if (header == NULL)
  {
    return WAVPARSER_INVALID_PARAMETER;
  }

  /* Go the start of the file */
  int32_t offset = 0;
  fat_seek_file(fd, &offset, FAT_SEEK_SET);

  /* Read the chunk id */
  fat_read_file(fd, (uint8_t*)buffer, 4 * sizeof(char));
  DEBUG_TRACE("chunk_id = %s\n", buffer);
  if (strncmp(buffer, "RIFF", 4) != 0)
  {
    return WAVPARSER_INVALID_CHUNK_ID;
  }

  /* Read the chunk size */
  fat_read_file(fd, (uint8_t*)&dword, sizeof(dword));
  DEBUG_TRACE("chunk_size = %li\n", dword);

  /* Read the format */
  fat_read_file(fd, (uint8_t*)buffer, 4 * sizeof(char));
  DEBUG_TRACE("format = %s\n", buffer);
  if (strncmp(buffer, "WAVE", 4) != 0)
  {
    return WAVPARSER_INVALID_FORMAT;
  }

  /* Read the sub chunk id 1 */
  fat_read_file(fd, (uint8_t*)buffer, 4 * sizeof(char));
  DEBUG_TRACE("subchunk_id 1 = %s\n", buffer);
  if (strncmp(buffer, "fmt ", 4) != 0)
  {
    return WAVPARSER_INVALID_CHUNK_ID;
  }

  /* Read the sub chunk size */
  fat_read_file(fd, (uint8_t*)&dword, sizeof(dword));
  DEBUG_TRACE("subchunk_size = %li\n", dword);

  /* Read the audio parameters */
  fat_read_file(fd, (uint8_t*)&word, sizeof(word));
  DEBUG_TRACE("audio_format = %i\n", word);
  if (word != 1)
  {
    /* Only support PCM format */
    return WAVPARSER_UNSUPPORTED_AUDIO_FORMAT;
  }

  fat_read_file(fd, (uint8_t*)&word, sizeof(word));
  DEBUG_TRACE("num_channels = %i\n", word);
  header->num_channels = (uint8_t)word;

  fat_read_file(fd, (uint8_t*)&dword, sizeof(dword));
  DEBUG_TRACE("sample_rate = %li\n", dword);
  header->sample_rate = (uint16_t)dword;

  fat_read_file(fd, (uint8_t*)&dword, sizeof(dword));
  DEBUG_TRACE("byte_rate = %li\n", dword);

  fat_read_file(fd, (uint8_t*)&word, sizeof(word));
  DEBUG_TRACE("block_align = %i\n", word);

  fat_read_file(fd, (uint8_t*)&word, sizeof(word));
  DEBUG_TRACE("bits_per_sample = %i\n", word);
  header->bits_per_sample = (uint8_t)word;

  /* Read the sub chunk id 2 */
  fat_read_file(fd, (uint8_t*)buffer, 4 * sizeof(char));
  DEBUG_TRACE("subchunk_id 2 = %s\n", buffer);
  if (strncmp(buffer, "data", 4) != 0)
  {
    return WAVPARSER_INVALID_CHUNK_ID;
  }

  /* Read the sub chunk size */
  fat_read_file(fd, (uint8_t*)&dword, sizeof(dword));
  DEBUG_TRACE("subchunk_size = %li\n", dword);

  header->data_size = dword;
  fat_tell_file(fd, &header->data_start);

  return WAVPARSER_OK;
}