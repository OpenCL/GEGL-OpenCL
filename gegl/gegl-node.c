#include "gegl-node.h"
#include "gegl-visitor.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_NUM_INPUTS,
  PROP_NUM_OUTPUTS,
  PROP_LAST 
};

typedef struct _GeglConnector  GeglConnector;
struct _GeglConnector
{
    GeglNode * sink;
    gint input;

    GeglNode * source;
    gint output;
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
        NULL
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
                                    G_MAXINT,
                                    0,
                                    G_PARAM_READWRITE));
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->inputs = g_array_new(FALSE,FALSE,sizeof(GeglConnector));
  self->outputs = g_array_new(FALSE,FALSE,sizeof(GList*));
  self->enabled = TRUE;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  gegl_node_remove_sources(self);
  g_array_free(self->inputs, TRUE);

  gegl_node_remove_sinks(self);
  g_array_free(self->outputs, TRUE);

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

/**
 * gegl_node_add_input:
 * @self: a #GeglNode.
 *
 * Add an input at the end.
 *
 **/
void
gegl_node_add_input(GeglNode *self)
{
  GeglConnector connector; 
  g_return_if_fail(GEGL_IS_NODE(self));

  connector.sink = self;
  connector.input = self->inputs->len;
  connector.source = NULL;
  connector.output = 0;
  self->inputs = g_array_append_val(self->inputs, connector);
}

/**
 * gegl_node_add_output:
 * @self: a #GeglNode.
 *
 * Add an output at the end.
 *
 **/
void
gegl_node_add_output(GeglNode *self)
{
  GList *outputs_list = NULL;
  g_return_if_fail(GEGL_IS_NODE(self));

  self->outputs = g_array_append_val(self->outputs, outputs_list);
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
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return self->inputs->len;
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
  gint i;
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (num_inputs >= 0);

  for(i = 0; i < num_inputs; i++)
    gegl_node_add_input(self);
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
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return self->outputs->len;
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
  gint i;
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (num_outputs >= 0);

  for(i = 0; i < num_outputs; i++)
    gegl_node_add_output(self);
}


/**
 * gegl_node_get_source:
 * @self: a #GeglNode.
 * @input: which input.
 *
 * Gets the source node for the specified input.
 *
 * Returns: a #GeglNode. 
 **/
GeglNode *
gegl_node_get_source (GeglNode * self, 
                      gint input)
{
  gint output = 0;
  return gegl_node_get_m_source(self, input, &output);
}

/**
 * gegl_node_set_source:
 * @self: a #GeglNode.
 * @source: the node to use.
 * @input: which input.
 *
 * Sets the source node for the specified input.
 *
 **/
void
gegl_node_set_source(GeglNode * self, 
                     GeglNode * source, 
                     gint input)
{
  gegl_node_set_m_source(self, source, input, 0);
}


/**
 * gegl_node_get_m_source:
 * @self: a #GeglNode.
 * @input: which input.
 * @output: returns which output of the source node is used. 
 *
 * Gets the source node and output for the specified input. 
 *
 * Returns: a #GeglNode. 
 **/
GeglNode *
gegl_node_get_m_source (GeglNode * self, 
                        gint input,
                        gint *output)
{
  GeglNode *source =  NULL;
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (input >= 0L && input < (gint)self->inputs->len, NULL); 

  *output = -1;

  if(self->inputs->len > 0L)
    {
      if(input >= 0L && input < (gint)self->inputs->len)
        {
          GeglConnector *connector = &g_array_index(self->inputs, GeglConnector, input);
          source = connector->source;
          *output = connector->output;
        }
    }

  return source;
}

/**
 * gegl_node_set_m_source:
 * @self: a #GeglNode.
 * @source: the node to use.
 * @input: which input.
 * @output: which output of source node to use.
 *
 * Sets the source node and output for the specified input.
 *
 **/
void
gegl_node_set_m_source(GeglNode * self, 
                       GeglNode * source, 
                       gint input,
                       gint output)
{
  gint num_outputs;
  GeglConnector *connector;
  GeglNode *old_source;
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail(input >= 0L && input < (gint)self->inputs->len); 

  connector = &g_array_index(self->inputs, GeglConnector, input);
  old_source = connector->source;

  if(old_source)
    remove_sink(old_source, connector);

  /* Set the passed output to use */
  if(source)
  {
    num_outputs = gegl_node_get_num_outputs(source);
    g_assert(output < num_outputs);
    connector->output = output;
  }
  else
    connector->output = 0;

  if(source)
    add_sink(source, connector);
}


static void
remove_sink(GeglNode *self, 
            GeglConnector *connector)
{
  if(self->outputs->len > 0L && connector)
    {
      GList **sinks = &g_array_index(self->outputs, GList*, connector->output); 

      *sinks = g_list_remove(*sinks, connector);

      /* sink loses a ref to me. */
      g_object_unref(self);
      connector->source = NULL;
    }
}

static void
add_sink(GeglNode *self,
         GeglConnector *connector)
{
  if(self->outputs->len > 0 && connector)
    {
      GList **sinks = &g_array_index(self->outputs, GList*, connector->output); 
      *sinks = g_list_append(*sinks, connector);

      /* sink gains a ref to me. */
      g_object_ref(self);
      connector->source = self;
    }
}

/**
 * gegl_node_get_num_sinks:
 * @self: a #GeglNode.
 * @output: which output.
 *
 * Gets the number of sinks for the specified output.
 *
 * Returns: number of sinks. 
 **/
gint 
gegl_node_get_num_sinks (GeglNode * self, 
                         gint output)
{
  gint num_sinks = 0;
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);
#if 0
  g_return_val_if_fail(output >= 0L && output < (gint)self->outputs->len, -1); 
#endif

  if(self->outputs->len > 0)
    {
      GList *sinks = g_array_index(self->outputs, GList*, output); 
      num_sinks = g_list_length(sinks);
    }

  return num_sinks;
}


/**
 * gegl_node_get_sink:
 * @self: a #GeglNode.
 * @output: which output to use.
 * @n: which sink to get.
 *
 * Gets the nth sink attached to the specified output.
 *
 * Returns: a #GeglNode. 
 **/
GeglNode*         
gegl_node_get_sink (GeglNode * self,
                    gint output,
                    gint n)
{
  GeglNode *sink = NULL;
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  if(self->outputs->len > 0)
    {
      GList *sinks = g_array_index(self->outputs, GList*, output); 
      GeglConnector * connector = (GeglConnector*)g_list_nth_data(sinks, n);
      if(connector)
        sink = connector->sink;
    }

  return sink;
}


/**
 * gegl_node_get_sink_input:
 * @self: a #GeglNode.
 * @output: which output to use.
 * @n: which sink.
 *
 * Gets which input of the nth sink of the specified output this node is
 * attached to.
 *
 * Returns: an input of the nth sink. 
 **/
gint              
gegl_node_get_sink_input(GeglNode * self,
                         gint output,
                         gint n)
{
  gint input = -1;
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  if(self->outputs->len > 0)
    {
      GList *sinks = g_array_index(self->outputs, GList*, output); 
      GeglConnector * connector = (GeglConnector*)g_list_nth_data(sinks, n);

      /* Just return the input */
      if(connector)
        input = connector->input;
    }

  return input;
}

/**
 * gegl_node_unlink:
 * @self: a #GeglNode.
 *
 * Removes all sources and sinks for this node.
 *
 **/
void      
gegl_node_unlink(GeglNode * self)

{
  g_return_if_fail (GEGL_IS_NODE (self));

  gegl_node_remove_sources(self);
  gegl_node_remove_sinks(self);
}

/**
 * gegl_node_remove_sources:
 * @self: a #GeglNode.
 *
 * Removes all sources for this node.
 *
 **/
void
gegl_node_remove_sources(GeglNode *self) 
{
  gint i;
  g_return_if_fail (GEGL_IS_NODE (self));

  for(i = 0; i < (gint)self->inputs->len; i++)
    gegl_node_set_source(self, NULL, i);
}

/**
 * gegl_node_remove_sinks:
 * @self: a #GeglNode.
 *
 * Removes all sinks for this node.
 *
 **/
void
gegl_node_remove_sinks(GeglNode *self) 
{
  g_return_if_fail (GEGL_IS_NODE (self));

  if(self->outputs->len > 0)
    {
      GList *sinks = NULL;
      do
        {
          sinks = g_array_index(self->outputs, GList*, 0);
          if(sinks) 
            {
              GeglConnector *connector = sinks->data;
              remove_sink(self,connector);
            }
        }
      while(sinks);
    }
}

/**
 * gegl_node_accept:
 * @self: a #GeglNode.
 * @visitor: a #GeglVisitor.
 *
 * Accept a visitor.
 *
 **/
void      
gegl_node_accept(GeglNode * self,
                 GeglVisitor * visitor)
{
  GeglNodeClass *klass;
  g_return_if_fail (GEGL_IS_NODE (self));
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
