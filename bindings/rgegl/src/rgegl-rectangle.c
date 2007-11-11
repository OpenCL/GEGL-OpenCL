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
 * License along with rgegl; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * 2007 © Øyvind Kolås.
 */

#include "rgegl.h"
#include "gegl/gegl-utils.h"

/************************************************

  This file is derived from rbgdkrectangle.c in ruby-gtk2
  which has the following copyright:

  Copyright (C) 2002,2003 Masao Mutoh

  This file is derived from rbgdkregion.c.
  rbgdkregion.c -
  Copyright (C) 1998-2000 Yukihiro Matsumoto,
                          Daisuke Kanda,
                          Hiroshi Igarashi
************************************************/

#define _SELF(r) ((GeglRectangle*)RVAL2BOXED(r, GEGL_TYPE_RECTANGLE))

static VALUE
geglrect_initialize(self, x, y, width, height)
    VALUE self, x, y, width, height;
{
    GeglRectangle new;

    new.x = NUM2INT(x);
    new.y = NUM2INT(y);
    new.width = NUM2INT(width);
    new.height = NUM2INT(height);

    G_INITIALIZE(self, &new);
    return Qnil;
}
#if 0
static VALUE
geglrect_intersect(self, other)
    VALUE self, other;
{
    GeglRectangle dest;
    gboolean ret = gegl_rect_intersect(&dest, _SELF(self), _SELF(other));
    return ret ? BOXED2RVAL(&dest, GEGL_TYPE_RECTANGLE) : Qnil;
}

static VALUE
geglrect_union(self, other)
    VALUE self, other;
{
    GeglRectangle dest;
    gegl_rect_bounding_box (&dest, _SELF(self), _SELF(other));
    return BOXED2RVAL(&dest, GEGL_TYPE_RECTANGLE);
}
#endif

/* Struct accessors */
static VALUE
geglrect_x(self)
    VALUE self;
{
    return INT2NUM(_SELF(self)->x);
}

static VALUE
geglrect_y(self)
    VALUE self;
{
    return INT2NUM(_SELF(self)->y);
}

static VALUE
geglrect_w(self)
    VALUE self;
{
    return INT2NUM(_SELF(self)->width);
}

static VALUE
geglrect_h(self)
    VALUE self;
{
    return INT2NUM(_SELF(self)->height);
}

static VALUE
geglrect_set_x(self, x)
    VALUE self, x;
{
    _SELF(self)->x = NUM2INT(x);
    return self;
}

static VALUE
geglrect_set_y(self, y)
    VALUE self, y;
{
    _SELF(self)->y = NUM2INT(y);
    return self;
}

static VALUE
geglrect_set_w(self, width)
    VALUE self, width;
{
    _SELF(self)->width = NUM2INT(width);
    return self;
}

static VALUE
geglrect_set_h(self, height)
    VALUE self, height;
{
    _SELF(self)->height = NUM2INT(height);
    return self;
}

static VALUE
geglrect_to_a(self)
    VALUE self;
{
  GeglRectangle* a = _SELF(self);
  return rb_ary_new3(4, INT2FIX(a->x), INT2FIX(a->y),
                     INT2FIX(a->width), INT2FIX(a->height));
}

void
Init_gegl_rectangle(VALUE module)
{
    VALUE geglRectangle = G_DEF_CLASS(GEGL_TYPE_RECTANGLE, "Rectangle", module);

    rb_define_method(geglRectangle, "initialize", geglrect_initialize, 4);
#if 0
    rb_define_method(geglRectangle, "intersect", geglrect_intersect, 1);
    rb_define_alias(geglRectangle, "&", "intersect");
    rb_define_method(geglRectangle, "union", geglrect_union, 1);
    rb_define_alias(geglRectangle, "|", "union");
#endif
    rb_define_method(geglRectangle, "x", geglrect_x, 0);
    rb_define_method(geglRectangle, "y", geglrect_y, 0);
    rb_define_method(geglRectangle, "width", geglrect_w, 0);
    rb_define_method(geglRectangle, "height", geglrect_h, 0);
    rb_define_method(geglRectangle, "set_x", geglrect_set_x, 1);
    rb_define_method(geglRectangle, "set_y", geglrect_set_y, 1);
    rb_define_method(geglRectangle, "set_width", geglrect_set_w, 1);
    rb_define_method(geglRectangle, "set_height", geglrect_set_h, 1);
    rb_define_method(geglRectangle, "to_a", geglrect_to_a, 0);

    G_DEF_SETTERS(geglRectangle);
}
