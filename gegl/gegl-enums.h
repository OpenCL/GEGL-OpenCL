/* This file is part of GEGL
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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 *
 */

/* This file holds public enums from GEGL
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 *
 * Normally, gegl-enums.c file would be be generated my glib-mkenums,
 * but we use the enum values' registered names for translatable,
 * human readable labels for the GUI, so gegl-enums.c is maintained
 * manually.
 *
 * DON'T FORGET TO UPDATE gegl-enums.c AFTER CHANGING THIS HEADER
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 */

#ifndef __GEGL_ENUMS_H__
#define __GEGL_ENUMS_H__

G_BEGIN_DECLS

typedef enum {
  GEGL_ABYSS_NONE,
  GEGL_ABYSS_CLAMP,
  GEGL_ABYSS_LOOP,
  GEGL_ABYSS_BLACK,
  GEGL_ABYSS_WHITE
} GeglAbyssPolicy;

GType gegl_abyss_policy_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ABYSS_POLICY (gegl_abyss_policy_get_type ())


typedef enum {
  GEGL_ACCESS_READ      = 1 << 0,
  GEGL_ACCESS_WRITE     = 1 << 1,
  GEGL_ACCESS_READWRITE = (GEGL_ACCESS_READ | GEGL_ACCESS_WRITE)
} GeglAccessMode;

GType gegl_access_mode_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ACCESS_MODE (gegl_access_mode_get_type ())


typedef enum {
  GEGL_DITHER_NONE,
  GEGL_DITHER_FLOYD_STEINBERG,
  GEGL_DITHER_BAYER,
  GEGL_DITHER_RANDOM,
  GEGL_DITHER_RANDOM_COVARIANT,
  GEGL_DITHER_ARITHMETIC_ADD,
  GEGL_DITHER_ARITHMETIC_ADD_COVARIANT,
  GEGL_DITHER_ARITHMETIC_XOR,
  GEGL_DITHER_ARITHMETIC_XOR_COVARIANT,
} GeglDitherMethod;

GType gegl_dither_method_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_DITHER_METHOD (gegl_dither_method_get_type ())


typedef enum {
  GEGL_ORIENTATION_HORIZONTAL,
  GEGL_ORIENTATION_VERTICAL
} GeglOrientation;

GType gegl_orientation_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ORIENTATION (gegl_orientation_get_type ())


typedef enum {
  GEGL_SAMPLER_NEAREST,
  GEGL_SAMPLER_LINEAR,
  GEGL_SAMPLER_CUBIC,
  GEGL_SAMPLER_NOHALO,
  GEGL_SAMPLER_LOHALO
} GeglSamplerType;

GType gegl_sampler_type_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_SAMPLER_TYPE (gegl_sampler_type_get_type ())

G_END_DECLS

#endif /* __GEGL_ENUMS_H__ */
