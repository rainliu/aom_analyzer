##
## Copyright (c) 2017, Alliance for Open Media. All rights reserved
##
## This source code is subject to the terms of the BSD 2 Clause License and
## the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
## was not distributed with this source code in the LICENSE file, you can
## obtain it at www.aomedia.org/license/software. If the Alliance for Open
## Media Patent License 1.0 was not distributed with this source code in the
## PATENTS file, you can obtain it at www.aomedia.org/license/patent.
##
set(AOM_UTIL_SOURCES
    "${AOM_ROOT}/aom_util/aom_thread.c"
    "${AOM_ROOT}/aom_util/aom_thread.h"
    "${AOM_ROOT}/aom_util/endian_inl.h")

if (CONFIG_BITSTREAM_DEBUG)
  set(AOM_UTIL_SOURCES
      ${AOM_UTIL_SOURCES}
      "${AOM_ROOT}/aom_util/debug_util.c"
      "${AOM_ROOT}/aom_util/debug_util.h")
endif ()

# Creates the aom_util build target and makes libaom depend on it. The libaom
# target must exist before this function is called.
function (setup_aom_util_targets)
  add_library(aom_util OBJECT ${AOM_UTIL_SOURCES})
  set(AOM_LIB_TARGETS ${AOM_LIB_TARGETS} aom_util PARENT_SCOPE)
  target_sources(aom PUBLIC $<TARGET_OBJECTS:aom_util>)
endfunction ()
