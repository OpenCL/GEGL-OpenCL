#ifndef __GEGL_ATTRIBUTES_H__
#define __GEGL_ATTRIBUTES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

#ifndef __TYPEDEF_GEGL_ATTRIBUTES__
#define __TYPEDEF_GEGL_ATTRIBUTES__
typedef struct _GeglAttributes GeglAttributes;
#endif

struct _GeglAttributes 
{
  GValue *value;
  GParamSpec *param_spec;
  GeglRect rect;
  GeglColorModel * color_model;
}; 

GeglAttributes*   gegl_attributes_new(); 

void              gegl_attributes_set_rect          (GeglAttributes * attributes,
                                                     GeglRect *rect);
void              gegl_attributes_get_rect          (GeglAttributes * attributes,
                                                     GeglRect *rect);
void              gegl_attributes_set_color_model   (GeglAttributes * attributes,
                                                     GeglColorModel * color_model);
GeglColorModel*   gegl_attributes_get_color_model   (GeglAttributes * attributes);

void              gegl_attributes_print             (GeglAttributes * attributes, 
                                                     GeglOp *op);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
