#include "gegl-node.h"
#include "gegl-object.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_INPUTS,
  PROP_LAST 
};

static void class_init (GeglNodeClass * klass);
static void init(GeglNode *self, GeglNodeClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void remove_inputs (GeglNode * self);
static GeglNode * add_output (GeglNode * self, GeglNode * output);
static void remove_output (GeglNode * self, GeglNode * output);
static gint node_multiplicity (GeglNode * self, GList * list, GeglNode * node);
static void init_traversal (GeglNode * self);

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

      type = g_type_register_static (GEGL_TYPE_OBJECT, "GeglNode", &typeInfo, 0);
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

  g_object_class_install_property (gobject_class, PROP_INPUTS,
                                   g_param_spec_pointer ("inputs",
                                                        "Inputs",
                                                        "The GeglNode's inputs",
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  return;
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->outputs = NULL;
  self->inputs = NULL;
  self->visited = TRUE;
  self->discovered = TRUE;
  self->shared_count = 0;
  self->num_inputs = -1;
  self->enabled = TRUE;
  self->inheriting_input = 0;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  /* Free inputs list and name, outputs should
    be free already */
  remove_inputs(self);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNode *self = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUTS:
      {
        GList *inputs = (GList*)g_value_get_pointer(value);
        if(inputs)
          gegl_node_set_inputs(self, inputs);
      }
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
  GeglNode *self = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUTS:
      g_value_set_pointer (value, (gpointer)gegl_node_get_inputs (self));
      break;
    default:
      break;
  }
}

/**
 * gegl_node_get_inputs:
 * @self: a #GeglNode.
 *
 * Gets a copy of the inputs list of the node.
 *
 * Returns: #GeglNode list of inputs.
 **/
GList * 
gegl_node_get_inputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (GList * )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (GList * )0);

  return g_list_copy(self->inputs);
}

/**
 * gegl_node_get_outputs:
 * @self: a #GeglNode.
 *
 * Gets the outputs list of the node.
 *
 * Returns: #GeglNode list of outputs. 
 **/
GList * 
gegl_node_get_outputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (GList * )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (GList * )0);

  return g_list_copy(self->outputs);
}

/**
 * gegl_node_set_nth_input:
 * @self: a #GeglNode.
 * @input: the node to make the nth input.
 * @n: which input.
 *
 * Sets the nth input of the node.
 *
 * Returns: the #GeglNode that was set.
 **/
GeglNode * 
gegl_node_set_nth_input (GeglNode * self, 
                         GeglNode * input, 
                         gint n)
{
  GList *llink;
  GeglNode *old_input; 

  g_return_val_if_fail (self != NULL, (GeglNode * )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (GeglNode * )0);

  if( n < 0 || n >= gegl_node_num_inputs(self) )
    {
      g_warning( "Cant set input %d\n", n);
      return NULL;
    }

  /* Get the nth link and the old input */
  llink = g_list_nth(self->inputs, n);
  old_input = (GeglNode *)llink->data; 

  /* Remove the old input */
  /*llink->data = NULL;*/ 

  /* Remove ref of the old input, fix its output list */
  if(old_input)
    remove_output(old_input,self);

  /* Put the new input in this link */
  llink->data = (gpointer)input; 

  /* Ref the new input, fix its output list */
  if(input)
    add_output(input,self);

  return input;
}

/**
 * gegl_node_get_nth_input:
 * @self: a #GeglNode.
 * @n: which input.
 *
 * Gets the nth input of the node.
 *
 * Returns: a #GeglNode, the nth input of this node.
 **/
GeglNode * 
gegl_node_get_nth_input (GeglNode * self, 
                         gint n)
{
   g_return_val_if_fail (self != NULL, (GeglNode * )0);
   g_return_val_if_fail (GEGL_IS_NODE (self), (GeglNode * )0);
   return (GeglNode *)g_list_nth_data(self->inputs,n);
}

/**
 * gegl_node_get_nth_output:
 * @self: a #GeglNode.
 * @n: which output.
 *
 * Gets the nth output of the node.
 *
 * Returns: a #GeglNode, the nth input of this node.
 **/
GeglNode * 
gegl_node_get_nth_output (GeglNode * self, 
                          guint n)
{
   g_return_val_if_fail (self != NULL, (GeglNode * )0);
   g_return_val_if_fail (GEGL_IS_NODE (self), (GeglNode * )0);
   return (GeglNode *)g_list_nth_data(self->outputs,n);
}

/**
 * gegl_node_set_inputs:
 * @self: a #GeglNode.
 * @list: list of inputs.
 *
 * Makes a copy of @list and installs this as the inputs 
 * list.
 *
 **/
void 
gegl_node_set_inputs (GeglNode * self, 
                      GList * list)
{
  GList *llink;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  /* If its not the right number of inputs return */
  if((self->num_inputs >= 0  && g_list_length(list) != self->num_inputs)) 
    {
      g_warning("Wrong number of inputs: %d\n", g_list_length(list));
      return;
    }

  if(self->inputs)
   remove_inputs(self);

  self->inputs = g_list_copy(list);

  /* Loop through the inputs, add me as an output */
  llink = self->inputs;

  while(llink)
    {
      GeglNode *input = (GeglNode*)(llink->data);

      /* Add me to the outputs list of this input */
      if(input)
        add_output(input,self);

      llink = g_list_next(llink);
    }
}

/**
 * remove_inputs:
 * @self: a #GeglNode.
 *
 * Removes the current inputs list.
 *
 **/
static void 
remove_inputs (GeglNode * self)
{
  GList *llink;
  gint count;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  llink = self->inputs;

  count = 0;
 
  while(llink)
    {
      GeglNode *input = (GeglNode*)(llink->data);

      /* Remove me as an output of this input */
      if(input)
        remove_output(input,self);

      llink = g_list_next(llink);
      count++;
    }

  /* Remove the list of inputs, not data though. */
  g_list_free(self->inputs);

  self->inputs = NULL;
}

/**
 * add_output:
 * @self: a #GeglNode.
 * @output: a #GeglNode to add to output list.
 *
 * Adds @output to the node's outputs list. 
 *
 * Returns: the added #GeglNode. 
 **/
static GeglNode * 
add_output (GeglNode * self, 
            GeglNode * output)
{
  g_return_val_if_fail (self != NULL, (GeglNode * )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (GeglNode * )0);
   
  g_return_val_if_fail (output != NULL, NULL);
  g_return_val_if_fail ((gegl_node_output_multiplicity(self,output)+1 == 
                         gegl_node_input_multiplicity(output,self)), NULL);

  /* Add the output to the list */
  self->outputs = g_list_append(self->outputs,
                                        (gpointer)output);

  /* ref me, since I am gaining an output */
  g_object_ref(self);

  return output;
}

/**
 * remove_output:
 * @self: a #GeglNode.
 * @output: a #GeglNode to remove from outputs list.
 *
 * Removes @output from the node's outputs list.
 *
 **/
static void 
remove_output (GeglNode * self, 
               GeglNode * output)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (output != NULL);
  g_return_if_fail ((gegl_node_output_multiplicity(self,output) == 
                    gegl_node_input_multiplicity(output,self)));

  /* Remove one occurence of this output */
  self->outputs = g_list_remove(self->outputs, (gpointer)output);

  /* unref me, since I am losing an output */
  g_object_unref(self);
}

/**
 * node_multiplicity:
 * @self: a #GeglNode.
 * @list: list to use.
 * @node: a #GeglNode to look for on @list.
 *
 * Finds the multiplicity of @node on @list. That is, how many times @node
 * appears on @list.
 *
 * Returns: Number of times @node is on @list. 
 **/
static gint 
node_multiplicity (GeglNode * self, 
                   GList * list, 
                   GeglNode * node)
{
  GList * occurence = list;
  gint multiplicity = 0;

  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gint )0);
     
  while (occurence != NULL) 
  {
    occurence = g_list_find(occurence, (gpointer)node);
    if(occurence != NULL)  
    {
       multiplicity++;
       occurence = occurence->next;
    }
  } 

  return multiplicity;
}

/**
 * gegl_node_input_multiplicity:
 * @self: a #GeglNode.
 * @input: a #GeglNode to look for.
 *
 * Finds the multiplicity of @input on the node's inputs list.  That is, how
 * many times @input appears on the inputs list.
 *
 * Returns: How many times @input is on the inputs list. 
 **/
gint 
gegl_node_input_multiplicity (GeglNode * self, 
                              GeglNode * input)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gint )0);

  return node_multiplicity(self, self->inputs, input);
}

/**
 * gegl_node_output_multiplicity:
 * @self: a #GeglNode.
 * @output: a #GeglNode to look for.
 *
 * Finds the multiplicity of @input on the node's outputs list.  That is, how
 * many times @output appears on the outputs list.
 *
 * Returns: How many times @output is on the outputs list. 
 **/
gint 
gegl_node_output_multiplicity (GeglNode * self, 
                               GeglNode * output)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return node_multiplicity(self, self->outputs, output);
}

/**
 * gegl_node_is_leaf:
 * @self: a #GeglNode.
 *
 * Tests to see if the node is a leaf node. A leaf node has no inputs. 
 *
 * Returns: TRUE if is a leaf. 
 **/
gboolean 
gegl_node_is_leaf (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gboolean )0);
   
  return (gegl_node_num_inputs(self) == 0) ? TRUE: FALSE;
}

/**
 * gegl_node_is_root:
 * @self: a #GeglNode.
 *
 * Tests to see if the node is a root node. A root node has no outputs.
 *
 * Returns: TRUE if it is a root. 
 **/
gboolean 
gegl_node_is_root (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gboolean )0);
   
  return (gegl_node_num_outputs(self) == 0) ? TRUE: FALSE;
}

/**
 * gegl_node_num_inputs:
 * @self: a #GeglNode.
 *
 * Gets the number of inputs.
 *
 * Returns: number of inputs. 
 **/
gint 
gegl_node_num_inputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length(self->inputs);
}

/**
 * gegl_node_inheriting_input:
 * @self: a #GeglNode.
 *
 * Gets the inheriting input of this node. Allows navigating down graph in
 * direction of leaves. 
 *
 * Returns: index of the inheriting input. 
 **/
gint 
gegl_node_inheriting_input (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return self->inheriting_input;
}

/**
 * gegl_node_get_enabled:
 * @self: a #GeglNode.
 *
 * Gets whether this node is enabled. 
 *
 * Returns: TRUE if this node is enabled. 
 **/
gboolean 
gegl_node_get_enabled (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gboolean )0);

  return self->enabled;
}

/**
 * gegl_node_set_enabled:
 * @self: a #GeglNode.
 *
 * Sets whether this node is enabled. 
 *
 **/
void 
gegl_node_set_enabled (GeglNode * self, gboolean enabled)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
   
  self->enabled = enabled;
}

/**
 * gegl_node_num_outputs:
 * @self: a #GeglNode.
 *
 * Gets the number of outputs.
 *
 * Returns: number of outputs. 
 **/
gint 
gegl_node_num_outputs (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gint )0);
   
  return g_list_length(self->outputs);
}

/**
 * gegl_node_shared_count:
 * @self: a #GeglNode.
 *
 * Gets the shared count. This is set during a breadth-first search and
 * indicates the number of times a particular node appears as an input to
 * nodes in the graph defined by that breadth-first search.
 *
 * Returns: shared count for a particular breadth-first defined graph. 
 **/
gint 
gegl_node_shared_count (GeglNode * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_NODE (self), (gint )0);
   
  return self->shared_count;
}

/**
 * init_traversal:
 * @self: a #GeglNode.
 *
 * Sets visited and discovered flags to FALSE for this node and all
 * descendants.
 *
 **/
static void 
init_traversal (GeglNode * self)
{
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
   
  /* Set the visited and discovered flags false */
  self->visited = FALSE; 
  self->discovered = FALSE; 
  self->shared_count = 0; 

  llink = self->inputs;

  /* Call init_traversal on my inputs */
  while(llink)
    {
      GeglNode *node = (GeglNode *)llink->data;
      if(node)
        init_traversal(node);
      llink = g_list_next(llink);
    }
}

/**
 * gegl_node_traverse_depth_first:
 * @self: a #GeglNode.
 * @visit_func: function to call on each node.
 * @data: data to pass to each node.
 * @init: init every descendant of this node. 
 *
 * Traverse the graph depth-first from this node through all descendants. 
 *
 **/
void 
gegl_node_traverse_depth_first (GeglNode * self, 
                                GeglNodeTraverseFunc visit_func, 
                                gpointer data, 
                                gboolean init)
{
  GeglNode *node;
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
   

  /* If its the first node, init everything below it */
  if(init)
    init_traversal(self);

  /* Then visit all the inputs first */
  llink = self->inputs;
  while(llink)
    {
      node = (GeglNode *)llink->data;
      if(node && !node->visited)
          gegl_node_traverse_depth_first(node, visit_func, data, FALSE);

      llink = g_list_next(llink);
    }

  /* Visit me last */
  visit_func(self,data);
  self->visited = TRUE;
}

/**
 * gegl_node_traverse_breadth_first:
 * @self: a #GeglNode.
 * @visit_func: function to call on each node.
 * @data: data to pass to each node.
 *
 * Traverse the graph breadth-first from this node through all descendants. 
 *
 **/
void 
gegl_node_traverse_breadth_first (GeglNode * self, 
                                  GeglNodeTraverseFunc visit_func, 
                                  gpointer data)
{
  GList *queue = NULL; 
  GList *first;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));
 
  /* Init all nodes under this one to unvisited */
  init_traversal(self);  

  /* Initialize the queue with me */
  queue = g_list_append(queue,(gpointer)self);
  
  /* Mark me as "discovered" */
  self->discovered = TRUE;

  /* Pop the top of the queue*/
  while((first = g_list_first(queue)))
    {
      /* Get the inputs of this node */
      GeglNode * node = (GeglNode *)(first->data);
      GList * llink = node->inputs;

      /* Delete this one from top of queue */
      queue = g_list_remove_link(queue,first);
      g_list_free_1(first);

      /* Loop through inputs and add to end of 
         queue if not discovered */
      while(llink)
        {
          GeglNode * input = (GeglNode *)llink->data;
          
          /* Add any undiscovered input to the queue at end,
             but skip any null nodes */
          if (input && !input->discovered)
            {
              queue = g_list_append(queue,(gpointer)input);

              /* Mark it as discovered */
              input->discovered = TRUE;
            }

          /* Increase the shared count for this input */
          if(input)
            input->shared_count++;

          /* Next input */
          llink = g_list_next(llink);
        }

      /* Visit the node */
      visit_func(node,data);
      node->visited = TRUE;
    }
}
