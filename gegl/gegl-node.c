#include "gegl-node.h"
#include "gegl-visitor.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglNodeClass * klass);
static void init(GeglNode *self, GeglNodeClass *klass);
static void finalize(GObject * self_object);

static void accept(GeglNode * node, GeglVisitor * visitor);

static GeglConnection* allocate_input_connections(GeglNode * self, gint num_inputs);
static void free_input_connections(GeglNode * self);

static GList** allocate_output_lists_array(GeglNode * self, gint num_outputs);
static void free_output_lists_array(GeglNode * self);

static void add_output(GeglNode * self, GeglNode * node, gint output_index);
static void remove_output (GeglNode * self, GeglNode * node, gint output_index);

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

  klass->accept = accept;

  return;
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->num_inputs = 0;
  self->input_connections = NULL;
  self->num_outputs = 0;
  self->outputs = NULL;

  self->enabled = TRUE;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  free_input_connections(self);
  free_output_lists_array(self);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_node_get_nth_input:
 * @self: a #GeglNode.
 * @n: which input.
 *
 * Gets the nth input.
 *
 * Returns: a #GeglNode, the nth input of this node.
 **/
GeglNode * 
gegl_node_get_nth_input (GeglNode * self, 
                         gint n)
{
  GeglConnection* input_connection = 
    gegl_node_get_nth_input_connection(self,n);
  if(!input_connection)
    return NULL;

  /* Just return the input node */
  return input_connection->input;
}

/**
 * gegl_node_get_nth_input_connection:
 * @self: a #GeglNode.
 * @n: which input.
 *
 * Returns the nth input connection.
 *
 * Returns: a #GeglConnection. 
 **/
GeglConnection *
gegl_node_get_nth_input_connection (GeglNode * self, 
                                    gint n)
{
  if(n < 0 || n >= self->num_inputs)
      return NULL;

  if(self->input_connections)
    return self->input_connections + n;
  else
    return NULL;
}

/**
 * gegl_node_set_nth_input:
 * @self: a #GeglNode.
 * @input: the node to make the nth input.
 * @n: which input.
 *
 * Sets the nth input of this node.
 *
 **/
void
gegl_node_set_nth_input(GeglNode * self, 
                        GeglNode * input, 
                        gint n)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  /* Assume will use the 0th output from this input. */
  gegl_node_set_nth_input_connection(self, input, 0, n); 
}

/**
 * gegl_node_set_nth_input_connection:
 * @self: a #GeglNode.
 * @input: the node to use for nth input.
 * @output_index: the output index of the inputt.
 * @n: which input.
 *
 * Sets the nth input connection of this node.
 *
 **/
void
gegl_node_set_nth_input_connection(GeglNode * self, 
                                   GeglNode * input, 
                                   gint output_index,
                                   gint n)
{
  GeglConnection * connections = NULL;

  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  connections = self->input_connections;

  if(connections[n].input)
    {
      remove_output(connections[n].input, self, connections[n].output_index); 

      /* I lose a reference to old_input. */
      g_object_unref(connections[n].input);
    }

  connections[n].input = input;
  connections[n].output_index = output_index;

  /* Should already be set, but thats okay. */
  connections[n].node = self;
  connections[n].input_index = n;

  /* Add me as an output with index. */ 
  if(connections[n].input)
    {
      add_output(connections[n].input, self, connections[n].output_index); 

      /* I gain a reference to source. */
      g_object_ref(connections[n].input);
    }
}

static void 
add_output(GeglNode * self,
           GeglNode * node,
           gint output_index)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  g_return_if_fail (output_index >= 0 && output_index < self->num_outputs);

  self->outputs[output_index] = g_list_append(self->outputs[output_index], node);
}

static void 
remove_output (GeglNode * self, 
               GeglNode * node,
               gint output_index)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  g_return_if_fail (output_index >= 0 && output_index < self->num_outputs);

  self->outputs[output_index] = g_list_remove(self->outputs[output_index], node);
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
 * @num_inputs: number of inputs.
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
    self->input_connections = allocate_input_connections(self, self->num_inputs);
}

static GeglConnection* 
allocate_input_connections(GeglNode *self, 
                     gint num_inputs)
{
  gint i;
  GeglConnection * input_connections = g_new(GeglConnection, num_inputs);

  for(i = 0; i < num_inputs; i++)
    {
      input_connections[i].input = NULL;
      input_connections[i].output_index = 0;

      input_connections[i].node = self;
      input_connections[i].input_index = i;
    }

  return input_connections;
}

static void
free_input_connections(GeglNode * self)
{
  GeglConnection * connections;
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  connections = self->input_connections;

  for(i = 0; i < self->num_inputs; i++)
    {
      if(connections[i].input)
        {
          remove_output(connections[i].input, 
                        self, 
                        connections[i].output_index);
          g_object_unref(connections[i].input);
        }
    }

  g_free(self->input_connections);
  self->input_connections = NULL;
}

/**
 * gegl_node_get_num_outputs:
 * @self: a #GeglNode.
 *
 * Gets the number of output indexs.
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
    self->outputs = 
      allocate_output_lists_array(self, self->num_outputs);
}

static GList** 
allocate_output_lists_array(GeglNode * self,
                            gint num_outputs)
{
  gint i;
  GList ** outputs = g_new(GList*, num_outputs);

  for(i = 0; i < num_outputs; i++)
    outputs[i] = NULL;

  return outputs;
}

static void
free_output_lists_array(GeglNode * self)
{
  gint i; 

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  /* Free each list.  Entries on each 
     list should be freed already.*/
  for(i = 0; i < self->num_outputs; i++)
    g_list_free(self->outputs[i]);

  /* Free the array of lists. */
  g_free(self->outputs);
}

/**
 * gegl_node_get_outputs:
 * @self: a #GeglNode.
 * @output_index: which output index.
 *
 * Returns a list of outputs for the passed output_index number. 
 *
 * Returns: a list of GeglNode pointers. 
 **/
GList *
gegl_node_get_outputs (GeglNode * self, 
                       gint output_index)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  g_return_val_if_fail (output_index >= 0 || output_index < self->num_outputs, NULL);

  return self->outputs[output_index];
}

/**
 * gegl_node_get_input_connections:
 * @self: a #GeglNode.
 *
 * Returns an array of input connections. Must be freed when finished.
 *
 * Returns: array of input connections. 
 **/
GeglConnection*
gegl_node_get_input_connections (GeglNode * self)
{
  gint i;
  GeglConnection *connections;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  connections = g_new(GeglConnection, self->num_inputs);
  for(i = 0; i < self->num_inputs; i++)
    {
      connections[i].input = self->input_connections[i].input;
      connections[i].output_index = self->input_connections[i].output_index;
      connections[i].node = self->input_connections[i].node;
      connections[i].input_index = self->input_connections[i].input_index;
    }

  return connections;
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
