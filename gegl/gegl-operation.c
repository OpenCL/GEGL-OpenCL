#include "gegl-filter.h"
#include "gegl-visitor.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglFilterClass * klass);
static void init (GeglFilter * self, GeglFilterClass * klass);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void validate_inputs (GeglFilter * self, GArray * collected_data);
static void validate_outputs (GeglFilter * self);

static void accept(GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

GType
gegl_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilter),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglFilter", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;

  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  klass->validate_inputs = validate_inputs;
  klass->validate_outputs = validate_outputs;
}

static void 
init (GeglFilter * self, 
      GeglFilterClass * klass)
{
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
 * gegl_filter_evaluate:
 * @self: a #GeglFilter.
 *
 * Evaluate the filter.
 * 
 **/
void      
gegl_filter_evaluate (GeglFilter * self) 
{
  GeglFilterClass *klass;
  g_return_if_fail (GEGL_IS_FILTER (self));

  klass = GEGL_FILTER_GET_CLASS(self);
  if(klass->prepare)
    (*klass->prepare)(self);

  if(klass->process)
    (*klass->process)(self);

  if(klass->finish)
    (*klass->finish)(self);
}

/**
 * gegl_filter_validate_inputs:
 * @self: a #GeglFilter.
 * @collected_data: Array of #GeglData to validate and convert.
 *
 * Validate and convert the inputs.
 * 
 **/
void      
gegl_filter_validate_inputs (GeglFilter * self, 
                             GArray * collected_data)
{
  GeglFilterClass *klass;
  g_return_if_fail (GEGL_IS_FILTER (self));

  klass = GEGL_FILTER_GET_CLASS(self);
  if(klass->validate_inputs)
    (*klass->validate_inputs)(self, collected_data);
}

static void
validate_inputs(GeglFilter *self,
                GArray *collected_data)
{
}

/**
 * gegl_filter_validate_outputs:
 * @self: a #GeglFilter.
 *
 * Validate the outputs.
 * 
 **/
void      
gegl_filter_validate_outputs (GeglFilter * self)
{
  GeglFilterClass *klass;
  g_return_if_fail (GEGL_IS_FILTER (self));

  klass = GEGL_FILTER_GET_CLASS(self);
  if(klass->validate_outputs)
    (*klass->validate_outputs)(self);
}

static void
validate_outputs(GeglFilter *self)
{
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_filter(visitor, GEGL_FILTER(node));
}
