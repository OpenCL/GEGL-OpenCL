#ifndef _RBGEGL_H
#define _RBGEGL_H 1
#include "ruby.h"
#include <gegl.h>
#include "rbgtk.h"
#include "rbgobject.h"
#include "rbgeglversion.h"

extern void Init_gegl(void);
extern void Init_gegl_buffer(VALUE);
extern void Init_gegl_color(VALUE);
extern void Init_gegl_node(VALUE);
extern void Init_gegl_processor(VALUE);
extern void Init_gegl_rectangle(VALUE);

#endif /* _RBGEGL_H */
