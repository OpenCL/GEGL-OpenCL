#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-node.h"
#include "gegl-data.h"

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
    GList *input_data_list;
    GList *output_data_list;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass 
{
    GeglNodeClass node_class;
};

void            gegl_op_class_install_input_data_property  (GeglOpClass *class,
                                                 GParamSpec   *pspec);
GParamSpec*     gegl_op_class_find_input_data_property (GeglOpClass *class,
			                                     const gchar  *property_name);
GParamSpec**    gegl_op_class_list_input_data_properties (GeglOpClass *class,
                                                 guint *n_properties_p);
                 
void            gegl_op_class_install_output_data_property (GeglOpClass *class,
                                                 GParamSpec   *pspec);
GParamSpec*     gegl_op_class_find_output_data_property (GeglOpClass *class,
			                                     const gchar  *property_name);
GParamSpec**    gegl_op_class_list_output_data_properties (GeglOpClass *class,
                                                 guint  *n_properties_p);

GType           gegl_op_get_type                (void);
void            gegl_op_apply                   (GeglOp * self);
void            gegl_op_apply_roi               (GeglOp * self, 
                                                 GeglRect *roi);

void            gegl_op_add_input_data         (GeglOp *self,
                                                 GeglData *data,
                                                 gint n);
void            gegl_op_add_output_data        (GeglOp *self,
                                                 GeglData *data,
                                                 gint n);
void            gegl_op_add_input               (GeglOp *self,
                                                 GType data_type,
                                                 gchar *name,
                                                 gint n);
void            gegl_op_add_output              (GeglOp *self, 
                                                 GType data_type,
                                                 gchar *name,
                                                 gint n);

GeglData*      gegl_op_get_input_data           (GeglOp *self, 
                                                 gint n);
GeglData*      gegl_op_get_output_data          (GeglOp *self, 
                                                 gint n);
void            gegl_op_free_input_data_list    (GeglOp * self);
void            gegl_op_free_output_data_list   (GeglOp * self);

GList *         gegl_op_get_input_data_list     (GeglOp *self);
GList *         gegl_op_get_output_data_list    (GeglOp *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
