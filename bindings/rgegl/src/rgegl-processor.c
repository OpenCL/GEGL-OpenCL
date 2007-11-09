/* rgegl - a ruby binding for GEGL
 *
 * rgegl is free software; you can redistribute it and/or
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
 * License along with rgegl; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * 2007 © Øyvind Kolås.
 */

#include "rgegl.h"

#define _SELF(self) GEGL_PROCESSOR(RVAL2GOBJ(self))

static VALUE
cprocessor_work (self)
  VALUE self;
{
  gboolean cresult;
  cresult = gegl_processor_work (_SELF (self), NULL);
  return cresult?Qtrue:Qfalse;
}

static VALUE
cprocessor_set_rectangle (self, r_rectangle)
    VALUE self, r_rectangle;
{
    GeglRectangle *rectangle;
    rectangle = RVAL2BOXED (r_rectangle, GEGL_TYPE_RECTANGLE);
    gegl_processor_set_rectangle (_SELF (self), rectangle);
    return self;
}


void
Init_gegl_processor(mGegl)
    VALUE mGegl;
{
    VALUE geglGeglProcessor = G_DEF_CLASS(GEGL_TYPE_PROCESSOR, "Processor", mGegl);
    rb_define_method(geglGeglProcessor, "work", cprocessor_work, 0);
    rb_define_method(geglGeglProcessor, "rectangle=", cprocessor_set_rectangle, 1);
    G_DEF_SETTERS (geglGeglProcessor);
}
