/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>

#include "common/rawenc.h"
#include "common/y4menc.h"

// Returns the Y4M name associated with the monochrome colorspace.
const char *monochrome_colorspace(unsigned int bit_depth) {
  switch (bit_depth) {
    case 8: return "Cmono";
    case 9: return "Cmono9";
    case 10: return "Cmono10";
    case 12: return "Cmono12";
    case 16: return "Cmono16";
    default: assert(0); return NULL;
  }
}

// Return the Y4M name of the colorspace, given the bit depth and image format.
const char *colorspace(unsigned int bit_depth, aom_img_fmt_t fmt) {
  switch (bit_depth) {
    case 8:
      return fmt == AOM_IMG_FMT_444A
                 ? "C444alpha"
                 : fmt == AOM_IMG_FMT_I444
                       ? "C444"
                       : fmt == AOM_IMG_FMT_I422 ? "C422" : "C420jpeg";
    case 9:
      return fmt == AOM_IMG_FMT_I44416
                 ? "C444p9 XYSCSS=444P9"
                 : fmt == AOM_IMG_FMT_I42216 ? "C422p9 XYSCSS=422P9"
                                             : "C420p9 XYSCSS=420P9";
    case 10:
      return fmt == AOM_IMG_FMT_I44416
                 ? "C444p10 XYSCSS=444P10"
                 : fmt == AOM_IMG_FMT_I42216 ? "C422p10 XYSCSS=422P10"
                                             : "C420p10 XYSCSS=420P10";
    case 12:
      return fmt == AOM_IMG_FMT_I44416
                 ? "C444p12 XYSCSS=444P12"
                 : fmt == AOM_IMG_FMT_I42216 ? "C422p12 XYSCSS=422P12"
                                             : "C420p12 XYSCSS=420P12";
    case 14:
      return fmt == AOM_IMG_FMT_I44416
                 ? "C444p14 XYSCSS=444P14"
                 : fmt == AOM_IMG_FMT_I42216 ? "C422p14 XYSCSS=422P14"
                                             : "C420p14 XYSCSS=420P14";
    case 16:
      return fmt == AOM_IMG_FMT_I44416
                 ? "C444p16 XYSCSS=444P16"
                 : fmt == AOM_IMG_FMT_I42216 ? "C422p16 XYSCSS=422P16"
                                             : "C420p16 XYSCSS=420P16";
    default: assert(0); return NULL;
  }
}

int y4m_write_file_header(char *buf, size_t len, int width, int height,
                          const struct AvxRational *framerate, int monochrome,
                          aom_img_fmt_t fmt, unsigned int bit_depth) {
  const char *color = monochrome ? monochrome_colorspace(bit_depth)
                                 : colorspace(bit_depth, fmt);
  return snprintf(buf, len, "YUV4MPEG2 W%u H%u F%u:%u I%c %s\n", width, height,
                  framerate->numerator, framerate->denominator, 'p', color);
}

int y4m_write_frame_header(char *buf, size_t len) {
  return snprintf(buf, len, "FRAME\n");
}

void y4m_write_image_file(const aom_image_t *img, const int *planes,
                          FILE *file) {
  int num_planes = img->monochrome ? 1 : 3;
  raw_write_image_file(img, planes, num_planes, file);
}

void y4m_update_image_md5(const aom_image_t *img, const int *planes,
                          MD5Context *md5) {
  int num_planes = img->monochrome ? 1 : 3;
  raw_update_image_md5(img, planes, num_planes, md5);
}
