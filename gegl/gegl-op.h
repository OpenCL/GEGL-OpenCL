#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-node.h"
#include "gegl-attributes.h"

#define GEGL_TYPE_OP               (gegl_op_get_type ())
#define GEGL_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OP, GeglOp))
#define GEGL_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OP, GeglOpClass))
#define GEGL_IS_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OP))
#define GEGL_IS_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OP))
#define GEGL_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OP, GeglOpClass))

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif
struct _GeglOp 
{
    GeglNode node;

    /*< private >*/
    GeglAttributes ** attributes;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass 
{
    GeglNodeClass node_class;
  
    void (*init_attributes)      (GeglOp *self);

};
                 
GType           gegl_op_get_type                (void);
void            gegl_op_apply                   (GeglOp * self);
void            gegl_op_apply_image             (GeglOp * self,
                                                 GeglOp * image,
                                                 GeglRect *roi);
void            gegl_op_apply_roi               (GeglOp * self, 
                                                 GeglRect *roi);

void            gegl_op_init_attributes         (GeglOp * self);
GeglAttributes* gegl_op_get_nth_attributes      (GeglOp *self, 
                                                 gint n); 
GList  *        gegl_op_get_attributes          (GeglOp *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
