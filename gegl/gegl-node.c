#include "gegl-node.h"
#include "gegl-visitor.h"
#include "gegl-input-value-types.h"
#include "gegl-input-param-specs.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_INPUT,
  PROP_NUM_INPUTS,
  PROP_NUM_OUTPUTS,
  PROP_LAST 
};

static void class_init (GeglNodeClass * klass);
static void init(GeglNode *self, GeglNodeClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void accept(GeglNode * node, GeglVisitor * visitor);

static void remove_sink(GeglNode *self, GeglConnector *connector);
static void add_sink(GeglNode *self, GeglConnector *connector);

static gpointer parent_class = NULL;

GType
gegl_node_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglNodeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglNode),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglNode", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->accept = accept;

  g_object_class_install_property (gobject_class, PROP_INPUT,
                                   gegl_param_spec_input ("input",
                                                          "Input",
                                                          "Input for node",
                                                          G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_NUM_INPUTS,
                                   g_param_spec_int ("num_inputs",
                                                      "NumInputs",
                                                      "Number of inputs for node",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_OUTPUTS,
                                   g_param_spec_int ("num_outputs",
                                                      "NumOutputs",
                                                      "Number of outputs for node",
                                                      0,
                                                      1,
                                                      0,
                                                      G_PARAM_READWRITE));
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->num_inputs = 0;
  self->inputs = NULL;
  self->num_outputs = 0;
  self->outputs = NULL;
  self->enabled = TRUE;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  gegl_node_remove_sources(self);
  gegl_node_free_inputs(self);

  gegl_node_remove_sinks(self);
  gegl_node_free_outputs(self);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUT:
      {
        gint n;
        GeglNode *source = g_value_get_input(value, &n); 
        gegl_node_set_source(node, source, n);  
      }
      break;
    case PROP_NUM_INPUTS:
      gegl_node_set_num_inputs(node, g_value_get_int(value));  
      break;
    case PROP_NUM_OUTPUTS:
      gegl_node_set_num_outputs(node, g_value_get_int(value));  
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_INPUTS:
      g_value_set_int(value, gegl_node_get_num_inputs(node));  
      break;
    case PROP_NUM_OUTPUTS:
      g_value_set_int(value, gegl_node_get_num_outputs(node));  
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

void
gegl_node_add_input(GeglNode *self,
                    gint n)
{
  GeglConnector * connector;
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n <= self->num_inputs); 

  connector = g_new(GeglConnector, 1);

  connector->source = NULL;
  connector->node = self;
  connector->input = n;

  self->inputs = g_list_insert(self->inputs, connector, n);
  self->num_inputs = g_list_length(self->inputs);
}

void
gegl_node_add_output(GeglNode *self,
                     gint n)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n <= self->num_outputs); 

  self->outputs = g_list_insert(self->outputs, NULL, n);
  self->num_outputs = g_list_length(self->outputs);
}

/**
 * gegl_node_get_num_inputs:
 * @self: a #GeglNode.
 *
 * Gets the number of inputs.
 *
 * Returns: number of inputs. 
 **/
gint 
gegl_node_get_num_inputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return self->num_inputs;
}


/**
 * gegl_node_set_num_inputs:
 * @self: a #GeglNode.
 * @num_inputs: number of inputs 
 *
 * Sets the number of inputs. 
 *
 **/
void 
gegl_node_set_num_inputs (GeglNode * self, 
                          gint num_inputs)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (num_inputs >= 0);

  if(num_inputs > 0) 
    {
      gint i;
      for(i = 0; i < num_inputs; i++)
        gegl_node_add_input(self,i);
    }
}

/**
 * gegl_node_get_num_outputs:
 * @self: a #GeglNode.
 *
 * Gets the number of outputs.
 *
 * Returns: number of outputs. 
 **/
gint 
gegl_node_get_num_outputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return self->num_outputs;
}


/**
 * gegl_node_set_num_outputs:
 * @self: a #GeglNode.
 * @num_outputs: number of outputs 
 *
 * Sets the number of outputs.
 *
 **/
void 
gegl_node_set_num_outputs (GeglNode * self, 
                           gint num_outputs)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (num_outputs >= 0 && num_outputs <=1);

  if(num_outputs > 0) 
    {
      gint i;
      for(i = 0; i < num_outputs; i++)
        gegl_node_add_output(self,i);
    }
}


/**
 * gegl_node_get_source:
 * @self: a #GeglNode.
 * @n: which source.
 *
 * Returns the nth source node and which output it uses as source.
 *
 * Returns: a #GeglNode. 
 **/
GeglNode *
gegl_node_get_source (GeglNode * self, 
                      gint n)
{
  GeglNode *source =  NULL;
  if(n < 0 || n >= self->num_inputs)
      return NULL;

  if(self->inputs)
    {
      GeglConnector *connector = g_list_nth_data(self->inputs, n);
      source = connector->source;
    }

  return source;
}

/**
 * gegl_node_set_source:
 * @self: a #GeglNode.
 * @source: the node to use for nth source.
 * @n: which source.
 *
 * Sets source and output of the nth source.
 *
 **/
void
gegl_node_set_source(GeglNode * self, 
                     GeglNode * source, 
                     gint n)
{
  GeglConnector *connector;
  GeglNode *old_source;

  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  connector = g_list_nth_data(self->inputs, n);
  old_source = connector->source;

  if(old_source)
    remove_sink(old_source, connector);

  if(source)
    add_sink(source, connector);
}

static void
remove_sink(GeglNode *self, 
            GeglConnector *connector)
{
  if(connector)
    {
      GList * llink = g_list_nth(self->outputs, 0); 
      llink->data = g_list_remove(llink->data, connector);

      /* sink loses a ref to me. */
      g_object_unref(self);
      connector->source = NULL;
    }
}

static void
add_sink(GeglNode *self,
         GeglConnector *connector)
{
  if(connector)
    {
      GList * llink = g_list_nth(self->outputs, 0); 
      llink->data = g_list_append(llink->data, connector);

      /* sink gains a ref to me. */
      g_object_ref(self);
      connector->source = self;
    }
}

/**
 * gegl_node_get_num_sinks:
 * @self: a #GeglNode.
 *
 * Gets the number of sinks.
 *
 * Returns: number of sinks. 
 **/
gint 
gegl_node_get_num_sinks (GeglNode * self)
{
  GList *sinks;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  sinks = g_list_nth_data(self->outputs, 0);
  return g_list_length(sinks);
}


/**
 * gegl_node_get_sink:
 * @self: a #GeglNode.
 * @n: nth sink attached to output.
 *
 * Gets the nth sink attached to nodes output.
 *
 * Returns: the #GeglNode that is the sink. 
 **/
GeglNode*         
gegl_node_get_sink (GeglNode * self,
                    gint n)
{
  GeglConnector * connector;
  GeglNode *node = NULL;
  GList *sinks;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  sinks = g_list_nth_data(self->outputs, 0);
  connector = (GeglConnector*)g_list_nth_data(sinks, n);

  /* Just return the node */
  if(connector)
    node = connector->node;

  return node;
}


/**
 * gegl_node_get_sink_input:
 * @self: a #GeglNode.
 * @n: nth sink attached to output.
 *
 * Gets the input of the nth sink attached to nodes output.
 *
 * Returns: the input of the #GeglNode that is the sink. 
 **/
gint              
gegl_node_get_sink_input(GeglNode * self,
                         gint n)
{
  GeglConnector * connector;
  gint input = -1;
  GList *sinks;

  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  sinks = g_list_nth_data(self->outputs, 0);
  connector = (GeglConnector*)g_list_nth_data(sinks, n);

  /* Just return the input */
  if(connector)
    input = connector->input;

  return input;
}

/**
 * gegl_node_unlink:
 * @self: a #GeglNode.
 *
 * Removes all sources and sinks from this node.
 *
 **/
void      
gegl_node_unlink(GeglNode * self)

{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  gegl_node_remove_sources(self);
  gegl_node_remove_sinks(self);
}

/**
 * gegl_node_remove_sources:
 * @self: a #GeglNode.
 *
 * Removes all sources from this node.
 *
 **/
void
gegl_node_remove_sources(GeglNode *self) 
{
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  for(i = 0; i < self->num_inputs; i++)
    gegl_node_set_source(self, NULL, i);
}

/**
 * gegl_node_remove_sinks:
 * @self: a #GeglNode.
 *
 * Removes any sinks of this node.
 *
 **/
void
gegl_node_remove_sinks(GeglNode *self) 
{
  GList *sinks;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  while((sinks = g_list_nth_data(self->outputs, 0)))
    {
      GeglConnector *connector = sinks->data;
      remove_sink(self, connector);
    }
}

void
gegl_node_free_inputs(GeglNode * self)
{
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  llink = self->inputs;
  while(llink)
    {
      /* Free the connectors for input.*/
      if(llink->data)
        g_free(llink->data);

      llink = g_list_next(llink);
    }

  g_list_free(self->inputs);
  self->inputs = NULL;
}

void
gegl_node_free_outputs(GeglNode *self)
{
  g_list_free(self->outputs);
  self->outputs = NULL;
}

/**
 * gegl_node_accept:
 * @self: a #GeglNode.
 * @visitor: a #GeglVisitor.
 *
 * Accepts a visitor.
 *
 **/
void      
gegl_node_accept(GeglNode * self,
                 GeglVisitor * visitor)
{
  GeglNodeClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (visitor != NULL);
  g_return_if_fail (GEGL_IS_VISITOR(visitor));

  klass = GEGL_NODE_GET_CLASS(self);

  if(klass->accept)
    (*klass->accept)(self, visitor);
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_node(visitor, node);
}
