#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#include "gegl-node.h"
#include "gegl-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*GeglValidateDataFunc)(GeglData* input_data, GeglData *other_data);

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
    GList *input_data_list;
    GList *output_data_list;
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
void            gegl_op_add_input_data          (GeglOp *self,
                                                 GType data_type,
                                                 gchar *name);
void            gegl_op_add_output_data         (GeglOp *self, 
                                                 GType data_type,
                                                 gchar *name);
gint            gegl_op_get_input_data_index    (GeglOp *self,
                                                 gchar *name);
gint            gegl_op_get_output_data_index   (GeglOp *self,
                                                 gchar *name);
GeglData*       gegl_op_get_nth_input_data      (GeglOp *self, 
                                                 gint n);
GeglData*       gegl_op_get_nth_output_data     (GeglOp *self, 
                                                 gint n);
GeglData*       gegl_op_get_input_data          (GeglOp *self, 
                                                 gchar *name);
GeglData*       gegl_op_get_output_data         (GeglOp *self, 
                                                 gchar *name);
GValue*         gegl_op_get_input_data_value    (GeglOp *self,
                                                 gchar *name);
void            gegl_op_set_input_data_value    (GeglOp *self, 
                                                 gchar *name,
                                                 const GValue *value);
GValue*         gegl_op_get_output_data_value   (GeglOp *self,
                                                 gchar *name);
void            gegl_op_set_output_data_value   (GeglOp *self, 
                                                 gchar *name,
                                                 const GValue *value);
GList *         gegl_op_get_input_data_list     (GeglOp *self);
GList *         gegl_op_get_output_data_list    (GeglOp *self);
void            gegl_op_append_input_data       (GeglOp *self,
                                                 GeglData *data);
void            gegl_op_append_output_data      (GeglOp *self,
                                                 GeglData *data);
void            gegl_op_free_input_data_list    (GeglOp * self);
void            gegl_op_free_output_data_list   (GeglOp * self);

void            gegl_op_validate_input_data     (GeglOp *op,
                                                 GList *collected_input_data_list,
                                                 GeglValidateDataFunc func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
