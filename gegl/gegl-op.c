#include "gegl-op.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-visitor.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-eval-mgr.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglOpClass * klass);
static void init (GeglOp * self, GeglOpClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void free_data(GeglOp *self, GList *list);
static GList * add_data(GeglOp *self, GList *list, GeglData *data);
static GeglData* get_data(GeglOp *self, GList *list, gint n);

static void accept (GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

static GParamSpecPool      *input_pspec_pool = NULL;
static GParamSpecPool      *output_pspec_pool = NULL;

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
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;

  input_pspec_pool = g_param_spec_pool_new (FALSE);
  output_pspec_pool = g_param_spec_pool_new (FALSE);
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  self->data_inputs = NULL;
  self->data_outputs = NULL;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglOp *self = GEGL_OP(gobject);

  gegl_op_free_data_inputs(self);
  gegl_op_free_data_outputs(self);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
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
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  {
    GeglEvalMgr * eval_mgr = g_object_new(GEGL_TYPE_EVAL_MGR,
                                          "root", self,
                                          "roi", roi,
                                          NULL);
    gegl_eval_mgr_evaluate(eval_mgr);
    g_object_unref(eval_mgr);
  }
}

/**
 * gegl_op_get_data_input
 * @self: a #GeglOp.
 * @n: which data.
 *
 * Gets the nth input data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_data_input(GeglOp *self,
                       gint n)
{
  return get_data(self, self->data_inputs, n);
}

/**
 * gegl_op_get_data_output
 * @self: a #GeglOp.
 * @n: which data.
 *
 * Gets the nth output data. 
 *
 * Returns: a #GeglData.
 **/
GeglData*
gegl_op_get_data_output(GeglOp *self,
                        gint n)
{
  return get_data(self, self->data_outputs, n);
}

static GeglData*
get_data(GeglOp *self,
         GList *list,
         gint n)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  if(n >= 0 && n < g_list_length(list))
    return g_list_nth_data(list, n);

  return NULL;
}
           
/**
 * gegl_op_get_data_inputs
 * @self: a #GeglOp.
 *
 * Get the inputs data list. 
 *
 * Returns: a #GeglData.
 **/
GList*
gegl_op_get_data_inputs(GeglOp *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->data_inputs; 
}

/**
 * gegl_op_get_data_outputs
 * @self: a #GeglOp.
 *
 * Get the outputs data list. 
 *
 * Returns: a #GeglData.
 **/
GList*
gegl_op_get_data_outputs(GeglOp *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->data_outputs; 
}

/**
 * gegl_op_append_input
 * @self: a #GeglOp.
 * @data_type: the type of the data to create.
 * @name: the name of the data.
 *
 * Append an input with the passed data type and name. 
 *
 **/
void
gegl_op_append_input(GeglOp *self,
                     GType data_type,
                     gchar *name)
{
  GeglOpClass *op_class = GEGL_OP_GET_CLASS(self);
  GParamSpec *pspec = gegl_op_class_find_data_input_property(op_class, name);
  GeglData *data = g_object_new(data_type, "param-spec", pspec, NULL);
  gegl_op_append_data_input(self, data);
  g_object_unref(data);
}

/**
 * gegl_op_append_data_input
 * @self: a #GeglOp.
 * @data: the #GeglData to append.
 *
 * Append data to list of input data. 
 *
 **/
void
gegl_op_append_data_input(GeglOp *self,
                          GeglData *data)
{
  gegl_node_add_input(GEGL_NODE(self), g_list_length(self->data_inputs));
  self->data_inputs = add_data(self, self->data_inputs, data);
}

/**
 * gegl_op_append_output
 * @self: a #GeglOp.
 * @data_type: the type of the data to create.
 * @name: the name of the data.
 *
 * Append an output with the passed data type and name. 
 *
 **/
void
gegl_op_append_output(GeglOp *self,
                      GType data_type,
                      gchar *name)
{
  GeglOpClass *op_class = GEGL_OP_GET_CLASS(self);
  GParamSpec *pspec = gegl_op_class_find_data_output_property(op_class, name);
  GeglData *data = g_object_new(data_type, "param-spec", pspec, NULL);
  gegl_op_append_data_output(self, data);
  g_object_unref(data);
}

/**
 * gegl_op_append_data_output
 * @self: a #GeglOp.
 * @data: the #GeglData to append.
 *
 * Append data to list of output data. 
 *
 **/
void
gegl_op_append_data_output(GeglOp *self,
                           GeglData *data)
{
  gegl_node_add_output(GEGL_NODE(self), g_list_length(self->data_outputs));
  self->data_outputs = add_data(self, self->data_outputs, data);
}

static GList *
add_data(GeglOp *self,
         GList *list,
         GeglData *data)
{
  g_return_val_if_fail(self != NULL, NULL);

  if(data)
    g_object_ref(data);

  return g_list_append(list, data);
}

/**
 * gegl_op_free_data_inputs
 * @self: a #GeglOp.
 *
 * Free the list of input data. 
 *
 **/
void
gegl_op_free_data_inputs(GeglOp * self)
{
  free_data(self, self->data_inputs);
  self->data_inputs = NULL;
}

/**
 * gegl_op_free_data_outputs
 * @self: a #GeglOp.
 *
 * Free the list of output data. 
 *
 **/
void
gegl_op_free_data_outputs(GeglOp * self)
{
  free_data(self, self->data_outputs);
  self->data_outputs = NULL;
}

static void 
free_data(GeglOp *self,
          GList *list)
{
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  llink = list;
  while(llink)
    {
      /* Free all the GeglData. */
      if(llink->data)
        g_object_unref(llink->data);

      llink = g_list_next(llink);
    }

  g_list_free(list);
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_op(visitor, GEGL_OP(node));
}

/**
 * gegl_op_class_install_data_input_property
 * @class: the #GeglOpClass.
 * @pspec: the #GParamSpec that describes this property.
 *
 * Install an input data property. 
 *
 **/
void
gegl_op_class_install_data_input_property (GeglOpClass *class,
                                      GParamSpec   *pspec)
{
  g_return_if_fail (GEGL_IS_OP_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (g_param_spec_pool_lookup (input_pspec_pool, pspec->name, G_OBJECT_CLASS_TYPE (class), FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains an input property named `%s'",
		 G_OBJECT_CLASS_NAME (class),
		 pspec->name);
      return;
    }

  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_param_spec_pool_insert (input_pspec_pool, pspec, G_OBJECT_CLASS_TYPE (class));
}

/**
 * gegl_op_class_find_data_input_property
 * @class: the #GeglOpClass.
 * @property_name: the name of this property.
 *
 * Find an input data property. 
 *
 * Returns: the #GParamSpec for the input data property
 **/
GParamSpec*
gegl_op_class_find_data_input_property (GeglOpClass *class,
                                        const gchar  *property_name)
{
  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (input_pspec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (class),
				   TRUE);
}

/**
 * gegl_op_class_list_data_input_properties
 * @class: the #GeglOpClass.
 * @n_properties_p: returns the number of properties found.
 *
 * List the input data properties. The returned array
 * should be freed when finished. 
 *
 * Returns: the #GParamSpec array of input data properties.
 **/
GParamSpec** /* free result */
gegl_op_class_list_data_input_properties (GeglOpClass *class,
                                          guint *n_properties_p)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);

  pspecs = g_param_spec_pool_list (input_pspec_pool,
				   G_OBJECT_CLASS_TYPE (class),
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}

/**
 * gegl_op_class_install_data_output_property
 * @class: the #GeglOpClass.
 * @pspec: the #GParamSpec that describes this property.
 *
 * Install a output data property. 
 *
 **/
void
gegl_op_class_install_data_output_property (GeglOpClass *class,
                                            GParamSpec   *pspec)
{
  g_return_if_fail (GEGL_IS_OP_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (g_param_spec_pool_lookup (output_pspec_pool, pspec->name, G_OBJECT_CLASS_TYPE (class), FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains an output property named `%s'",
		 G_OBJECT_CLASS_NAME (class),
		 pspec->name);
      return;
    }

  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_param_spec_pool_insert (output_pspec_pool, pspec, G_OBJECT_CLASS_TYPE (class));
}

/**
 * gegl_op_class_find_data_output_property
 * @class: the #GeglOpClass.
 * @property_name: the name of this property.
 *
 * Find an output data property. 
 *
 * Returns: the #GParamSpec for the output data property
 **/
GParamSpec*
gegl_op_class_find_data_output_property (GeglOpClass *class,
                                         const gchar  *property_name)
{
  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (output_pspec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (class),
				   TRUE);
}

/**
 * gegl_op_class_list_data_output_properties
 * @class: the #GeglOpClass.
 * @n_properties_p: returns the number of properties found.
 *
 * List the output data properties. The returned array
 * should be freed when finished. 
 *
 * Returns: the #GParamSpec array of output data properties.
 **/
GParamSpec** /* free result */
gegl_op_class_list_data_output_properties (GeglOpClass *class,
                                           guint *n_properties_p)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);

  pspecs = g_param_spec_pool_list (output_pspec_pool,
				   G_OBJECT_CLASS_TYPE (class),
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}
