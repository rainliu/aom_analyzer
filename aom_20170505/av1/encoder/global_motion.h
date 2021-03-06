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

#ifndef AV1_ENCODER_GLOBAL_MOTION_H_
#define AV1_ENCODER_GLOBAL_MOTION_H_

#include "aom/aom_integer.h"
#include "aom_scale/yv12config.h"
#include "av1/common/mv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RANSAC_NUM_MOTIONS 1

void convert_model_to_params(const double *params, WarpedMotionParams *model);

int is_enough_erroradvantage(double erroradv, int params_cost);

double refine_integerized_param(WarpedMotionParams *wm,
                                TransformationType wmtype,
#if CONFIG_HIGHBITDEPTH
                                int use_hbd, int bd,
#endif  // CONFIG_HIGHBITDEPTH
                                uint8_t *ref, int r_width, int r_height,
                                int r_stride, uint8_t *dst, int d_width,
                                int d_height, int d_stride, int n_refinements);

/*
  Computes "num_motions" candidate global motion parameters between two frames.
  The array "params_by_motion" should be length 8 * "num_motions". The ordering
  of each set of parameters is best described  by the homography:

        [x'     (m2 m3 m0   [x
    z .  y'  =   m4 m5 m1 *  y
         1]      m6 m7 1)    1]

  where m{i} represents the ith value in any given set of parameters.

  "num_inliers" should be length "num_motions", and will be populated with the
  number of inlier feature points for each motion. Params for which the
  num_inliers entry is 0 should be ignored by the caller.
*/
int compute_global_motion_feature_based(
    TransformationType type, YV12_BUFFER_CONFIG *frm, YV12_BUFFER_CONFIG *ref,
#if CONFIG_HIGHBITDEPTH
    int bit_depth,
#endif
    int *num_inliers_by_motion, double *params_by_motion, int num_motions);
#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // AV1_ENCODER_GLOBAL_MOTION_H_
