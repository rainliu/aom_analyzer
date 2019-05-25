#include "trace.h"

static int g_trace_level = 0;
static char g_trace_dump[1024];

int get_trace_level(){
  return g_trace_level;
}

void set_trace_level(int level){
  g_trace_level = level;
}

char* get_trace_dump(){
  return g_trace_dump;
}

void set_trace_dump(const char *outfile_pattern) {
  sprintf(g_trace_dump, "%s.dump", outfile_pattern);
}

void dprintf_yuv(int level, unsigned char* img, int width, int height, int stride)
{
  for (int j = 0; j < height; j++){
    for (int i = 0; i < width; i++){
      AOM_DPRINTF(level, "%02x ", img[j*stride + i]);
    }
    AOM_DPRINTF(level, "\n");
  }
}
