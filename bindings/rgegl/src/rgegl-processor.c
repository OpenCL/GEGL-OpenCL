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
}
