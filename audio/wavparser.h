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

#if !defined(WAVPARSER_H)
#define WAVPARSER_H

#include <stdint.h>
#include <stdio.h>

#include "fat.h"

typedef enum {
  WAVPARSER_OK,
  WAVPARSER_INVALID_PARAMETER,
  WAVPARSER_INVALID_CHUNK_ID,
  WAVPARSER_INVALID_FORMAT,
  WAVPARSER_UNSUPPORTED_AUDIO_FORMAT,
} t_wavparser_status;

typedef struct {
  uint32_t data_start;
  uint32_t data_size;
  uint16_t sample_rate;
  uint8_t  num_channels;
  uint8_t  bits_per_sample;
} t_wavparser_header;

t_wavparser_status wavparser_parse_header(struct fat_file_struct* file, t_wavparser_header* header);

#endif /* WAVPARSER_H */