/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

//#define USE_UI

#ifdef GEGL_PROPERTIES

#define DEFAULT_CONFIG \
"\n"\
"substrate=white\n"\
"black=rgb 0 0 0\n"\
"ink1=black\n"\
"ink2=red\n"\
"ink2.black=black\n"\
"\n"\

enum_start (ink_sim_mode)
  enum_value (GEGL_SSIM_PROOF,          "proof",          N_("Proof"))
  enum_value (GEGL_SSIM_SEPARATE,       "separate",       N_("Separate"))
  enum_value (GEGL_SSIM_SEPARATE_PROOF, "separate-proof", N_("Separate and proof"))
enum_end (GeglInkSimMode)

property_string (config, _("Ink configuration"), DEFAULT_CONFIG)
  description (_("Textual desciption of inks used for simulated print-job"))
  ui_meta ("multiline", "true")

property_enum (mode, _("Mode"), GeglInkSimMode, ink_sim_mode,
                 GEGL_SSIM_SEPARATE_PROOF)
  description (_("how the ink simulator is used"))

#ifdef USE_UI

property_color (substrate_color, _("Substrate color"), "#ffffff", _("paper/fabric/material color"))
property_color (ink1_color, _("Ink1 color"), "#00ffff00", _("ink color"))
property_color (ink2_color, _("Ink2 color"), "#ff00ff00", _("ink color"))
property_color (ink3_color, _("Ink3 color"), "#ffff0000", _("ink color"))
property_color (ink4_color, _("Ink4 color"), "#00000000", _("ink color"))
property_color (ink5_color, _("Ink5 color"), "#ff000000", _("ink color"))

property_double (ink_limit, _("Ink limit"), 0.0, 5.0, 3.0,
                   _("maximum amount of ink for one pixel, 2.5 = 250%% coverage"))

#endif

property_int (debug_width, _("Debug width"), 0)
   value_range (0, 150)
   description (_("how wide peel off bands for ink order vis"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME ink_simulator
#define GEGL_OP_C_SOURCE ink-simulator.c

#include "gegl-op.h"
#include <math.h>

/*********************/

  /* the code is seld contained; but shared inside this gegl-op for now */


#ifndef SSIM_H_
#define SSIM_H_

#include <stdint.h>

typedef struct _SSim SSim;
#define SSIM_MAX_INKS 16

SSim *ssim_new              (const char *config);
void  ssim_destroy          (SSim       *ssim);
void  ssim_proof            (SSim       *ssim,
                             float      *ink_levels,
                             float      *rgba);
void  ssim_separate         (SSim       *ssim,
                             float      *rgb,
                             float      *ink_levels);
int   ssim_get_ink_count    (SSim       *ssim);
/* this allows proofing with a lower amount of inks,
 * without writing a full new config for doing that.
 */
void  ssim_set_ink_count    (SSim       *ssim,
                             int         count);
float ssim_get_ink_limit    (SSim *ssim);
void  ssim_set_ink_limit    (SSim *ssim, float limit);
void  ssim_set_halftone_sim (SSim *ssim,
                             int   enabled);
int   ssim_get_halftone_sim (SSim *ssim);

//const Spectrum *ssim_get_spectrum (SSim *ssim, const char *name)
//void ssim_set_spectrum (SSim *ssim, const char *name, Spectrum *spectrum);

/* 1 0.0                1.0
 * 2 0.0      0.50      1.0
 * 3 0.0   0.33  0.66   1.0
 * 4 0.0 0.25 0.50 0.75 1.0
 */


#endif

/* Spectral Color Simulator
 *
 * ssim is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ssim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ssim; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Øyvind Kolås <pippin@gimp.org>
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

/* the kubelka munk method is more broken than the naive internal one */
//#define KUBELKA_MUNK 1

static const char *config_internal =

"observer_l= 390 20 1 0.002177 0.0158711 0.03687185 0.06265 0.13059 0.26828 0.57836 0.849104 0.9704655 0.9793335 0.85568175 0.59266275 0.30191075 0.11285 0.032197 0.007657 0.0017424 0.0004039 0.000009 0.000002\n"

"observer_m= 390 20 1 0.00207 0.01863 0.057983 0.11151 0.221277 0.4010105 0.75920 0.97423 0.93118 0.694917 0.37578 0.14093 0.03995 0.0096 0.002 0.0004 0.0001095 0.0000026 0\n"

"observer_s= 390 20 1 0.05312 0.45811575 0.9226575 0.83515125 0.46085625 0.146032125 0.03805695 0.00723844 0.0010894 0.000158823 0.0000025 0.000000336 0\n"

"observer_l= 390 5 1 4.07619E-04 1.06921E-03 2.54073E-03 5.31546E-03 9.98835E-03 1.60130E-02 2.33957E-02 3.09104E-02 3.97810E-02 4.94172E-02 5.94619E-02 6.86538E-02 7.95647E-02 9.07704E-02 1.06663E-01 1.28336E-01 1.51651E-01 1.77116E-01 2.07940E-01 2.44046E-01 2.82752E-01 3.34786E-01 3.91705E-01 4.56252E-01 5.26538E-01 5.99867E-01 6.75313E-01 7.37108E-01 7.88900E-01 8.37403E-01 8.90871E-01 9.26660E-01 9.44527E-01 9.70703E-01 9.85636E-01 9.96979E-01 9.99543E-01 9.87057E-01 9.57841E-01 9.39781E-01 9.06693E-01 8.59605E-01 8.03173E-01 7.40680E-01 6.68991E-01 5.93248E-01 5.17449E-01 4.45125E-01 3.69168E-01 3.00316E-01 2.42316E-01 1.93730E-01 1.49509E-01 1.12638E-01 8.38077E-02 6.16384E-02 4.48132E-02 3.21660E-02 2.27738E-02 1.58939E-02 1.09123E-02 7.59453E-03 5.28607E-03 3.66675E-03 2.51327E-03 1.72108E-03 1.18900E-03 8.22396E-04 5.72917E-04 3.99670E-04 2.78553E-04 1.96528E-04 1.38482E-04 9.81226E-05 6.98827E-05 4.98430E-05 3.57781E-05 2.56411E-05 1.85766E-05 1.34792E-05 9.80671E-06 7.14808E-06 5.24229E-06 3.86280E-06 2.84049E-06 2.10091E-06 1.56506E-06 1.16700E-06 8.73008E-07\n"

"observer_m= 390 5 1 3.58227E-04 9.64828E-04 2.37208E-03 5.12316E-03 9.98841E-03 1.72596E-02 2.73163E-02 3.96928E-02 5.55384E-02 7.50299E-02 9.57612E-02 1.16220E-01 1.39493E-01 1.62006E-01 1.93202E-01 2.32275E-01 2.71441E-01 3.10372E-01 3.55066E-01 4.05688E-01 4.56137E-01 5.22970E-01 5.91003E-01 6.66404E-01 7.43612E-01 8.16808E-01 8.89214E-01 9.34977E-01 9.61962E-01 9.81481E-01 9.98931E-01 9.91383E-01 9.61876E-01 9.35829E-01 8.90949E-01 8.40969E-01 7.76526E-01 7.00013E-01 6.11728E-01 5.31825E-01 4.54142E-01 3.76527E-01 3.04378E-01 2.39837E-01 1.85104E-01 1.40431E-01 1.04573E-01 7.65841E-02 5.54990E-02 3.97097E-02 2.80314E-02 1.94366E-02 1.37660E-02 9.54315E-03 6.50455E-03 4.42794E-03 3.06050E-03 2.11596E-03 1.45798E-03 9.98424E-04 6.77653E-04 4.67870E-04 3.25278E-04 2.25641E-04 1.55286E-04 1.07388E-04 7.49453E-05 5.24748E-05 3.70443E-05 2.62088E-05 1.85965E-05 1.33965E-05 9.63397E-06 6.96522E-06 5.06711E-06 3.68617E-06 2.69504E-06 1.96864E-06 1.45518E-06 1.07784E-06 8.00606E-07 5.96195E-07 4.48030E-07 3.38387E-07 2.54942E-07 1.93105E-07 1.47054E-07 1.11751E-07 8.48902E-08\n"

"observer_s= 390 5 1 6.14265E-03 1.59515E-02 3.96308E-02 8.97612E-02 1.78530E-01 3.05941E-01 4.62692E-01 6.09570E-01 7.56885E-01 8.69984E-01 9.66960E-01 9.93336E-01 9.91329E-01 9.06735E-01 8.23726E-01 7.37043E-01 6.10456E-01 4.70894E-01 3.50108E-01 2.58497E-01 1.85297E-01 1.35351E-01 9.67990E-02 6.49614E-02 4.12337E-02 2.71300E-02 1.76298E-02 1.13252E-02 7.17089E-03 4.54287E-03 2.83352E-03 1.75573E-03 1.08230E-03 6.64512E-04 4.08931E-04 2.51918E-04 1.55688E-04 9.67045E-05 6.04705E-05 3.81202E-05 2.42549E-05 1.55924E-05 1.01356E-05 6.66657E-06 4.43906E-06 2.99354E-06 0.0\n"

"cyan= 390 20 1 0.8 0.8 0.9 0.95 0.95 0.95 0.9 0.9 0.82 0.75 0.6 0.55 0.5 0.4 0.3 0.22 0.3 0.37 0.57 0.72\n"
"magenta= 390 20 1 1 1 1 0 0 0 0 0 0 1 1 1 0 0 0 0 0 0 0 0\n"
"yellow= 390 20 1 0.42 0.48 0.53 0.58 0.64 0.68 0.77 0.86 0.9  1 1 1 1 0 0 0 0 0 0 0\n"

"black= 300 20 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \n"
"blue=  390 20 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
"green= 390 20 1 0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0\n"
"red =  390 20 1 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1\n"
"white= 390 20 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"

"paper = 400 10 0.11 7.5404286 7.8298571 8.4084286 8.7702857 8.8400000 8.8737143 8.9232857 8.9704286 9.0148571 9.0687143 9.1084286 9.1482857 9.1900000 9.2054286 9.2240000 9.2302857 9.2060000 9.2301429 9.2268571 9.2401429 9.2400000 9.2524286 9.2640000 9.2732857 9.3015714 9.3287143 9.3427143 9.3327143 9.3237143 9.3204286 9.3301429 # from jeremie gerhardt, assumed neutral substrate measurement \n"

"cyan = 400 10 0.11 9.8113888 1.0670272 1.2382906 1.4463066 1.9420151 2.4855488 2.7483403 2.9397748 2.9864173 2.8038144 2.4006503 1.8511818 1.2758176 8.0055737 4.8084866 2.7895001 1.6998021 1.2898931 1.2199036 1.2299055 1.3098927 1.4198782 1.5898522 1.6498449 1.7198448 1.7798469 1.7498584 1.7598522 1.7398515 1.7498482 1.8798300\n"

"magenta = 400 10 0.11 1.2863557 1.2808865 1.2690493 1.2432869 1.1399922 1.0005523 8.3708221 6.7146438 5.0772161 3.7486537 2.9692297 2.3795507 1.8397582 1.5498352 1.5698391 1.6698210 1.6798067 1.8997681 3.0094126 6.9169947 1.5584424 2.7888956 3.9888990 4.8850636 5.4932014 5.9286392 6.2264940 6.3711174 6.3289353 6.2883122 6.3226963 \n"

"yellow = 400 10 0.11 3.4901877 3.4328886 3.3266590 2.5688609 2.3791376 2.3691969 2.4292326 2.6091947 3.1389379 5.1374679 1.1957328 2.7395495 4.7471132 6.3588601 7.3552370 7.8975594 8.1510866 8.3363665 8.4318689 8.5226294 8.5732716 8.6260495 8.6731905 8.7107605 8.7727353 8.8243792 8.8577589 8.8516172 8.8355047 8.8305203 8.8606813 # from Jeremie Gerhardt \n"

"dark_magenta = 400 10 0.11 1.5181483 1.5186113 1.5193041 1.4096572 1.3497226 1.2997584 1.2897838 1.2898034 1.2398345 1.2298553 1.2098722 1.1898877 1.1599039 1.1399109 1.1099196 1.0799251 1.0499245 1.0399305 1.0199326 1.0099363 9.9993749 1.0099384 1.0099403 1.0299396 1.0499422 1.0899426 1.1899345 1.3199169 1.4398983 1.5698778 1.6698658 # from Jeremie Gerhardt \n"

"blue = 400 10 0.11 1.2677560 1.3045020 1.3761595 1.4124810 1.3252730 1.1739987 9.9769206 8.2918111 6.5253930 4.9276710 3.8886779 3.0892423 2.1896575 1.6398155 1.5998328 1.7298079 1.6798067 1.7797965 2.2596690 2.9394595 3.2793269 3.2393654 3.2393856 3.2593942 3.4293824 3.7093343 4.0792291 4.2691287 4.0591902 3.7692949 3.4694203 # from Jeremie Gerhardt \n"

"light_magenta = 400 10 0.11 5.4856769 5.4619427 5.4111386 5.1254544 4.9862032 4.9564770 4.8769006 4.8372269 4.8774315 4.6379375 4.1484949 3.4590498 2.5795246 2.0197201 2.0097362 2.2296807 2.2796439 2.5895690 4.2488281 9.2046574 1.9255898 3.3553174 4.8132837 5.9475725 6.7066222 7.1778868 7.4608931 7.6137962 7.7141798 7.7933523 7.8914000 # from Jeremie Gerhardt \n"

"gray= 390 20 1 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5\n"
"D65= 390 20 0.54648200 0.91486000 0.86682300 1.17008000 1.14861000 1.08811000 1.07802000 1.07689000 1.04046000 0.96334200 0.88685600 0.89599100 0.83288600 0.80026800 0.82277800 0.69721300 0.74349000 0.69885600 0.63592700 0.66805400\n"

"D65= 300 5 1 0.000341 0.016643 0.032945 0.117652 0.20236 0.286447 0.370535 0.385011 0.399488 0.424302 0.449117 0.45775 0.466383 0.493637 0.520891 0.510323 0.499755 0.523118 0.546482 0.687015 0.827549 0.871204 0.91486 0.924589 0.934318 0.90057 0.866823 0.957736 1.04865 1.10936 1.17008 1.1741 1.17812 1.16336 1.14861 1.15392 1.15923 1.12367 1.08811 1.09082 1.09.354 1.08578 1.07802 1.06296 1.0479 1.06239 1.07689 1.06047 1.04405 1.04225 1.04046 1.02023 1.00 0.981671 0.963342 0.960611 0.95788 0.922368 0.886856 0.893459 0.900062 0.898026 0.895991 0.886489 0.876987 0.854936 0.832886 0.834939 0.836992 0.81863 0.800268 0.801207 0.802146 0.812462 0.822778 0.80281 0.782842 0.740027 0.697213 0.706652 0.716091 0.72979 0.74349 0.679765 0.61604 0.657448 0.698856 0.724863 0.75087 0.693398 0.635927 0.550054 0.464182 0.566118 0.668054 0.650941 0.633828 0.638434 0.64304 0.618779 0.594519 0.557054 0.51959 0.546998 0.574406 0.588765 0.603125\n"

"D55= 390 20 1 0.3809 0.6855 0.6791 0.9799 0.9991 0.9808 1.0070 1.0421 1.0297 0.9722 0.9143 0.9514 0.9045 0.8885 0.9395 0.7968 0.8484 0.7930 0.7188 0.7593\n"
"D50= 390 20 1 0.4980 0.5850 0.5780 0.8720 0.9140 0.9200 0.9660 1.0210 1.0230 0.9770 0.9350 0.9930 0.9570 0.9570 1.0300 0.8740 0.9290 0.8660 0.7820 0.8290\n"
"E=white\n"

"illuminant=D65\n"
"substrate=paper\n"
"inklimit=3.0\n"

;

#define SPECTRUM_BANDS   22
#define SPECTRUM_GAP     13

//#define SPECTRUM_BANDS   11
//#define SPECTRUM_GAP     26

#define SPECTRUM_START   400
#define SPECTRUM_DB_SIZE 512
#define LUT_DIM          48

//static int STOCHASTIC_ITERATIONS = 384;
//static float STOCHASTIC_DIFFUSION = 0.03;
static int STOCHASTIC_ITERATIONS = 256;
static float STOCHASTIC_DIFFUSION = 0.5;

#ifndef CLAMP
#define CLAMP(a,b,c) ((a)<(b)?(b):(a)>(c)?(c):(c))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define INK_FLAG_DISABLED   0
#define INK_FLAG_SEPARATE   1
#define INK_FLAG_PROOF      2
#define INK_FLAG_ENABLED   (INK_FLAG_SEPARATE | INK_FLAG_PROOF)

typedef struct _Ink Ink;
typedef struct _Spectrum Spectrum;

enum {
  SSIM_COLOR_SUBSTRATE = -1,
  SSIM_COLOR_ILLUMINANT = 0,
  SSIM_COLOR_INK1 = 1,
  SSIM_COLOR_INK2 = 2,
  SSIM_COLOR_INK3 = 3,
  SSIM_COLOR_INK4 = 4,
  SSIM_COLOR_INK5 = 5,
  SSIM_COLOR_INK6 = 6,
  SSIM_COLOR_INK7 = 7,
  SSIM_COLOR_INK8 = 8
};

struct _Spectrum {
  float bands[SPECTRUM_BANDS];
};

struct _Ink {
  Spectrum on_white; /* spectral energy of this ink; as reflected of a fully reflective background */
  Spectrum on_black; /* spectral energy of this ink; as reflected of a fully reflective background */
  Spectrum k;
  Spectrum s;
  float   scale;         /* scale factor; increasing the amount of spectral contribution */

  float   dot_gain;
  int32_t flags;
};

typedef struct _InkMix InkMix;

struct _InkMix
{
  int32_t defined;
  float   level[SSIM_MAX_INKS];
};

typedef struct _SpectrumDb SpectrumDb;

struct _SpectrumDb
{
  Spectrum spectrum[SPECTRUM_DB_SIZE];
  char     name[SPECTRUM_DB_SIZE][32];
  int      count;
};

struct _SSim
{
  SpectrumDb db;

  Spectrum illuminant;
  Spectrum substrate;
  Spectrum mixtures[1 << SSIM_MAX_INKS];

  Ink      ink_def[SSIM_MAX_INKS];
  int32_t  inks;
  float    ink_limit;
  InkMix   lut[LUT_DIM*LUT_DIM*LUT_DIM]; 
  int32_t  debug_width;
  int32_t  halftone_sim;
  char    *src; /* cached version of the source resulting in a configuration */
};

/* need a spectral resampler,.. 
 *
 * setting data structure directly, a
 *
 */

const Spectrum *ssim_get_spectrum (SSim *ssim, const char *name);

static inline int lut_indice (float  val, float *delta)
{
  /* LUT_DIM-1 to have both 0.0 and 1.0 values to interpolate from */
  int v = floor (val * (LUT_DIM - 1));
  if (v < 0)
    v = 0;
  if (v >= (LUT_DIM-1))
    v = LUT_DIM - 2;
  //if (delta)
     *delta = ((val * (LUT_DIM - 1)) - v);
  return v;
}

static inline int
lut_index (int ri,
           int gi,
           int bi)
{
  return ri * LUT_DIM * LUT_DIM + gi * LUT_DIM + bi;
}

/* from http://cvrl.ucl.ac.uk/cones.htm */
static Spectrum STANDARD_OBSERVER_L;
static Spectrum STANDARD_OBSERVER_M;
static Spectrum STANDARD_OBSERVER_S;


static inline void
spectrum_remove_light (Spectrum       *s,
                       const Spectrum *ink,
                       float          coverage)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = s->bands[i] * (1.0 - coverage) +
                  s->bands[i] * ink->bands[i] * coverage;
}

static inline void
spectrum_add_opaque_ink (Spectrum       *s,
                         const Spectrum *ink,
                         const Spectrum *illuminant,
                         float          coverage)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = s->bands[i] * (1.0 - coverage) +
                  ink->bands[i] * illuminant->bands[i] * coverage;
}

static inline void
spectrum_lerp (Spectrum       *s,
               const Spectrum *a,
               const Spectrum *b,
               float          delta)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i] * (1.0 - delta) + b->bands[i] * delta;
}

static inline void
spectrum_lerp2 (Spectrum       *s,
                const Spectrum *a,
                const Spectrum *b,
                const Spectrum *delta)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i] * (1.0 - delta->bands[i]) + b->bands[i] * delta->bands[i];
}

static inline void
spectrum_add (Spectrum       *s,
              const Spectrum *a,
              const Spectrum *b)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i] + b->bands[i];
}

static inline void
spectrum_add_factor (Spectrum       *s,
                     const Spectrum *a,
                     const Spectrum *b,
                     float          factor)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i] + b->bands[i] * factor;
}

static inline void
spectrum_set (Spectrum       *s,
              const Spectrum *a)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i];
}


static inline void
add_ink (Spectrum       *s,
         const Spectrum *illuminant,
         const Spectrum *ink,
         const Spectrum *opaqueness,
         float          coverage,
         float          dot_gain)
{
  Spectrum opaque = *s;
  coverage = pow (coverage, dot_gain);

  /* XXX: could have shortcuts for non-opaque/fully opaque inks */
  spectrum_add_opaque_ink (&opaque, ink, illuminant, coverage);
  /* additive mixing */

  spectrum_remove_light (s, ink, coverage);
  /* subtractive mixing  */
  
  /* linear interpolation between additive and subtractive mixing;
   * this simplification is a gap in reality; how it compares to
   * actual prints remains to be seen.
   */
  //spectrum_lerp (s, s, &opaque, opacity);
  spectrum_lerp2 (s, s, &opaque, opaqueness);
}

static float spectrum_integrate (const Spectrum *s,
                                 const Spectrum *is,
                                 float           scale)
{
  float result = 0.0;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s->bands[i] * is->bands[i];
  return (result / SPECTRUM_BANDS) * scale;
}

static inline void
spectrum_to_rgba (const Spectrum *spec,
                  float         *rgba)
{
  float l, m, s;
  l = spectrum_integrate (spec, &STANDARD_OBSERVER_L, 0.62);
  m = spectrum_integrate (spec, &STANDARD_OBSERVER_M, 0.59);
  s = spectrum_integrate (spec, &STANDARD_OBSERVER_S, 0.66);
  /* this LMS to linear RGB -- likely not quite right..  */
  rgba[0] = ((30.83) * l + (-29.83) * m + (1.61) * s);
  rgba[1] = ((-6.481) * l + (17.7155) * m + (-2.532) * s);
  rgba[2] = ((-0.375) * l + (-1.19906) * m + (14.27) * s);
  rgba[3] = 1.0;
}

#ifdef KUBELKA_MUNK

static float inv_coth (float x)
{
  return 0.5 * log ( ( x  + 1 ) / (x-1));
}

inline static void ssim_compute_ks (float on_white,
                                   float on_black,
                                   float *k,
                                   float *s)
{
  float a, b;
  if (on_black <= 0.0)
    on_black = 0.0001;
  if (on_white <= on_black)
    on_white = on_black + 0.001;
  if (on_white >= 1.0)
  {
    on_white = 0.9999;
  }
  if (on_black >= on_white)
  {
    on_black = on_white - 0.0001;
  }
  a= 0.5 * (on_white + (on_black - on_white + 1) / (on_black));
  b = sqrt (a * a - 1);
  *s = 1/b * inv_coth ((b * b - (a - on_white) * (a - 1)) / (b * (1 - on_white)));
  *k = *s * (a - 1);
}

static inline float ssim_ks_to_reflectance (float k, float s)
{
  float ks = (k/s);
  return sqrt(1 + (ks) - sqrt((ks)*(ks)+2*ks));
}

#endif

static inline void ssim_compute_opacity (float white, float black,
                                         float *k, float *s)
{
  //black = 0 * (1-opacity) + white * opacity;
  //black = white * opacity;
  if (white == 0.0)
    white = 0.001;
  //opacity = black / white;
  *k = black / white;
}

/* compute kubelka munks constants
 */
static void color_recompute (SSim *ssim, Ink *ink)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    {
      float white = ink->on_white.bands[i];
      float black = ink->on_black.bands[i];
      float s, k;


#ifdef KUBELKA_MUNK
      ssim_compute_ks (white, black, &k, &s);
#else
      ssim_compute_opacity (white, black, &k, &s);
      s = 0;
#endif

      ink->k.bands[i] = k;
      ink->s.bands[i] = s;
    }
}

static inline Spectrum
inks_to_spectrum_continous (SSim  *ssim,
                            float *ink_levels)
{
  int i;
  Spectrum spec = ssim->illuminant;
#ifdef KUBELKA_MUNK
  Spectrum k, s;
  int b;
/*
  for (b = 0; b < SPECTRUM_BANDS; b++)
  {
    float k,s;
    k = 0.001;//001027;// * 0.01;
    s = 0.01;//0.009110460; // * 0.01;

    for (i = 0; i < ssim->inks; i++) 
      if (ink_levels[i] > 0.001)
      {
        k += ssim->ink_def[i].k.bands[b] *
           ink_levels[i] * ssim->ink_def[i].scale;
        s += ssim->ink_def[i].s.bands[b] *
           ink_levels[i] * ssim->ink_def[i].scale;
      }
    spec.bands[b] = ssim_ks_to_reflectance (k, s) * ssim->illuminant.bands[b];
  }
*/
  for (b = 0; b < SPECTRUM_BANDS; b++)
  {
    k.bands[b] = 0.001;
    s.bands[b] = 0.01;
  }
  for (i = 0; i < ssim->inks; i++) 
  {
    float level = ink_levels[i] * ssim->ink_def[i].scale;

    if (level > 0.00001)
    {
      level = pow (level, ssim->ink_def[i].dot_gain);
      for (b = 0; b < SPECTRUM_BANDS; b++)
      {
        k.bands[b] += ssim->ink_def[i].k.bands[b] * level;
        s.bands[b] += ssim->ink_def[i].s.bands[b] * level;
      }
    }
  }
  for (b = 0; b < SPECTRUM_BANDS; b++)
    spec.bands[b] = ssim_ks_to_reflectance (k.bands[b], s.bands[b]) * ssim->illuminant.bands[b];
#else
  spectrum_remove_light (&spec, &ssim->substrate, 1.0);

  for (i = 0; i < ssim->inks; i++) 
    add_ink (&spec, &ssim->illuminant,
                    &ssim->ink_def[i].on_white,
                    &ssim->ink_def[i].k,
                    ink_levels[i] * ssim->ink_def[i].scale,
                    ssim->ink_def[i].dot_gain);

#endif
  return spec;
}

/* Compute coverages:
 *
 * This implements the average amount of each combination of binary
 * coverage combinations of inks.
 *
 * 2: inks
 *
 * ab_coverage = ink_a * ink_b
 * a_coverage  = ink_a - ab_coverage
 * b_coverage  = ink_b - ab_coverage
 * 0_coverage  = 1 - full_coverage - a_coverage - b_coverage;
 *
 * 3: inks
 *
 * abc_coverage  = ink_a * ink_b * ink_c
 * ab_coverage   = ink_a * ink_b - abc_coverage;
 * ac_coverage   = ink_a * ink_c - abc_coverage;
 * bc_coverage   = ink_b * ink_c - abc_coverage;
 * a_coverage    = ink_a - abc_coverage - ab_coverage - ac_coverage;
 * b_coverage    = ink_b - abc_coverage - ab_coverage - bc_coverage;
 * c_coverage    = ink_c - abc_coverage - ac_coverage - bc_coverage;
 * 0_coverage    = 1 - full_coverage - 
 *                     a_coverage - b_coverage - c_coverage -
 *                     ab_coverage - ac_coverage - bc_coverage;
 *
 *
 * 4: inks
 *
 */

#define BITSET(var,pos) ((var) & (1<<(pos)))
static inline void compute_coverages (int          inks,
                                      const float *ink_levels,
                                      float       *coverages)
{
  int i;
  float tsum = 0;
  for (i = (1 << inks)-1; i > 0; i--)
  {
    int j;
    float sum = 0;
    for (j = 0; j < inks; j++)
    {
      if (BITSET (i, j))
      {
        if (sum == 0)
          sum = ink_levels[j];
        else
          sum *= ink_levels[j];
      }
    }
    for (j = (1 <<inks)-1; j > i; j --)
    {
      if (i &&  (  (j & i) == i))
        sum -= coverages[j];
    }
    coverages[i] = sum;
    tsum += sum;
  }
  coverages[0] = 1.0 - tsum;
}

static void compute_neugebauer_primaries (SSim *ssim)
{
  int i;
  for (i = (1 << ssim->inks)-1; i >= 0; i--)
  {
    float ink_levels[SSIM_MAX_INKS];
    int j;
    for (j = 0; j < ssim->inks; j++)
    {
      if (BITSET (i, j))
        ink_levels[j] = 1.0;
      else
        ink_levels[j] = 0.0;
    }
    {
    Spectrum spec = inks_to_spectrum_continous (ssim, ink_levels);
    spectrum_set (&ssim->mixtures[i], &spec);
    }
  }
}


static inline Spectrum
inks_to_spectrum (SSim *ssim,
                  float    *ink_levels)
{
  if (ssim->halftone_sim)
  {
    int i;
    Spectrum sum = {{0,}};
    float   coverages[1 << ssim->inks];
    compute_coverages (ssim->inks, ink_levels, coverages);

    for (i = 0; i < (1 << ssim->inks); i++)
    {
      spectrum_add_factor (&sum, &sum, &ssim->mixtures[i], coverages[i]);
    }
    return sum;
  }
  return inks_to_spectrum_continous (ssim, ink_levels);
}

void
ssim_proof (SSim *ssim,
            float *ink_levels,
            float *rgba)
{
  Spectrum spec = inks_to_spectrum (ssim, ink_levels);
  spectrum_to_rgba (&spec, rgba);

  /* XXX: todo : try to add LUT here as well */
}

static inline float
colordiff (float *rgb_a,
           float *rgb_b)
{
  return 
    (rgb_a[0]-rgb_b[0])*(rgb_a[0]-rgb_b[0])+
    (rgb_a[1]-rgb_b[1])*(rgb_a[1]-rgb_b[1])+
    (rgb_a[2]-rgb_b[2])*(rgb_a[2]-rgb_b[2]);
}

static inline void
ssim_separate_stochastic (SSim   *ssim,
                          float  *rgb,
                          float  *ink_levels,
                          int     iterations,
                          float   rrange)
{
  float best[SSIM_MAX_INKS] = {};
  float bestdiff = 1000.0;
  int i;

  for (i = 0; i < ssim->inks; i++)
    best[i] = ink_levels[i];  /* start with no inks; seek towards right color/spectra
                     * by adding - not removing
                     *
                     * starting with a known good neighbour - if available
                     * from the LUT; should improve uniformity an decrease
                     * warp in LUT.
                     */

  for (i = 0; i < iterations; i++)
  {
    int j;
    float attempt[SSIM_MAX_INKS];
    float softrgb[4];
    float diff;
    float inksum = 0;

    do {
      inksum = 0.0;
      for (j = 0; j < ssim->inks; j++)
      {
        attempt[j] = best[j] + ((random()%10000)/5000.0-1.0) * rrange;
        attempt[j] = CLAMP(attempt[j],0,1);
        inksum += attempt[j];
      }
    } while (inksum > ssim->ink_limit);

    ssim_proof (ssim, attempt, softrgb);
    diff = colordiff (rgb, softrgb) + (inksum / 50.0) * (inksum / 50.0);
    if (diff < bestdiff)
    {
      bestdiff = diff;
      for (j = 0; j < ssim->inks; j++)
        best[j] = attempt[j];
      if (diff < 0.0001) /* close enough */
        break;
    }
  }

  for (i = 0; i < ssim->inks; i++)
    ink_levels[i] = best[i];
}

static inline float *
ensure_lut (SSim *ssim,
            int    ri,
            int    gi,
            int    bi)
{
  int l_index;
  l_index = lut_index (ri, gi, bi);
  if (!ssim->lut[l_index].defined)
  {
    float trgb[3] = {(float)ri / LUT_DIM,
                      (float)gi / LUT_DIM,
                      (float)bi / LUT_DIM };

    /* feed in relevant neighbour if set */
    ssim_separate_stochastic (ssim, trgb, &ssim->lut[l_index].level[0], STOCHASTIC_ITERATIONS, STOCHASTIC_DIFFUSION);
    ssim->lut[l_index].defined = 1;
  }
  return &ssim->lut[l_index].level[0];
}

static inline void
lerp_inks (int          inks,
           float       *ink_res,
           const float *inka,
           const float *inkb,
           float        delta)
{
  int i;
  for (i = 0; i < inks; i++)
    ink_res[i] = inka[i]  * (1.0 - delta) + inkb[i] * delta;
}


void ssim_separate (SSim  *ssim,
                    float *rgb,
                    float *ink_levels)
{
  float rdelta, gdelta, bdelta;
  int ri = lut_indice (rgb[0], &rdelta);
  int gi = lut_indice (rgb[1], &gdelta);
  int bi = lut_indice (rgb[2], &bdelta);
  float *ink_corner[8];
  float  temp1[SSIM_MAX_INKS];
  float  temp2[SSIM_MAX_INKS];
  float  temp3[SSIM_MAX_INKS];
  float  temp4[SSIM_MAX_INKS];

/* numbering of corners, and positions of R,G,B axes
      6
      /\
    /   \
 7/   d  \5
  |\     /|
  |  \4/  |
 3\B G|2  |
   \  |  /1
     \|/R
      0       */

  ink_corner[0] = ensure_lut (ssim, ri + 0, gi + 0, bi + 0);
  ink_corner[1] = ensure_lut (ssim, ri + 1, gi + 0, bi + 0);
  ink_corner[2] = ensure_lut (ssim, ri + 1, gi + 0, bi + 1);
  ink_corner[3] = ensure_lut (ssim, ri + 0, gi + 0, bi + 1);

  ink_corner[4] = ensure_lut (ssim, ri + 0, gi + 1, bi + 0);
  ink_corner[5] = ensure_lut (ssim, ri + 1, gi + 1, bi + 0);
  ink_corner[6] = ensure_lut (ssim, ri + 1, gi + 1, bi + 1);
  ink_corner[7] = ensure_lut (ssim, ri + 0, gi + 1, bi + 1);

  lerp_inks (ssim->inks, temp1, ink_corner[0], ink_corner[1], rdelta);
  lerp_inks (ssim->inks, temp2, ink_corner[3], ink_corner[2], rdelta);
  lerp_inks (ssim->inks, temp3, ink_corner[4], ink_corner[5], rdelta);
  lerp_inks (ssim->inks, temp4, ink_corner[7], ink_corner[6], rdelta);
  lerp_inks (ssim->inks, temp1, temp1, temp3, gdelta);
  lerp_inks (ssim->inks, temp2, temp2, temp4, gdelta);
  lerp_inks (ssim->inks, ink_levels, temp1, temp2, bdelta);
}

#if 0
static int str_has_prefix (const char *str, const char *prefix)
{
  int i;
  for (i = 0; prefix[i] && str[i]; i++)
  {
    if (prefix[i] != str[i])
      return 0;
  }
  if (prefix[i] == 0)
    return 1;
  return 0;
}
#endif

static Spectrum parse_spectrum (SSim *ssim, char *spectrum)
{
  Spectrum s;
  char key[32];
  const Spectrum *tmp;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s.bands[i] = 1;
  
  if (!spectrum)
    return s;
  while (*spectrum == ' ') spectrum ++;
  for (i = 0; spectrum[i] && spectrum[i]!=' '; i++)
    key[i] = spectrum[i];
  key[i]=0;

  if (!strcmp (key, "rgb"))
  {
    Spectrum red   = *ssim_get_spectrum (ssim, "red");
    Spectrum green = *ssim_get_spectrum (ssim, "green");
    Spectrum blue  = *ssim_get_spectrum (ssim, "blue");
    int i;
    float r = 0, g = 0, b = 0;
    char *p = spectrum + 3;

    r = strtod (p, &p);
    if (p) g = strtod (p, &p);
    if (p) b = strtod (p, &p);

    for (i = 0; i<SPECTRUM_BANDS; i++)
      s.bands[i] = red.bands[i] * r + green.bands[i] * g + blue.bands[i] * b;
  }

  tmp = ssim_get_spectrum (ssim, key);
  if (tmp)
  {
    s = *tmp;
  } 
  else
  {
    float num_array [100];
    int band;
    band = 0;
    do {
      num_array[band++] = strtod (spectrum, &spectrum);
    } while (spectrum && band <= 100);

    if (band > 3)
    {
      float nm_start = num_array[0];
      float nm_gap = num_array[1];
      float nm_scale = num_array[2];

      int i;
      int j;
      float nm = nm_start;
      for (i = 3; i < band; i++)
      {
        j = (int) ( (nm - SPECTRUM_START) / SPECTRUM_GAP);
        if (j >=0 && j < SPECTRUM_BANDS)
          {
            int k;
            for (k = j; k < SPECTRUM_BANDS; k++)
            s.bands[k] = num_array[i] * nm_scale;
          }
        nm += nm_gap;
      }

      j = (int) ( (nm - SPECTRUM_START) / SPECTRUM_GAP);
      if (j >=0 && j < SPECTRUM_BANDS)
       {
         int k;
         for (k = j; k < SPECTRUM_BANDS; k++)
          s.bands[k] = 0.0;
       }
    }
  }

  return s;
}

const Spectrum *ssim_get_spectrum (SSim *ssim, const char *name)
{
  int i;
  if (!strcmp (name, "illuminant")) return &ssim->illuminant;
  if (!strcmp (name, "substrate")) return &ssim->substrate;
  if (!strcmp (name, "observer_l")) { return &STANDARD_OBSERVER_L; }
  if (!strcmp (name, "observer_m")) { return &STANDARD_OBSERVER_M; }
  if (!strcmp (name, "observer_s")) { return &STANDARD_OBSERVER_S; }

  for (i = 0; i < ssim->db.count; i++)
  {
    if (!strcmp (name, ssim->db.name[i]))
    {
      return &ssim->db.spectrum[i];
    }
  }
  return NULL;
}

void ssim_set_spectrum (SSim *ssim, const char *name, Spectrum *spectrum);
void ssim_set_spectrum (SSim *ssim, const char *name, Spectrum *spectrum)
{
  int i;

  if (!strcmp (name, "illuminant")) { ssim->illuminant = *spectrum; return; }
  if (!strcmp (name, "substrate"))  { ssim->substrate = *spectrum; return; }
  if (!strcmp (name, "observer_l")) { STANDARD_OBSERVER_L = *spectrum; return; }
  if (!strcmp (name, "observer_m")) { STANDARD_OBSERVER_M = *spectrum; return; }
  if (!strcmp (name, "observer_s")) { STANDARD_OBSERVER_S = *spectrum; return; }

  for (i = 0; i < ssim->db.count; i++)
  {
    if (!strcmp (name, ssim->db.name[i]))
    {
      ssim->db.spectrum[i] = *spectrum;
      return;
    }
  }
  if (ssim->db.count >= SPECTRUM_DB_SIZE-1)
  {
    /* eeek */
    return;
  }
  i = ssim->db.count;
  strncpy (ssim->db.name[i], name, 32);
  ssim->db.spectrum[i] = *spectrum;
  ssim->db.count++;
}


static void parse_config_line (SSim   *ssim,
                               const char *line)
{
  char *key;
  char *p;
  Spectrum s;
  int i;
  if (!line)
    return;

  if (!strchr (line, '=')) /* lines without = are simply skipped */
    return;
  while (*line == ' ') line++;

  key = strdup (line);
  p = strchr (key, '=');

  while (*p == '=' || *p == ' ')
  {
    *p = '\0';
    p--;
  }


  if (!strcmp (key, "inklimit"))
    {
      ssim->ink_limit = strchr(line, '=') ? strtod (strchr (line, '=')+1, NULL) : 3.0;
      if (ssim->ink_limit < 0.2)
        ssim->ink_limit = 0.2;
      free (key);
      return;
    }
  else if (!strcmp (key, "debugwidth"))
    {
      ssim->debug_width= strchr(line, '=') ? strtod (strchr (line, '=')+1, NULL) : 25;
      free (key);
      return;
    }
  else if (!strcmp (key, "halftone"))
    {
      ssim->halftone_sim = strchr(line, '=') ? strtod (strchr (line, '=')+1, NULL) : 0;
      free (key);
      return;
    }
  else if (!strcmp (key, "iterations"))
    {
      STOCHASTIC_ITERATIONS = (strchr(line, '=') ? atoi (strchr (line, '=')+1) : 42);
      free (key);
      return;
    }
  else if (!strcmp (key, "diffusion"))
    {
      STOCHASTIC_DIFFUSION = strchr(line, '=') ?
                 strtod (strchr (line, '=')+1, NULL) : 0;
      free (key);
      return;
    }

  s = parse_spectrum (ssim, strchr (line, '=') +1);

  {
    ssim_set_spectrum (ssim, key, &s);
    for (i = 0; i < SSIM_MAX_INKS; i++)
    {
      Ink *ink = &ssim->ink_def[i];
      char prefix[20];
      sprintf (prefix, "ink%i", i+1);
      if (!strcmp (key, prefix))
        {
          ink->on_white = s;
          ink->on_black = s; /* default to making paints, not inks */
          memset (&ink->on_black, 0, sizeof (Spectrum));
          if (strstr(line, "g=")) ink->dot_gain = strtod (strstr(line, "g=") + 2, NULL);
          if (strstr(line, "s=")) ink->scale = strtod (strstr(line, "s=") + 2, NULL);
          ssim->inks = MAX(ssim->inks, i+1);
          
          free (key);

          color_recompute (ssim, ink);
          return;
        }

      sprintf (prefix, "ink%i.black", i+1);
      if (!strcmp (key, prefix))
        {
          ink->on_black = s;
          if (strstr(line, "g=")) ink->dot_gain = strtod (strstr(line, "g=") + 2, NULL);
          if (strstr(line, "s=")) ink->scale = strtod (strstr(line, "s=") + 2, NULL);
          ssim->inks = MAX(ssim->inks, i+1);
          
          free (key);
          color_recompute (ssim, ink);
          return;
        }
    }
  }
  free (key);
}


static void
ssim_reset (SSim *ssim)
{
  int i;
  memset (ssim, 0, sizeof (SSim));
  for (i = 0; i < SSIM_MAX_INKS; i++)
    {
      ssim->ink_def[i].scale = 1.0;
      ssim->ink_def[i].dot_gain = 1.0;
    }
  ssim->ink_limit = SSIM_MAX_INKS;
}

static void
ssim_parse_int (SSim *ssim, const char *p)
{
  char acc[4096]; /* XXX: not protected */
  int acci = 0;
  acci = 0;
  while (*p)
  {
    switch (*p)
    {
      case '\n':
        parse_config_line (ssim, acc);
        acci = 0;
        acc[acci] = 0;
        break;
      default:
        acc[acci++] = *p;
        acc[acci] = 0;
        break;
    }
    p++;
  }
}

static void
ssim_parse_config (SSim       *ssim,
                   const char *p)
{
  if (!p)
    return;
  
  if (ssim->src)
    {
      if (!strcmp (ssim->src, p))
        return;
      free (ssim->src);
      ssim->src = NULL;
    }

  ssim_reset (ssim);

  ssim->src = strdup (p);

  ssim_parse_int (ssim, config_internal);
  ssim_parse_int (ssim, p);

  compute_neugebauer_primaries (ssim);
  if (STOCHASTIC_DIFFUSION < 0.1)
    STOCHASTIC_DIFFUSION = 0.1;
  else if (!(STOCHASTIC_DIFFUSION < 100.0))
    STOCHASTIC_DIFFUSION = 100.0;
  
}

SSim *
ssim_new (const char *config)
{
  SSim *ssim = calloc (sizeof (SSim), 1);
  ssim_parse_config (ssim, config);
  return ssim;
}

void
ssim_destroy (SSim *ssim)
{
  if (ssim->src)
    {
      free (ssim->src);
      ssim->src = NULL;
    }
  free (ssim);
}


int   ssim_get_ink_count    (SSim *ssim)
{
  return ssim->inks;
}

void ssim_set_ink_count    (SSim *ssim, int count)
{
  ssim->inks = count;
}

float ssim_get_ink_limit (SSim *ssim)
{
  return ssim->inks;
}

void ssim_set_ink_limit (SSim *ssim, float limit)
{
  ssim->ink_limit = limit;
}

void  ssim_set_halftone_sim (SSim *ssim,
                             int   enabled)
{
  ssim->halftone_sim = enabled;
}

int   ssim_get_halftone_sim (SSim *ssim)
{
  return ssim->halftone_sim;
}

/*********************/

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *input_format = gegl_operation_get_source_format (operation, "input");
  gint input_components = 1;
  if (input_format)
    input_components = babl_format_get_n_components (input_format);


  switch (o->mode)
  {
    case GEGL_SSIM_PROOF:
      gegl_operation_set_format (operation, "input",
        babl_format_n (babl_type("float"), input_components));
      gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
      break;
    case GEGL_SSIM_SEPARATE:
      //gegl_operation_set_format (operation, "output", 
      //  babl_format_n (babl_type("float"), ssim_get_ink_count (o->user_data)));
      gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));

      gegl_operation_set_format (operation, "input",
          babl_format ("RGBA float"));
      break;
    default:
    case GEGL_SSIM_SEPARATE_PROOF:
      gegl_operation_set_format (operation, "output", babl_format ("RGB float"));

      gegl_operation_set_format (operation, "input",
          babl_format ("RGBA float"));
      break;
  }

#ifdef USE_UI
  float color[4];
  GString *conf_str;
  conf_str = g_string_new ("illuminant=D65\n");

  gegl_color_get_pixel (o->substrate_color, babl_format ("RGBA float"), color);
  g_string_append_printf (conf_str, "substrate = rgb %f %f %f\n",
      color[0], color[1], color[2]);

  gegl_color_get_pixel (o->ink1_color, babl_format ("RGBA float"), color);
  g_string_append_printf (conf_str, "ink1 = rgb %f %f %f o=%f\n",
      color[0], color[1], color[2], color[3]);

  gegl_color_get_pixel (o->ink2_color, babl_format ("RGBA float"), color);
  g_string_append_printf (conf_str, "ink2 = rgb %f %f %f o=%f\n",
      color[0], color[1], color[2], color[3]);

  gegl_color_get_pixel (o->ink3_color, babl_format ("RGBA float"), color);
  g_string_append_printf (conf_str, "ink3 = rgb %f %f %f o=%f\n",
      color[0], color[1], color[2], color[3]);

  gegl_color_get_pixel (o->ink4_color, babl_format ("RGBA float"), color);
  g_string_append_printf (conf_str, "ink4 = rgb %f %f %f o=%f\n",
      color[0], color[1], color[2], color[3]);

  g_string_append_printf (conf_str, "inklimit=%f\n", o->ink_limit);

  if (o->user_data)
    ssim_destroy (o->user_data);
  o->user_data = ssim_new (conf_str->str);

  g_string_free (conf_str, TRUE);
#else

  if (o->user_data)
    ssim_destroy (o->user_data);
  o->user_data = ssim_new (o->config);
#endif
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat *in  = in_buf;
  gfloat *out = out_buf;
  SSim *ssim = o->user_data;
  int in_components = 
    babl_format_get_n_components (gegl_operation_get_format (op, "input"));

  switch (o->mode)
  {
    case GEGL_SSIM_PROOF:
      {
        if (ssim_get_ink_count (ssim) > 3)
        {
          while (samples--)
            {
              gfloat inks[4];
              int i;
              for (i = 0; i < 4; i++)
                inks[i] = in[i];
              ssim_proof  (ssim, inks, out);
              in  += in_components;
              out += 4;
            }
        }
        else
        {
          while (samples--)
            {
              ssim_proof  (ssim, in, out);
              in  += in_components;
              out += 4;
            }
        }
      }
      break;
    case GEGL_SSIM_SEPARATE:
      /* eeek hard coded for rgba output */
    while (samples--)
      {
        int i;
        int ink_count = ssim_get_ink_count (ssim);
        gfloat inks[SSIM_MAX_INKS];
        ssim_separate (ssim, in, inks);
        for (i = 0; i < MIN(4, ink_count); i++)
          out[i] = inks[i];
        if (ink_count < 4)
          out[3] = 1.0;
        if (ink_count < 3)
          out[2] = 0.0;
        if (ink_count < 2)
          out[1] = 0.0;
        in  += in_components;
        out += 4;
      }
      break;
    case GEGL_SSIM_SEPARATE_PROOF:
      if (o->debug_width)
      {
        gint x = roi->x;
        gint y = roi->y;
        while (samples--)
          {
            gfloat inks[SSIM_MAX_INKS];
            gint foo = ((x+y)/o->debug_width);
            gint actual_inks = ssim_get_ink_count (ssim);
            ssim_separate (ssim, in, inks);
            ssim_set_ink_count (ssim, MIN(foo, actual_inks));
            ssim_proof (ssim, inks, out);
            ssim_set_ink_count (ssim, actual_inks);

            in  += in_components;
            out += 3;

            x++; if (x>=roi->x+roi->width){x=roi->x;y++;}
          }
      }
      else
      {
        while (samples--)
          {
            gfloat inks[SSIM_MAX_INKS];
            ssim_separate (ssim, in, inks);
            ssim_proof (ssim, inks, out);

            in  += in_components;
            out += 3;
          }
      }
      break;
  }
  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);
  if (o->user_data)
  {
    ssim_destroy (o->user_data);
    o->user_data = NULL;
  }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  GObjectClass                  *gobject_class;
  
  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
  point_filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:ink-simulator",
  "title"       , _("Ink Simulator"),
  "categories"  , "misc",
  "description" ,
        _("Spectral ink and paint simulator, for softproofing/simulating physical color mixing and interactions."
          ""),
        NULL);
}


#endif
