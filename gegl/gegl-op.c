#include "gegl-op.h"
#include "gegl-visitor.h"
#include "gegl-eval-mgr.h"
#include <string.h>

static void class_init (GeglOpClass * klass);
static void init (GeglOp * self, GeglOpClass * klass);
static void finalize(GObject * gobject);


static void free_data(GeglOp *self, GArray *array);
static GArray * add_data(GeglOp *self, GArray *array, GeglData *data);
static GeglData* get_nth_data(GeglOp *self, GArray *array, gint n);
static GeglData* get_data(GeglOp *self, GArray *array, gchar * name);
static gint get_data_index(GeglOp *self, GArray *array, gchar * name);

static GList * make_data_list(GArray *array);

static void accept (GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

GType
gegl_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_NODE , 
                                     "GeglOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  node_class->accept = accept;
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  self->input_data_array = g_array_new(FALSE,FALSE,sizeof(GeglData*));
  self->output_data_array = g_array_new(FALSE,FALSE,sizeof(GeglData*));
}

static void
finalize(GObject *gobject)
{
  GeglOp *self = GEGL_OP(gobject);

  gegl_op_free_input_data_array(self);
  g_array_free(self->input_data_array, TRUE);

  gegl_op_free_output_data_array(self);
  g_array_free(self->output_data_array, TRUE);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_op_apply:
 * @self: a #GeglOp
 *
 * Apply the op. 
 *
 **/
void      
gegl_op_apply(GeglOp * self)
{
  g_return_if_fail (GEGL_IS_OP (self));

  gegl_op_apply_roi(self, NULL);
}

/**
 * gegl_op_apply_roi
 * @self: a #GeglOp.
 * @roi: a #GeglRect.
 *
 * Apply the op for the roi. 
 *
 **/
void      
gegl_op_apply_roi(GeglOp * self, 
                  GeglRect *roi)
{
  GeglEvalMgr * eval_mgr;
  g_return_if_fail (GEGL_IS_OP (self));

  eval_mgr = g_object_new(GEGL_TYPE_EVAL_MGR,
                          "root", self,
                          "roi", roi,
                          NULL);

  gegl_eval_mgr_evaluate(eval_mgr);
  g_object_unref(eval_mgr);
}

/**
 * gegl_op_get_nth_input_data
 * @self: a #GeglOp.
 * @n: which data.
 *
 * Gets the nth input data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_nth_input_data(GeglOp *self,
                           gint n)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return get_nth_data(self, self->input_data_array, n);
}

/**
 * gegl_op_get_nth_output_data
 * @self: a #GeglOp.
 * @n: which data.
 *
 * Gets the nth output data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_nth_output_data(GeglOp *self,
                            gint n)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return get_nth_data(self, self->output_data_array, n);
}

static GeglData*
get_nth_data(GeglOp *self,
             GArray *array,
             gint n)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  if(n >= 0L && n < (gint)array->len)
    return g_array_index(array, GeglData*, n);

  return NULL;
}

/**
 * gegl_op_get_input_data
 * @self: a #GeglOp.
 * @name: name of input data.
 *
 * Gets the input data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_input_data(GeglOp *self,
                       gchar * name)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return get_data(self, self->input_data_array, name);
}

/**
 * gegl_op_get_output_data
 * @self: a #GeglOp.
 * @name: name of output data.
 *
 * Gets the output data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_output_data(GeglOp *self,
                        gchar * name)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return get_data(self, self->output_data_array, name);
}

static GeglData*
get_data(GeglOp *self,
               GArray *array,
               gchar * name)
{
  gint i;
  for(i = 0; i < (gint)array->len; i++)
    {
      GeglData *data = g_array_index(array, GeglData*, i); 
      const gchar * data_name = gegl_data_get_name(data);
      if (!strcmp(data_name, name))
        return data;
    }

  return NULL;
}
           
/**
 * gegl_op_get_input_data_array
 * @self: a #GeglOp.
 *
 * Get the input data array. 
 *
 * Returns: a GArray of #GeglData pointers.
 **/
GArray*
gegl_op_get_input_data_array(GeglOp *self)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->input_data_array; 
}

/**
 * gegl_op_get_output_data_array
 * @self: a #GeglOp.
 *
 * Get the output data array. 
 *
 * Returns: a GArray of #GeglData pointers.
 **/
GArray*
gegl_op_get_output_data_array(GeglOp *self)
{
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->output_data_array; 
}


/**
 * gegl_op_get_input_data_index
 * @self: a #GeglOp.
 * @name: name of input data.
 *
 * Get the input data index. 
 *
 * Returns: a integer.
 **/
gint
gegl_op_get_input_data_index(GeglOp *self,
                             gchar *name)
{
  g_return_val_if_fail (GEGL_IS_OP (self), -1);

  return get_data_index(self, self->input_data_array, name);
}

/**
 * gegl_op_get_output_data_index
 * @self: a #GeglOp.
 * @name: name of output data.
 *
 * Get the output data index. 
 *
 * Returns: a integer.
 **/
gint
gegl_op_get_output_data_index(GeglOp *self,
                              gchar *name)
{
  g_return_val_if_fail (GEGL_IS_OP (self), -1);

  return get_data_index(self, self->output_data_array, name);
}

static gint 
get_data_index(GeglOp *self,
                     GArray *array,
                     gchar * name)
{
  gint i;
  for(i = 0; i < (gint)array->len; i++)
    {
      GeglData *data = g_array_index(array, GeglData*, i); 
      const gchar * data_name = gegl_data_get_name(data);
      if (!strcmp(data_name, name))
        return i;
    }

  return -1;
}

/**
 * gegl_op_get_input_value
 * @self: a #GeglOp.
 *
 * Get the input data's value. 
 *
 * Returns: a #GValue.
 **/
GValue*
gegl_op_get_input_data_value(GeglOp *self,
                             gchar *name)
{
  GeglData * data;
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  data = get_data(self, self->input_data_array, name);

  if(!data) 
    return NULL;

  return gegl_data_get_value(data);
}


/**
 * gegl_op_set_input_data_value
 * @self: a #GeglOp.
 * @name: input data name 
 * @value: value to copy 
 *
 * Set the input data's value to this. 
 *
 **/
void
gegl_op_set_input_data_value(GeglOp *self,
                             gchar *name,
                             const GValue *value)
{
  GeglData * data;
  g_return_if_fail (GEGL_IS_OP (self));

  data = get_data(self, self->input_data_array, name);

  if(!data) 
    {
       g_warning("Couldnt find input data value %s\n", name);
       return;
    }

  gegl_data_copy_value(data, value); 
}

/**
 * gegl_op_get_output_value
 * @self: a #GeglOp.
 *
 * Get the output data's value. 
 *
 * Returns: a #GValue.
 **/
GValue*
gegl_op_get_output_data_value(GeglOp *self,
                              gchar *name)
{
  GeglData * data;
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  data = get_data(self, self->output_data_array, name);

  if(!data) 
    return NULL;

  return gegl_data_get_value(data);
}


/**
 * gegl_op_set_output_data_value
 * @self: a #GeglOp.
 * @name: output data name 
 * @value: value to copy 
 *
 * Set the output data's value to this. 
 *
 **/
void
gegl_op_set_output_data_value(GeglOp *self,
                              gchar *name,
                              const GValue *value)
{
  GeglData * data;
  g_return_if_fail (GEGL_IS_OP (self));

  data = get_data(self, self->output_data_array, name);

  if(!data) 
    {
       g_warning("Couldnt find output data value %s\n", name);
       return;
    }

  gegl_data_copy_value(data, value); 
}

/**
 * gegl_op_add_input_data
 * @self: a #GeglOp.
 * @data_type: the type of the data to create.
 * @name: the name of the data.
 *
 * Add an input with the passed data type and name. 
 *
 **/
void
gegl_op_add_input_data(GeglOp *self,
                     GType data_type,
                     gchar *name)
{
  GeglData * data;
  g_return_if_fail (GEGL_IS_OP (self));

  data = g_object_new(data_type,"data_name", name, NULL);
  gegl_op_append_input_data(self, data);
  g_object_unref(data);
}

/**
 * gegl_op_add_output_data
 * @self: a #GeglOp.
 * @data_type: the type of the data to create.
 * @name: the name of the data.
 *
 * Add an output with the passed data type and name. 
 *
 **/
void
gegl_op_add_output_data(GeglOp *self,
                      GType data_type,
                      gchar *name)
{
  GeglData *data;
  g_return_if_fail (GEGL_IS_OP (self));

  data = g_object_new(data_type,"data_name", name, NULL);
  gegl_op_append_output_data(self, data);
  g_object_unref(data);
}

/**
 * gegl_op_append_input_data
 * @self: a #GeglOp.
 * @data: the #GeglData to append.
 *
 * Append data to list of input data. 
 *
 **/
void
gegl_op_append_input_data(GeglOp *self,
                          GeglData *data)
{
  g_return_if_fail (GEGL_IS_OP (self));

  gegl_node_add_input(GEGL_NODE(self));
  self->input_data_array = add_data(self, self->input_data_array, data);
}

/**
 * gegl_op_append_output_data
 * @self: a #GeglOp.
 * @data: the #GeglData to append.
 *
 * Append data to list of output data. 
 *
 **/
void
gegl_op_append_output_data(GeglOp *self,
                           GeglData *data)
{
  g_return_if_fail (GEGL_IS_OP (self));

  gegl_node_add_output(GEGL_NODE(self));
  self->output_data_array = add_data(self, self->output_data_array, data);
}

static GArray *
add_data(GeglOp *self,
               GArray *array,
               GeglData *data)
{
  g_return_val_if_fail(self != NULL, NULL);

  if(data)
    g_object_ref(data);

  return g_array_append_val(array, data);
}

/**
 * gegl_op_free_input_data_list
 * @self: a #GeglOp.
 *
 * Free the list of input data. 
 *
 **/
void
gegl_op_free_input_data_list(GeglOp * self)
{
  g_return_if_fail (GEGL_IS_OP (self));

  free_data(self,self->input_data_array);
}

/**
 * gegl_op_free_input_data_array
 * @self: a #GeglOp.
 *
 * Free the array of input data. 
 *
 **/
void
gegl_op_free_input_data_array(GeglOp * self)
{
  g_return_if_fail (GEGL_IS_OP (self));

  free_data(self, self->input_data_array);
}

/**
 * gegl_op_free_output_data_list
 * @self: a #GeglOp.
 *
 * Free the list of output data. 
 *
 **/
void
gegl_op_free_output_data_list(GeglOp * self)
{
  g_return_if_fail (GEGL_IS_OP (self));

  free_data(self,self->output_data_array);
}

/**
 * gegl_op_free_output_data_array
 * @self: a #GeglOp.
 *
 * Free the array of output data. 
 *
 **/
void
gegl_op_free_output_data_array(GeglOp * self)
{
  g_return_if_fail (GEGL_IS_OP (self));

  free_data(self, self->output_data_array);
}

static void 
free_data(GeglOp *self,
                GArray *array)
{
  gint i;

  for(i = 0 ; i < (gint)array->len; i++)
    {
      GeglData *data = g_array_index(array, GeglData*, i);

      if(data)
        g_object_unref(data);
    }
}

static GList *
make_data_list(GArray *array)
{
  GList * list = NULL;
  gint i;

  for(i = 0 ; i < (gint)array->len; i++)
    {
      GeglData *data = g_array_index(array, GeglData*, i);
      list = g_list_append(list, data); 
    }
  return list;
}
 
void
gegl_op_validate_input_data_array(GeglOp *self,
                                  GArray *collected_array,
                                  GeglValidateDataFunc func)
{
  gint num_inputs;
  gint i;
  g_return_if_fail (GEGL_IS_OP (self));
  g_return_if_fail (collected_array);

  num_inputs = gegl_node_get_num_inputs(GEGL_NODE(self));

  g_assert((gint)collected_array->len == num_inputs); 

   /* 
     Traverse through the input data, calling 
     the passed function back with the input data
     and the corresponding collected input data. 
   */

  for(i = 0; i < num_inputs; i++)
  {
    GeglData *collected_data = g_array_index(collected_array, GeglData*, i);
    GeglData *input_data = gegl_op_get_nth_input_data(self, i);
    (*func)(input_data, collected_data);
  }
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_op(visitor, GEGL_OP(node));
}
