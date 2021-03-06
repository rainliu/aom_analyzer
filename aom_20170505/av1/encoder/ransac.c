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
#include <memory.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "av1/encoder/ransac.h"
#include "av1/encoder/mathutils.h"

#define MAX_MINPTS 4
#define MAX_DEGENERATE_ITER 10
#define MINPTS_MULTIPLIER 5

#define INLIER_THRESHOLD 1.0
#define MIN_TRIALS 20

////////////////////////////////////////////////////////////////////////////////
// ransac
typedef int (*IsDegenerateFunc)(double *p);
typedef void (*NormalizeFunc)(double *p, int np, double *T);
typedef void (*DenormalizeFunc)(double *params, double *T1, double *T2);
typedef int (*FindTransformationFunc)(int points, double *points1,
                                      double *points2, double *params);
typedef void (*ProjectPointsDoubleFunc)(double *mat, double *points,
                                        double *proj, const int n,
                                        const int stride_points,
                                        const int stride_proj);

static void project_points_double_translation(double *mat, double *points,
                                              double *proj, const int n,
                                              const int stride_points,
                                              const int stride_proj) {
  int i;
  for (i = 0; i < n; ++i) {
    const double x = *(points++), y = *(points++);
    *(proj++) = x + mat[0];
    *(proj++) = y + mat[1];
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void project_points_double_rotzoom(double *mat, double *points,
                                          double *proj, const int n,
                                          const int stride_points,
                                          const int stride_proj) {
  int i;
  for (i = 0; i < n; ++i) {
    const double x = *(points++), y = *(points++);
    *(proj++) = mat[2] * x + mat[3] * y + mat[0];
    *(proj++) = -mat[3] * x + mat[2] * y + mat[1];
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void project_points_double_affine(double *mat, double *points,
                                         double *proj, const int n,
                                         const int stride_points,
                                         const int stride_proj) {
  int i;
  for (i = 0; i < n; ++i) {
    const double x = *(points++), y = *(points++);
    *(proj++) = mat[2] * x + mat[3] * y + mat[0];
    *(proj++) = mat[4] * x + mat[5] * y + mat[1];
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void project_points_double_hortrapezoid(double *mat, double *points,
                                               double *proj, const int n,
                                               const int stride_points,
                                               const int stride_proj) {
  int i;
  double x, y, Z, Z_inv;
  for (i = 0; i < n; ++i) {
    x = *(points++), y = *(points++);
    Z_inv = mat[7] * y + 1;
    assert(fabs(Z_inv) > 0.000001);
    Z = 1. / Z_inv;
    *(proj++) = (mat[2] * x + mat[3] * y + mat[0]) * Z;
    *(proj++) = (mat[5] * y + mat[1]) * Z;
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void project_points_double_vertrapezoid(double *mat, double *points,
                                               double *proj, const int n,
                                               const int stride_points,
                                               const int stride_proj) {
  int i;
  double x, y, Z, Z_inv;
  for (i = 0; i < n; ++i) {
    x = *(points++), y = *(points++);
    Z_inv = mat[6] * x + 1;
    assert(fabs(Z_inv) > 0.000001);
    Z = 1. / Z_inv;
    *(proj++) = (mat[2] * x + mat[0]) * Z;
    *(proj++) = (mat[4] * x + mat[5] * y + mat[1]) * Z;
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void project_points_double_homography(double *mat, double *points,
                                             double *proj, const int n,
                                             const int stride_points,
                                             const int stride_proj) {
  int i;
  double x, y, Z, Z_inv;
  for (i = 0; i < n; ++i) {
    x = *(points++), y = *(points++);
    Z_inv = mat[6] * x + mat[7] * y + 1;
    assert(fabs(Z_inv) > 0.000001);
    Z = 1. / Z_inv;
    *(proj++) = (mat[2] * x + mat[3] * y + mat[0]) * Z;
    *(proj++) = (mat[4] * x + mat[5] * y + mat[1]) * Z;
    points += stride_points - 2;
    proj += stride_proj - 2;
  }
}

static void normalize_homography(double *pts, int n, double *T) {
  double *p = pts;
  double mean[2] = { 0, 0 };
  double msqe = 0;
  double scale;
  int i;
  for (i = 0; i < n; ++i, p += 2) {
    mean[0] += p[0];
    mean[1] += p[1];
  }
  mean[0] /= n;
  mean[1] /= n;
  for (p = pts, i = 0; i < n; ++i, p += 2) {
    p[0] -= mean[0];
    p[1] -= mean[1];
    msqe += sqrt(p[0] * p[0] + p[1] * p[1]);
  }
  msqe /= n;
  scale = (msqe == 0 ? 1.0 : sqrt(2) / msqe);
  T[0] = scale;
  T[1] = 0;
  T[2] = -scale * mean[0];
  T[3] = 0;
  T[4] = scale;
  T[5] = -scale * mean[1];
  T[6] = 0;
  T[7] = 0;
  T[8] = 1;
  for (p = pts, i = 0; i < n; ++i, p += 2) {
    p[0] *= scale;
    p[1] *= scale;
  }
}

static void invnormalize_mat(double *T, double *iT) {
  double is = 1.0 / T[0];
  double m0 = -T[2] * is;
  double m1 = -T[5] * is;
  iT[0] = is;
  iT[1] = 0;
  iT[2] = m0;
  iT[3] = 0;
  iT[4] = is;
  iT[5] = m1;
  iT[6] = 0;
  iT[7] = 0;
  iT[8] = 1;
}

static void denormalize_homography(double *params, double *T1, double *T2) {
  double iT2[9];
  double params2[9];
  invnormalize_mat(T2, iT2);
  multiply_mat(params, T1, params2, 3, 3, 3);
  multiply_mat(iT2, params2, params, 3, 3, 3);
}

static void denormalize_homography_reorder(double *params, double *T1,
                                           double *T2) {
  double params_denorm[MAX_PARAMDIM];
  memcpy(params_denorm, params, sizeof(*params) * 8);
  params_denorm[8] = 1.0;
  denormalize_homography(params_denorm, T1, T2);
  params[0] = params_denorm[2];
  params[1] = params_denorm[5];
  params[2] = params_denorm[0];
  params[3] = params_denorm[1];
  params[4] = params_denorm[3];
  params[5] = params_denorm[4];
  params[6] = params_denorm[6];
  params[7] = params_denorm[7];
}

static void denormalize_affine_reorder(double *params, double *T1, double *T2) {
  double params_denorm[MAX_PARAMDIM];
  params_denorm[0] = params[0];
  params_denorm[1] = params[1];
  params_denorm[2] = params[4];
  params_denorm[3] = params[2];
  params_denorm[4] = params[3];
  params_denorm[5] = params[5];
  params_denorm[6] = params_denorm[7] = 0;
  params_denorm[8] = 1;
  denormalize_homography(params_denorm, T1, T2);
  params[0] = params_denorm[2];
  params[1] = params_denorm[5];
  params[2] = params_denorm[0];
  params[3] = params_denorm[1];
  params[4] = params_denorm[3];
  params[5] = params_denorm[4];
  params[6] = params[7] = 0;
}

static void denormalize_rotzoom_reorder(double *params, double *T1,
                                        double *T2) {
  double params_denorm[MAX_PARAMDIM];
  params_denorm[0] = params[0];
  params_denorm[1] = params[1];
  params_denorm[2] = params[2];
  params_denorm[3] = -params[1];
  params_denorm[4] = params[0];
  params_denorm[5] = params[3];
  params_denorm[6] = params_denorm[7] = 0;
  params_denorm[8] = 1;
  denormalize_homography(params_denorm, T1, T2);
  params[0] = params_denorm[2];
  params[1] = params_denorm[5];
  params[2] = params_denorm[0];
  params[3] = params_denorm[1];
  params[4] = -params[3];
  params[5] = params[2];
  params[6] = params[7] = 0;
}

static void denormalize_translation_reorder(double *params, double *T1,
                                            double *T2) {
  double params_denorm[MAX_PARAMDIM];
  params_denorm[0] = 1;
  params_denorm[1] = 0;
  params_denorm[2] = params[0];
  params_denorm[3] = 0;
  params_denorm[4] = 1;
  params_denorm[5] = params[1];
  params_denorm[6] = params_denorm[7] = 0;
  params_denorm[8] = 1;
  denormalize_homography(params_denorm, T1, T2);
  params[0] = params_denorm[2];
  params[1] = params_denorm[5];
  params[2] = params[5] = 1;
  params[3] = params[4] = 0;
  params[6] = params[7] = 0;
}

static int find_translation(int np, double *pts1, double *pts2, double *mat) {
  int i;
  double sx, sy, dx, dy;
  double sumx, sumy;

  double T1[9], T2[9];
  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  sumx = 0;
  sumy = 0;
  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    sumx += dx - sx;
    sumy += dy - sy;
  }
  mat[0] = sumx / np;
  mat[1] = sumy / np;
  denormalize_translation_reorder(mat, T1, T2);
  return 0;
}

static int find_rotzoom(int np, double *pts1, double *pts2, double *mat) {
  const int np2 = np * 2;
  double *a = (double *)aom_malloc(sizeof(*a) * np2 * 10);
  double *b = a + np2 * 4;
  double *temp = b + np2;
  int i;
  double sx, sy, dx, dy;

  double T1[9], T2[9];
  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    a[i * 2 * 4 + 0] = sx;
    a[i * 2 * 4 + 1] = sy;
    a[i * 2 * 4 + 2] = 1;
    a[i * 2 * 4 + 3] = 0;
    a[(i * 2 + 1) * 4 + 0] = sy;
    a[(i * 2 + 1) * 4 + 1] = -sx;
    a[(i * 2 + 1) * 4 + 2] = 0;
    a[(i * 2 + 1) * 4 + 3] = 1;

    b[2 * i] = dx;
    b[2 * i + 1] = dy;
  }
  if (!least_squares(4, a, np2, 4, b, temp, mat)) {
    aom_free(a);
    return 1;
  }
  denormalize_rotzoom_reorder(mat, T1, T2);
  aom_free(a);
  return 0;
}

static int find_affine(int np, double *pts1, double *pts2, double *mat) {
  const int np2 = np * 2;
  double *a = (double *)aom_malloc(sizeof(*a) * np2 * 14);
  double *b = a + np2 * 6;
  double *temp = b + np2;
  int i;
  double sx, sy, dx, dy;

  double T1[9], T2[9];
  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    a[i * 2 * 6 + 0] = sx;
    a[i * 2 * 6 + 1] = sy;
    a[i * 2 * 6 + 2] = 0;
    a[i * 2 * 6 + 3] = 0;
    a[i * 2 * 6 + 4] = 1;
    a[i * 2 * 6 + 5] = 0;
    a[(i * 2 + 1) * 6 + 0] = 0;
    a[(i * 2 + 1) * 6 + 1] = 0;
    a[(i * 2 + 1) * 6 + 2] = sx;
    a[(i * 2 + 1) * 6 + 3] = sy;
    a[(i * 2 + 1) * 6 + 4] = 0;
    a[(i * 2 + 1) * 6 + 5] = 1;

    b[2 * i] = dx;
    b[2 * i + 1] = dy;
  }
  if (!least_squares(6, a, np2, 6, b, temp, mat)) {
    aom_free(a);
    return 1;
  }
  denormalize_affine_reorder(mat, T1, T2);
  aom_free(a);
  return 0;
}

static int find_vertrapezoid(int np, double *pts1, double *pts2, double *mat) {
  const int np3 = np * 3;
  double *a = (double *)aom_malloc(sizeof(*a) * np3 * 14);
  double *U = a + np3 * 7;
  double S[7], V[7 * 7], H[9];
  int i, mini;
  double sx, sy, dx, dy;
  double T1[9], T2[9];

  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    a[i * 3 * 7 + 0] = a[i * 3 * 7 + 1] = 0;
    a[i * 3 * 7 + 2] = -sx;
    a[i * 3 * 7 + 3] = -sy;
    a[i * 3 * 7 + 4] = -1;
    a[i * 3 * 7 + 5] = dy * sx;
    a[i * 3 * 7 + 6] = dy;

    a[(i * 3 + 1) * 7 + 0] = sx;
    a[(i * 3 + 1) * 7 + 1] = 1;
    a[(i * 3 + 1) * 7 + 2] = a[(i * 3 + 1) * 7 + 3] = a[(i * 3 + 1) * 7 + 4] =
        0;
    a[(i * 3 + 1) * 7 + 5] = -dx * sx;
    a[(i * 3 + 1) * 7 + 6] = -dx;

    a[(i * 3 + 2) * 7 + 0] = -dy * sx;
    a[(i * 3 + 2) * 7 + 1] = -dy;
    a[(i * 3 + 2) * 7 + 2] = dx * sx;
    a[(i * 3 + 2) * 7 + 3] = dx * sy;
    a[(i * 3 + 2) * 7 + 4] = dx;
    a[(i * 3 + 2) * 7 + 5] = a[(i * 3 + 2) * 7 + 6] = 0;
  }
  if (SVD(U, S, V, a, np3, 7)) {
    aom_free(a);
    return 1;
  } else {
    double minS = 1e12;
    mini = -1;
    for (i = 0; i < 7; ++i) {
      if (S[i] < minS) {
        minS = S[i];
        mini = i;
      }
    }
  }
  H[1] = H[7] = 0;
  for (i = 0; i < 1; i++) H[i] = V[i * 7 + mini];
  for (; i < 6; i++) H[i + 1] = V[i * 7 + mini];
  for (; i < 7; i++) H[i + 2] = V[i * 7 + mini];

  denormalize_homography_reorder(H, T1, T2);
  aom_free(a);
  if (H[8] == 0.0) {
    return 1;
  } else {
    // normalize
    double f = 1.0 / H[8];
    for (i = 0; i < 8; i++) mat[i] = f * H[i];
  }
  return 0;
}

static int find_hortrapezoid(int np, double *pts1, double *pts2, double *mat) {
  const int np3 = np * 3;
  double *a = (double *)aom_malloc(sizeof(*a) * np3 * 14);
  double *U = a + np3 * 7;
  double S[7], V[7 * 7], H[9];
  int i, mini;
  double sx, sy, dx, dy;
  double T1[9], T2[9];

  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    a[i * 3 * 7 + 0] = a[i * 3 * 7 + 1] = a[i * 3 * 7 + 2] = 0;
    a[i * 3 * 7 + 3] = -sy;
    a[i * 3 * 7 + 4] = -1;
    a[i * 3 * 7 + 5] = dy * sy;
    a[i * 3 * 7 + 6] = dy;

    a[(i * 3 + 1) * 7 + 0] = sx;
    a[(i * 3 + 1) * 7 + 1] = sy;
    a[(i * 3 + 1) * 7 + 2] = 1;
    a[(i * 3 + 1) * 7 + 3] = a[(i * 3 + 1) * 7 + 4] = 0;
    a[(i * 3 + 1) * 7 + 5] = -dx * sy;
    a[(i * 3 + 1) * 7 + 6] = -dx;

    a[(i * 3 + 2) * 7 + 0] = -dy * sx;
    a[(i * 3 + 2) * 7 + 1] = -dy * sy;
    a[(i * 3 + 2) * 7 + 2] = -dy;
    a[(i * 3 + 2) * 7 + 3] = dx * sy;
    a[(i * 3 + 2) * 7 + 4] = dx;
    a[(i * 3 + 2) * 7 + 5] = a[(i * 3 + 2) * 7 + 6] = 0;
  }

  if (SVD(U, S, V, a, np3, 7)) {
    aom_free(a);
    return 1;
  } else {
    double minS = 1e12;
    mini = -1;
    for (i = 0; i < 7; ++i) {
      if (S[i] < minS) {
        minS = S[i];
        mini = i;
      }
    }
  }
  H[3] = H[6] = 0;
  for (i = 0; i < 3; i++) H[i] = V[i * 7 + mini];
  for (; i < 5; i++) H[i + 1] = V[i * 7 + mini];
  for (; i < 7; i++) H[i + 2] = V[i * 7 + mini];

  denormalize_homography_reorder(H, T1, T2);
  aom_free(a);
  if (H[8] == 0.0) {
    return 1;
  } else {
    // normalize
    double f = 1.0 / H[8];
    for (i = 0; i < 8; i++) mat[i] = f * H[i];
  }
  return 0;
}

static int find_homography(int np, double *pts1, double *pts2, double *mat) {
  // Implemented from Peter Kovesi's normalized implementation
  const int np3 = np * 3;
  double *a = (double *)aom_malloc(sizeof(*a) * np3 * 18);
  double *U = a + np3 * 9;
  double S[9], V[9 * 9], H[9];
  int i, mini;
  double sx, sy, dx, dy;
  double T1[9], T2[9];

  normalize_homography(pts1, np, T1);
  normalize_homography(pts2, np, T2);

  for (i = 0; i < np; ++i) {
    dx = *(pts2++);
    dy = *(pts2++);
    sx = *(pts1++);
    sy = *(pts1++);

    a[i * 3 * 9 + 0] = a[i * 3 * 9 + 1] = a[i * 3 * 9 + 2] = 0;
    a[i * 3 * 9 + 3] = -sx;
    a[i * 3 * 9 + 4] = -sy;
    a[i * 3 * 9 + 5] = -1;
    a[i * 3 * 9 + 6] = dy * sx;
    a[i * 3 * 9 + 7] = dy * sy;
    a[i * 3 * 9 + 8] = dy;

    a[(i * 3 + 1) * 9 + 0] = sx;
    a[(i * 3 + 1) * 9 + 1] = sy;
    a[(i * 3 + 1) * 9 + 2] = 1;
    a[(i * 3 + 1) * 9 + 3] = a[(i * 3 + 1) * 9 + 4] = a[(i * 3 + 1) * 9 + 5] =
        0;
    a[(i * 3 + 1) * 9 + 6] = -dx * sx;
    a[(i * 3 + 1) * 9 + 7] = -dx * sy;
    a[(i * 3 + 1) * 9 + 8] = -dx;

    a[(i * 3 + 2) * 9 + 0] = -dy * sx;
    a[(i * 3 + 2) * 9 + 1] = -dy * sy;
    a[(i * 3 + 2) * 9 + 2] = -dy;
    a[(i * 3 + 2) * 9 + 3] = dx * sx;
    a[(i * 3 + 2) * 9 + 4] = dx * sy;
    a[(i * 3 + 2) * 9 + 5] = dx;
    a[(i * 3 + 2) * 9 + 6] = a[(i * 3 + 2) * 9 + 7] = a[(i * 3 + 2) * 9 + 8] =
        0;
  }

  if (SVD(U, S, V, a, np3, 9)) {
    aom_free(a);
    return 1;
  } else {
    double minS = 1e12;
    mini = -1;
    for (i = 0; i < 9; ++i) {
      if (S[i] < minS) {
        minS = S[i];
        mini = i;
      }
    }
  }

  for (i = 0; i < 9; i++) H[i] = V[i * 9 + mini];
  denormalize_homography_reorder(H, T1, T2);
  aom_free(a);
  if (H[8] == 0.0) {
    return 1;
  } else {
    // normalize
    double f = 1.0 / H[8];
    for (i = 0; i < 8; i++) mat[i] = f * H[i];
  }
  return 0;
}

// Generate a random number in the range [0, 32768).
static unsigned int lcg_rand16(unsigned int *state) {
  *state = (unsigned int)(*state * 1103515245ULL + 12345);
  return *state / 65536 % 32768;
}

static int get_rand_indices(int npoints, int minpts, int *indices,
                            unsigned int *seed) {
  int i, j;
  int ptr = lcg_rand16(seed) % npoints;
  if (minpts > npoints) return 0;
  indices[0] = ptr;
  ptr = (ptr == npoints - 1 ? 0 : ptr + 1);
  i = 1;
  while (i < minpts) {
    int index = lcg_rand16(seed) % npoints;
    while (index) {
      ptr = (ptr == npoints - 1 ? 0 : ptr + 1);
      for (j = 0; j < i; ++j) {
        if (indices[j] == ptr) break;
      }
      if (j == i) index--;
    }
    indices[i++] = ptr;
  }
  return 1;
}

typedef struct {
  int num_inliers;
  double variance;
  int *inlier_indices;
} RANSAC_MOTION;

// Return -1 if 'a' is a better motion, 1 if 'b' is better, 0 otherwise.
static int compare_motions(const void *arg_a, const void *arg_b) {
  const RANSAC_MOTION *motion_a = (RANSAC_MOTION *)arg_a;
  const RANSAC_MOTION *motion_b = (RANSAC_MOTION *)arg_b;

  if (motion_a->num_inliers > motion_b->num_inliers) return -1;
  if (motion_a->num_inliers < motion_b->num_inliers) return 1;
  if (motion_a->variance < motion_b->variance) return -1;
  if (motion_a->variance > motion_b->variance) return 1;
  return 0;
}

static int is_better_motion(const RANSAC_MOTION *motion_a,
                            const RANSAC_MOTION *motion_b) {
  return compare_motions(motion_a, motion_b) < 0;
}

static void copy_points_at_indices(double *dest, const double *src,
                                   const int *indices, int num_points) {
  for (int i = 0; i < num_points; ++i) {
    const int index = indices[i];
    dest[i * 2] = src[index * 2];
    dest[i * 2 + 1] = src[index * 2 + 1];
  }
}

static const double kInfiniteVariance = 1e12;

static void clear_motion(RANSAC_MOTION *motion, int num_points) {
  motion->num_inliers = 0;
  motion->variance = kInfiniteVariance;
  memset(motion->inlier_indices, 0,
         sizeof(*motion->inlier_indices * num_points));
}

static int ransac(const int *matched_points, int npoints,
                  int *num_inliers_by_motion, double *params_by_motion,
                  int num_desired_motions, const int minpts,
                  IsDegenerateFunc is_degenerate,
                  FindTransformationFunc find_transformation,
                  ProjectPointsDoubleFunc projectpoints) {
  static const double PROBABILITY_REQUIRED = 0.9;
  static const double EPS = 1e-12;

  int N = 10000, trial_count = 0;
  int i = 0;
  int ret_val = 0;

  unsigned int seed = (unsigned int)npoints;

  int indices[MAX_MINPTS] = { 0 };

  double *points1, *points2;
  double *corners1, *corners2;
  double *image1_coord;

  // Store information for the num_desired_motions best transformations found
  // and the worst motion among them, as well as the motion currently under
  // consideration.
  RANSAC_MOTION *motions, *worst_kept_motion = NULL;
  RANSAC_MOTION current_motion;

  // Store the parameters and the indices of the inlier points for the motion
  // currently under consideration.
  double params_this_motion[MAX_PARAMDIM];

  double *cnp1, *cnp2;

  for (i = 0; i < num_desired_motions; ++i) {
    num_inliers_by_motion[i] = 0;
  }
  if (npoints < minpts * MINPTS_MULTIPLIER || npoints == 0) {
    return 1;
  }

  points1 = (double *)aom_malloc(sizeof(*points1) * npoints * 2);
  points2 = (double *)aom_malloc(sizeof(*points2) * npoints * 2);
  corners1 = (double *)aom_malloc(sizeof(*corners1) * npoints * 2);
  corners2 = (double *)aom_malloc(sizeof(*corners2) * npoints * 2);
  image1_coord = (double *)aom_malloc(sizeof(*image1_coord) * npoints * 2);

  motions =
      (RANSAC_MOTION *)aom_malloc(sizeof(RANSAC_MOTION) * num_desired_motions);
  for (i = 0; i < num_desired_motions; ++i) {
    motions[i].inlier_indices =
        (int *)aom_malloc(sizeof(*motions->inlier_indices) * npoints);
    clear_motion(motions + i, npoints);
  }
  current_motion.inlier_indices =
      (int *)aom_malloc(sizeof(*current_motion.inlier_indices) * npoints);
  clear_motion(&current_motion, npoints);

  worst_kept_motion = motions;

  if (!(points1 && points2 && corners1 && corners2 && image1_coord && motions &&
        current_motion.inlier_indices)) {
    ret_val = 1;
    goto finish_ransac;
  }

  cnp1 = corners1;
  cnp2 = corners2;
  for (i = 0; i < npoints; ++i) {
    *(cnp1++) = *(matched_points++);
    *(cnp1++) = *(matched_points++);
    *(cnp2++) = *(matched_points++);
    *(cnp2++) = *(matched_points++);
  }

  while (N > trial_count) {
    double sum_distance = 0.0;
    double sum_distance_squared = 0.0;

    clear_motion(&current_motion, npoints);

    int degenerate = 1;
    int num_degenerate_iter = 0;

    while (degenerate) {
      num_degenerate_iter++;
      if (!get_rand_indices(npoints, minpts, indices, &seed)) {
        ret_val = 1;
        goto finish_ransac;
      }

      copy_points_at_indices(points1, corners1, indices, minpts);
      copy_points_at_indices(points2, corners2, indices, minpts);

      degenerate = is_degenerate(points1);
      if (num_degenerate_iter > MAX_DEGENERATE_ITER) {
        ret_val = 1;
        goto finish_ransac;
      }
    }

    if (find_transformation(minpts, points1, points2, params_this_motion)) {
      trial_count++;
      continue;
    }

    projectpoints(params_this_motion, corners1, image1_coord, npoints, 2, 2);

    for (i = 0; i < npoints; ++i) {
      double dx = image1_coord[i * 2] - corners2[i * 2];
      double dy = image1_coord[i * 2 + 1] - corners2[i * 2 + 1];
      double distance = sqrt(dx * dx + dy * dy);

      if (distance < INLIER_THRESHOLD) {
        current_motion.inlier_indices[current_motion.num_inliers++] = i;
        sum_distance += distance;
        sum_distance_squared += distance * distance;
      }
    }

    if (current_motion.num_inliers >= worst_kept_motion->num_inliers &&
        current_motion.num_inliers > 1) {
      int temp;
      double fracinliers, pNoOutliers, mean_distance;
      mean_distance = sum_distance / ((double)current_motion.num_inliers);
      current_motion.variance =
          sum_distance_squared / ((double)current_motion.num_inliers - 1.0) -
          mean_distance * mean_distance * ((double)current_motion.num_inliers) /
              ((double)current_motion.num_inliers - 1.0);
      if (is_better_motion(&current_motion, worst_kept_motion)) {
        // This motion is better than the worst currently kept motion. Remember
        // the inlier points and variance. The parameters for each kept motion
        // will be recomputed later using only the inliers.
        worst_kept_motion->num_inliers = current_motion.num_inliers;
        worst_kept_motion->variance = current_motion.variance;
        memcpy(worst_kept_motion->inlier_indices, current_motion.inlier_indices,
               sizeof(*current_motion.inlier_indices) * npoints);

        assert(npoints > 0);
        fracinliers = (double)current_motion.num_inliers / (double)npoints;
        pNoOutliers = 1 - pow(fracinliers, minpts);
        pNoOutliers = fmax(EPS, pNoOutliers);
        pNoOutliers = fmin(1 - EPS, pNoOutliers);
        temp = (int)(log(1.0 - PROBABILITY_REQUIRED) / log(pNoOutliers));

        if (temp > 0 && temp < N) {
          N = AOMMAX(temp, MIN_TRIALS);
        }

        // Determine the new worst kept motion and its num_inliers and variance.
        for (i = 0; i < num_desired_motions; ++i) {
          if (is_better_motion(worst_kept_motion, &motions[i])) {
            worst_kept_motion = &motions[i];
          }
        }
      }
    }
    trial_count++;
  }

  // Sort the motions, best first.
  qsort(motions, num_desired_motions, sizeof(RANSAC_MOTION), compare_motions);

  // Recompute the motions using only the inliers.
  for (i = 0; i < num_desired_motions; ++i) {
    copy_points_at_indices(points1, corners1, motions[i].inlier_indices,
                           motions[i].num_inliers);
    copy_points_at_indices(points2, corners2, motions[i].inlier_indices,
                           motions[i].num_inliers);

    find_transformation(motions[i].num_inliers, points1, points2,
                        params_by_motion + (MAX_PARAMDIM - 1) * i);
    num_inliers_by_motion[i] = motions[i].num_inliers;
  }

finish_ransac:
  aom_free(points1);
  aom_free(points2);
  aom_free(corners1);
  aom_free(corners2);
  aom_free(image1_coord);
  aom_free(current_motion.inlier_indices);
  for (i = 0; i < num_desired_motions; ++i) {
    aom_free(motions[i].inlier_indices);
  }
  aom_free(motions);

  return ret_val;
}

static int is_collinear3(double *p1, double *p2, double *p3) {
  static const double collinear_eps = 1e-3;
  const double v =
      (p2[0] - p1[0]) * (p3[1] - p1[1]) - (p2[1] - p1[1]) * (p3[0] - p1[0]);
  return fabs(v) < collinear_eps;
}

static int is_degenerate_translation(double *p) {
  return (p[0] - p[2]) * (p[0] - p[2]) + (p[1] - p[3]) * (p[1] - p[3]) <= 2;
}

static int is_degenerate_affine(double *p) {
  return is_collinear3(p, p + 2, p + 4);
}

static int is_degenerate_homography(double *p) {
  return is_collinear3(p, p + 2, p + 4) || is_collinear3(p, p + 2, p + 6) ||
         is_collinear3(p, p + 4, p + 6) || is_collinear3(p + 2, p + 4, p + 6);
}

int ransac_translation(int *matched_points, int npoints,
                       int *num_inliers_by_motion, double *params_by_motion,
                       int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 3,
                is_degenerate_translation, find_translation,
                project_points_double_translation);
}

int ransac_rotzoom(int *matched_points, int npoints, int *num_inliers_by_motion,
                   double *params_by_motion, int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 3, is_degenerate_affine,
                find_rotzoom, project_points_double_rotzoom);
}

int ransac_affine(int *matched_points, int npoints, int *num_inliers_by_motion,
                  double *params_by_motion, int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 3, is_degenerate_affine,
                find_affine, project_points_double_affine);
}

int ransac_homography(int *matched_points, int npoints,
                      int *num_inliers_by_motion, double *params_by_motion,
                      int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 4,
                is_degenerate_homography, find_homography,
                project_points_double_homography);
}

int ransac_hortrapezoid(int *matched_points, int npoints,
                        int *num_inliers_by_motion, double *params_by_motion,
                        int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 4,
                is_degenerate_homography, find_hortrapezoid,
                project_points_double_hortrapezoid);
}

int ransac_vertrapezoid(int *matched_points, int npoints,
                        int *num_inliers_by_motion, double *params_by_motion,
                        int num_desired_motions) {
  return ransac(matched_points, npoints, num_inliers_by_motion,
                params_by_motion, num_desired_motions, 4,
                is_degenerate_homography, find_vertrapezoid,
                project_points_double_vertrapezoid);
}
