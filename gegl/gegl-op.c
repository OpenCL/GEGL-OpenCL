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

static void free_data_list(GeglOp *self, GList *data_list);
static GList * add_data(GeglOp *self, GList *data_list, GeglData *data, gint n);
static GeglData* get_data(GeglOp *self, GList *data_list, gint n);

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
  self->input_data_list = NULL;
  self->output_data_list = NULL;
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

  gegl_op_free_input_data_list(self);
  gegl_op_free_output_data_list(self);

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

void
gegl_op_class_install_input_data_property (GeglOpClass *class,
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

GParamSpec*
gegl_op_class_find_input_data_property (GeglOpClass *class,
			                       const gchar  *property_name)
{
  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (input_pspec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (class),
				   TRUE);
}

GParamSpec** /* free result */
gegl_op_class_list_input_data_properties (GeglOpClass *class,
				                     guint        *n_properties_p)
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

void
gegl_op_class_install_output_data_property (GeglOpClass *class,
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

GParamSpec*
gegl_op_class_find_output_data_property (GeglOpClass *class,
			                       const gchar  *property_name)
{
  g_return_val_if_fail (GEGL_IS_OP_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (output_pspec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (class),
				   TRUE);
}

GParamSpec** /* free result */
gegl_op_class_list_output_data_properties (GeglOpClass *class,
				                      guint        *n_properties_p)
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

/**
 * gegl_op_apply
 * @self: a #GeglOp.
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

GeglData*
gegl_op_get_input_data(GeglOp *self,
                        gint n)
{
  return get_data(self, self->input_data_list, n);
}

GeglData*
gegl_op_get_output_data(GeglOp *self,
                         gint n)
{
  return get_data(self, self->output_data_list, n);
}

static GeglData*
get_data(GeglOp *self,
          GList *data_list,
          gint n)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  if(n >= 0 && n < g_list_length(data_list))
    return g_list_nth_data(data_list, n);

  return NULL;
}
           
GList*
gegl_op_get_input_data_list(GeglOp *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->input_data_list; 
}

GList*
gegl_op_get_output_data_list(GeglOp *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->output_data_list; 
}

void
gegl_op_add_input(GeglOp *self,
                  GType data_type,
                  gchar *name,
                  gint n)
{
  GeglOpClass *op_class = GEGL_OP_GET_CLASS(self);
  GParamSpec *pspec = gegl_op_class_find_input_data_property(op_class, name);
  GeglData *data = g_object_new(data_type, "param-spec", pspec, NULL);
  gegl_op_add_input_data(self, data, n);
  g_object_unref(data);
}

void
gegl_op_add_input_data(GeglOp *self,
                        GeglData *data,
                        gint n)
{
  gegl_node_add_input(GEGL_NODE(self), n);
  self->input_data_list = add_data(self, self->input_data_list, data, n);
}

void
gegl_op_add_output(GeglOp *self,
                   GType data_type,
                   gchar *name,
                   gint n)
{
  GeglOpClass *op_class = GEGL_OP_GET_CLASS(self);
  GParamSpec *pspec = gegl_op_class_find_output_data_property(op_class, name);
  GeglData *data = g_object_new(data_type, "param-spec", pspec, NULL);
  gegl_op_add_output_data(self, data, n);
  g_object_unref(data);
}

void
gegl_op_add_output_data(GeglOp *self,
                        GeglData *data,
                        gint n)
{
  gegl_node_add_output(GEGL_NODE(self), n);
  self->output_data_list = add_data(self, self->output_data_list, data, n);
}

static GList *
add_data(GeglOp *self,
          GList *data_list,
          GeglData *data,
          gint n)
{
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(n >= 0 && n <= g_list_length(data_list), NULL); 

  if(data)
    g_object_ref(data);

  return g_list_insert(data_list, data, n);
}

void
gegl_op_free_input_data_list(GeglOp * self)
{
  free_data_list(self, self->input_data_list);
  self->input_data_list = NULL;
}

void
gegl_op_free_output_data_list(GeglOp * self)
{
  free_data_list(self, self->output_data_list);
  self->output_data_list = NULL;
}

static void 
free_data_list(GeglOp *self,
           GList *data_list)
{
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  llink = data_list;
  while(llink)
    {
      /* Free all the GeglData. */
      if(llink->data)
        g_object_unref(llink->data);

      llink = g_list_next(llink);
    }

  g_list_free(data_list);
}


static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_op(visitor, GEGL_OP(node));
}
