#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#include "gegl-node.h"
#include "gegl-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_OP               (gegl_op_get_type ())
#define GEGL_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OP, GeglOp))
#define GEGL_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OP, GeglOpClass))
#define GEGL_IS_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OP))
#define GEGL_IS_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OP))
#define GEGL_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OP, GeglOpClass))

typedef struct _GeglOp GeglOp;
struct _GeglOp 
{
    GeglNode node;

    /*< private >*/
    GList *data_inputs;
    GList *data_outputs;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass 
{
    GeglNodeClass node_class;
};

GType           gegl_op_get_type                (void);
void            gegl_op_apply                   (GeglOp * self);
void            gegl_op_apply_roi               (GeglOp * self, 
                                                 GeglRect *roi);

void            gegl_op_append_data_input       (GeglOp *self,
                                                 GeglData *data);
void            gegl_op_append_data_output      (GeglOp *self,
                                                 GeglData *data);
void            gegl_op_append_input            (GeglOp *self,
                                                 GType data_type,
                                                 gchar *name);
void            gegl_op_append_output           (GeglOp *self, 
                                                 GType data_type,
                                                 gchar *name);

GeglData*       gegl_op_get_data_input          (GeglOp *self, 
                                                 gint n);
GeglData*       gegl_op_get_data_output         (GeglOp *self, 
                                                 gint n);
void            gegl_op_free_data_inputs        (GeglOp * self);
void            gegl_op_free_data_outputs       (GeglOp * self);

GList *         gegl_op_get_data_inputs         (GeglOp *self);
GList *         gegl_op_get_data_outputs        (GeglOp *self);

void            gegl_op_class_install_data_input_property  (GeglOpClass *class,
                                                 GParamSpec   *pspec);
GParamSpec*     gegl_op_class_find_data_input_property (GeglOpClass *class,
			                                     const gchar  *property_name);
GParamSpec**    gegl_op_class_list_data_input_properties (GeglOpClass *class,
                                                 guint *n_properties_p);
                 
void            gegl_op_class_install_data_output_property (GeglOpClass *class,
                                                 GParamSpec   *pspec);
GParamSpec*     gegl_op_class_find_data_output_property (GeglOpClass *class,
			                                     const gchar  *property_name);
GParamSpec**    gegl_op_class_list_data_output_properties (GeglOpClass *class,
                                                 guint  *n_properties_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
