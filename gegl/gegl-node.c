#include "gegl-node.h"
#include "gegl-visitor.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_NUM_INPUTS,
  PROP_NUM_OUTPUTS,
  PROP_SOURCE,
  PROP_SOURCE0,
  PROP_SOURCE1,
  PROP_SOURCE_OUTPUT,
  PROP_SOURCE0_OUTPUT,
  PROP_SOURCE1_OUTPUT,
  PROP_LAST 
};

static void class_init (GeglNodeClass * klass);
static void init(GeglNode *self, GeglNodeClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void accept(GeglNode * node, GeglVisitor * visitor);

static GeglConnector* allocate_connectors(GeglNode * self, gint num_inputs);
static void free_connectors(GeglNode * self);

static GList** allocate_sinks(GeglNode * self, gint num_outputs);
static void free_sinks(GeglNode * self);

static void add_sink(GeglNode * self, GeglConnector * connector);
static void remove_sink(GeglNode * self, GeglConnector * connector);

static GeglConnector* get_connector(GeglNode * self, gint n);

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

  g_object_class_install_property (gobject_class, PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        "Source",
                                                        "Source of this node",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE0,
                                   g_param_spec_object ("source0",
                                                        "Source0",
                                                        "Source 0 of this node",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE1,
                                   g_param_spec_object ("source1",
                                                        "Source1",
                                                        "Source 1 of this node",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE_OUTPUT,
                                   g_param_spec_int ("source_output",
                                                      "SourceOutput",
                                                      "Source's output ",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE0_OUTPUT,
                                   g_param_spec_int ("source0_output",
                                                      "Source0Output",
                                                      "Source0's output ",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE1_OUTPUT,
                                   g_param_spec_int ("source1_output",
                                                      "Source1Output",
                                                      "Source1's output",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  return;
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->num_inputs = 0;
  self->connectors = NULL;
  self->num_outputs = 0;
  self->sinks = NULL;
  self->enabled = TRUE;

  return;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  gegl_node_remove_sources(self);
  
  /* I dont think we need to remove sinks but doesnt hurt. */
  gegl_node_remove_sinks(self);
  free_connectors(self);
  free_sinks(self);

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
    case PROP_SOURCE:
    case PROP_SOURCE0:
      gegl_node_set_source_node(node, (GeglNode*)g_value_get_object(value), 0);  
      break;
    case PROP_SOURCE1:
      gegl_node_set_source_node(node, (GeglNode*)g_value_get_object(value), 1);  
      break;
    case PROP_SOURCE_OUTPUT:
    case PROP_SOURCE0_OUTPUT:
      gegl_node_set_source_output(node, g_value_get_int(value), 0);  
      break;
    case PROP_SOURCE1_OUTPUT:
      gegl_node_set_source_output(node, g_value_get_int(value), 1);  
      break;
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
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_INPUTS:
      g_value_set_int(value, gegl_node_get_num_inputs(node));  
      break;
    case PROP_NUM_OUTPUTS:
      g_value_set_int(value, gegl_node_get_num_outputs(node));  
      break;
    case PROP_SOURCE:
    case PROP_SOURCE0:
      g_value_set_object(value, (GObject*)gegl_node_get_source_node(node, 0));  
      break;
    case PROP_SOURCE1:
      g_value_set_object(value, (GObject*)gegl_node_get_source_node(node, 1));  
      break;
    case PROP_SOURCE_OUTPUT:
    case PROP_SOURCE0_OUTPUT:
      g_value_set_int(value, gegl_node_get_source_output(node, 0));  
      break;
    case PROP_SOURCE1_OUTPUT:
      g_value_set_int(value, gegl_node_get_source_output(node, 1));  
      break;
    default:
      break;
  }
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

  self->num_inputs = num_inputs;

  if(self->num_inputs > 0)
    self->connectors = allocate_connectors(self, self->num_inputs);
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
  g_return_if_fail (num_outputs >= 0);

  self->num_outputs = num_outputs;

  if(self->num_outputs > 0)
    self->sinks = allocate_sinks(self, self->num_outputs);
}


/**
 * gegl_node_get_source:
 * @self: a #GeglNode.
 * @output: pointer to output (out)
 * @n: which source.
 *
 * Returns the nth source node and which output it uses as source.
 *
 * Returns: a #GeglNode. 
 **/
GeglNode *
gegl_node_get_source (GeglNode * self, 
                      gint * output,
                      gint n)
{
  GeglNode *source =  NULL;
  if(n < 0 || n >= self->num_inputs)
      return NULL;

  *output = -1;

  if(self->connectors)
    {
      *output = self->connectors[n].output;
       source = self->connectors[n].source;
    }

  return source;
}


/**
 * gegl_node_set_source:
 * @self: a #GeglNode.
 * @source: the node to use for nth source.
 * @output: the output of the source.
 * @n: which source.
 *
 * Sets source and output of the nth source.
 *
 **/
void
gegl_node_set_source(GeglNode * self, 
                     GeglNode * source, 
                     gint output,
                     gint n)
{
  GeglConnector * connectors = NULL;

  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  connectors = self->connectors;

  /* Remove me as a sink of old source. */ 
  if(connectors[n].source)
    {
      remove_sink(connectors[n].source, &connectors[n]); 

      /* I lose a reference to old_source. */
      g_object_unref(connectors[n].source);
    }

  connectors[n].source = source;
  connectors[n].output = output;

  /* Should already be set, but thats okay. */
  connectors[n].node = self;
  connectors[n].input = n;

  /* Add me as a sink of the source. */ 
  if(connectors[n].source)
    {
      add_sink(connectors[n].source, &connectors[n]); 

      /* I gain a reference to source. */
      g_object_ref(connectors[n].source);
    }
}


/**
 * gegl_node_get_source_node:
 * @self: a #GeglNode.
 * @n: which source.
 *
 * Gets the nth source.
 *
 * Returns: a #GeglNode, the nth source of this node.
 **/
GeglNode * 
gegl_node_get_source_node (GeglNode * self, 
                           gint n)
{
  GeglConnector* connector = 
    get_connector(self,n);
  if(!connector)
    return NULL;

  /* Just return the source node */
  return connector->source;
}


/**
 * gegl_node_set_source_node:
 * @self: a #GeglNode.
 * @source: the node to make the nth source.
 * @n: which source.
 *
 * Sets the nth source of this node.
 *
 **/
void
gegl_node_set_source_node(GeglNode * self, 
                          GeglNode * source, 
                          gint n)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  /* Assume will use the 0th output from this source. */
  gegl_node_set_source(self, source, 0, n);
}


/**
 * gegl_node_get_source_output:
 * @self: a #GeglNode.
 * @n: which source.
 *
 * Gets the nth source output.
 *
 * Returns: The nth source output.
 **/
gint 
gegl_node_get_source_output(GeglNode * self, 
                            gint n)
{
  GeglConnector * connector;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  connector = get_connector(self, n);

  g_return_val_if_fail(connector->source, -1);

  return connector->output;
}


/**
 * gegl_node_set_source_output:
 * @self: a #GeglNode.
 * @output: The output for the nth source. 
 * @n: nth source.
 *
 * Sets the nth source output of this node.
 *
 **/
void
gegl_node_set_source_output(GeglNode * self, 
                            gint output, 
                            gint n)
{
  GeglConnector * connector;
  gint source_num_outputs;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  connector = get_connector(self, n);

  g_return_if_fail(connector->source);
  source_num_outputs = gegl_node_get_num_outputs(connector->source);
  g_return_if_fail(connector->output < source_num_outputs &&
                   connector->output >=0);

  remove_sink(connector->source, connector);
  connector->output = output; 
  add_sink(connector->source, connector);
}

/**
 * gegl_node_get_sinks:
 * @self: a #GeglNode.
 * @output: which output index.
 *
 * Returns a list of GeglConnectors for the passed output number. 
 *
 * Returns: a list of GeglConnector pointers. 
 **/
GList *
gegl_node_get_sinks (GeglNode * self, 
                     gint output)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  g_return_val_if_fail (output >= 0 && 
                        output < self->num_outputs, NULL);

  return self->sinks[output];
}


/**
 * gegl_node_get_num_sinks:
 * @self: a #GeglNode.
 * @output: output index
 *
 * Gets the number of sinks for this output.
 *
 * Returns: number of sinks. 
 **/
gint 
gegl_node_get_num_sinks (GeglNode * self,
                         gint output)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  if(self->num_outputs == 0)
    return 0;

  {
    g_return_val_if_fail (output >= 0 && 
                          output < self->num_outputs, -1);

    return g_list_length(self->sinks[output]);
  }
}


/**
 * gegl_node_get_total_num_sinks:
 * @self: a #GeglNode.
 *
 * Gets the total number of sinks for this node.
 *
 * Returns: total number of sinks. 
 **/
gint 
gegl_node_get_total_num_sinks (GeglNode * self)
{
  gint total = 0;
  gint i;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  for(i = 0; i < self->num_outputs; i++)
    total += g_list_length(self->sinks[i]);

  return total;
}


/**
 * gegl_node_get_sink:
 * @self: a #GeglNode.
 * @output: the output.
 * @n: nth sink attached to output.
 *
 * Gets the nth sink attached to nodes output.
 *
 * Returns: the #GeglNode that is the sink. 
 **/
GeglNode*         
gegl_node_get_sink (GeglNode * self,
                    gint output,
                    gint n)
{
  GeglConnector * connector;
  GeglNode *node = NULL;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail(output < self->num_outputs &&
                       output >=0, NULL);

  connector = (GeglConnector*)g_list_nth_data(self->sinks[output], n);

  /* Just return the node */
  if(connector)
    node = connector->node;

  return node;
}


/**
 * gegl_node_get_sink_input:
 * @self: a #GeglNode.
 * @output: which output.
 * @n: nth sink attached to output.
 *
 * Gets the input of the nth sink attached to nodes output.
 *
 * Returns: the input of the #GeglNode that is the sink. 
 **/
gint              
gegl_node_get_sink_input(GeglNode * self,
                         gint output,
                         gint n)
{
  GeglConnector * connector;
  gint input = -1;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);
  g_return_val_if_fail(output < self->num_outputs &&
                       output >=0, -1);

  connector = (GeglConnector*)g_list_nth_data(self->sinks[output], n);

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
    {
      gegl_node_set_source(self, NULL, 0, i);
    }
}

/**
 * gegl_node_remove_sources:
 * @self: a #GeglNode.
 *
 * Removes all sinks from this node.
 *
 **/
void
gegl_node_remove_sinks(GeglNode *self) 
{
  gint i, j;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  for(i = 0; i < self->num_outputs; i++)
    {
      for(j=0; j < g_list_length(self->sinks[i]); j++)
        {
          GeglConnector *connector = 
              (GeglConnector*)g_list_nth_data(self->sinks[i], j); 

          gegl_node_set_source(connector->node, 
                               NULL, 
                               0, 
                               connector->input); 
        }
    }
}


static GeglConnector *
get_connector (GeglNode * self, 
               gint n)
{
  if(n < 0 || n >= self->num_inputs)
      return NULL;

  if(self->connectors)
    return &self->connectors[n];
  else
    return NULL;
}

static void 
add_sink(GeglNode * self,
         GeglConnector * connector)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (connector != NULL);

  g_return_if_fail (connector->output >= 0 &&
                    connector->output < self->num_outputs);

  self->sinks[connector->output] = 
    g_list_append(self->sinks[connector->output], connector);
}

static void 
remove_sink (GeglNode * self, 
             GeglConnector * connector)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (connector != NULL);

  g_return_if_fail (connector->output >= 0 && 
                    connector->output < self->num_outputs);

  self->sinks[connector->output] = 
    g_list_remove(self->sinks[connector->output], connector);
}

static GeglConnector* 
allocate_connectors(GeglNode *self, 
                    gint num_inputs)
{
  gint i;
  GeglConnector * connectors = g_new(GeglConnector, num_inputs);

  for(i = 0; i < num_inputs; i++)
    {
      connectors[i].source = NULL;
      connectors[i].output = 0;

      connectors[i].node = self;
      connectors[i].input = i;
    }

  return connectors;
}

static void
free_connectors(GeglNode * self)
{
  g_free(self->connectors);
  self->connectors = NULL;
}

static GList** 
allocate_sinks(GeglNode * self,
               gint num_outputs)
{
  gint i;
  GList ** sinks = g_new(GList*, num_outputs);

  for(i = 0; i < num_outputs; i++)
    sinks[i] = NULL;

  return sinks;
}

static void
free_sinks(GeglNode * self)
{
  gint i; 

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  /* Free each list.  Entries on each 
     list should be freed already.*/
  for(i = 0; i < self->num_outputs; i++)
    {
      g_list_free(self->sinks[i]);
      self->sinks[i] = NULL;
    }

  /* Free the array of lists. */
  g_free(self->sinks);
  self->sinks = NULL;
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
