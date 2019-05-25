#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/////////////////////////////////////////////////////////////////////////////////////////////
/* Debug level masks */
#define AOM_DPRINTF_INFO_NULL         0x00000000  //No dump
#define AOM_DPRINTF_INFO_SEQ          0x00000001  //Sequence Header
#define AOM_DPRINTF_INFO_UDC          0x00000002  //Uncompressed Data Chunk
#define AOM_DPRINTF_INFO_CFH          0x00000004  //Compressed Frame Header
#define AOM_DPRINTF_INFO_TIL          0x00000008  //Tiles
#define AOM_DPRINTF_INFO_LSB          0x00000010  //LargestSuperBlock
#define AOM_DPRINTF_INFO_SB           0x00000020  //SuperBlock
#define AOM_DPRINTF_INFO_SBH          0x00000040  //SuperBlockHeader
#define AOM_DPRINTF_INFO_SBD          0x00000080  //SuperBlockData
#define AOM_DPRINTF_INFO_TOKEN        0x00000100  //
#define AOM_DPRINTF_INFO_COEF         0x00000200  //
#define AOM_DPRINTF_INFO_RESIDUE      0x00000400  //
#define AOM_DPRINTF_INFO_PRED         0x00000800  //
#define AOM_DPRINTF_INFO_RECON        0x00001000  //
#define AOM_DPRINTF_INFO_LOOP         0x00002000  //
#define AOM_DPRINTF_INFO_CDEF         0x00004000  //
#define AOM_DPRINTF_INFO_FINAL        0x00008000  //

/* debug level for this library */
#define AOM_DPRINTF_LEVEL

#ifdef __cplusplus
extern "C" {
#endif

  int   get_trace_level();
  void  set_trace_level(int level);
  char* get_trace_dump();
  void  set_trace_dump(const char *outfile_pattern);
  void  dprintf_yuv(int level, unsigned char*  img, int width, int height, int stride);

  static void AOM_DPRINTF(int level, const char *fmt, ...)
  {
#ifdef AOM_DPRINTF_LEVEL
    if (get_trace_level() & level) {
      va_list args;
      va_start(args, fmt);
      vfprintf(stderr, fmt, args);
      va_end(args);
    }
#endif
  }

#ifdef __cplusplus
}
#endif

#endif // _TRACE_H_
