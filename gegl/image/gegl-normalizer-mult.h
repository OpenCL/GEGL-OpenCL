/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#ifndef GEGL_NORMALIZER_MULT_H
#define GEGL_NORMALIZER_MULT_H

#include "gegl-normalizer.h"

#define GEGL_TYPE_NORMALIZER_MULT               (gegl_normalizer_mult_get_type ())
#define GEGL_NORMALIZER_MULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NORMALIZER_MULT, GeglNormalizerMult))
#define GEGL_NORMALIZER_MULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NORMALIZER_MULT, GeglNormalizerMultClass))
#define GEGL_IS_NORMALIZER_MULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NORMALIZER_MULT))
#define GEGL_IS_NORMALIZER_MULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NORMALIZER_MULT))
#define GEGL_NORMALIZER_MULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NORMALIZER_MULT, GeglNormalizerMultClass))

GType gegl_normalizer_mult_get_type (void) G_GNUC_CONST;

typedef struct _GeglNormalizerMult GeglNormalizerMult;
struct _GeglNormalizerMult
{
  GeglNormalizer parent;
  gdouble alpha;
};

typedef struct _GeglNormalizerMultClass GeglNormalizerMultClass;
struct _GeglNormalizerMultClass
{
  GeglNormalizerClass parent_class;
};

#endif
