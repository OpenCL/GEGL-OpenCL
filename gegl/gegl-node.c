#include "gegl-node.h"
#include "gegl-object.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglNodeClass * klass);
static void init(GeglNode *self, GeglNodeClass *klass);
static void finalize(GObject * self_object);

static GList* allocate_input_infos_list(GeglNode * self, gint num_inputs);
static void free_input_infos_list(GeglNode * self);
static GList* allocate_output_list(GeglNode * self, gint num_outputs);
static void free_output_list(GeglNode * self);

static void add_output(GeglNode * self, GeglNode * node, gint index);
static void remove_output (GeglNode * self, GeglNode * node, gint index);

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

  return;
}

static void 
init (GeglNode * self, 
      GeglNodeClass * klass)
{
  self->num_inputs = 0;
  self->input_infos = NULL;
  self->num_outputs = 0;
  self->outputs = NULL;

  self->enabled = TRUE;
  self->visited = TRUE;
  self->discovered = TRUE;
  self->shared_count = 0;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  free_input_infos_list(self);
  free_output_list(self);

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
  GeglInputInfo * input_info = gegl_node_get_nth_input_info(self,n);

  if(!input_info)
    return NULL;
  else 
    return input_info->input;
}

/**
 * gegl_node_get_nth_input_info:
 * @self: a #GeglNode.
 * @n: which input.
 *
 * Allocates and returns an input info for nth input.
 *
 * Returns: an allocated #GeglInputInfo. 
 **/
GeglInputInfo * 
gegl_node_get_nth_input_info (GeglNode * self, 
                              gint n)
{
  GeglInputInfo * info;
  GeglInputInfo * input_info;

  if(n < 0 || n >= self->num_inputs)
      return NULL;

  info = (GeglInputInfo*)g_list_nth_data(self->input_infos, n);
  input_info = g_new(GeglInputInfo, 1);

  input_info->input = info->input; 
  input_info->index = info->index; 

  return input_info;
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
  GeglInputInfo input_info;

  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  input_info.input = input;
  input_info.index = 0;

  gegl_node_set_nth_input_info(self, &input_info, n); 
}


/**
 * gegl_node_set_nth_input_info:
 * @self: a #GeglNode.
 * @input_info: the node and index for the nth input.
 * @n: which input.
 *
 * Sets the nth input info of this node.
 *
 **/
void
gegl_node_set_nth_input_info(GeglNode * self, 
                             GeglInputInfo * input_info, 
                             gint n)
{
  GeglInputInfo * info = NULL;

  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < self->num_inputs); 

  info = (GeglInputInfo*)g_list_nth_data(self->input_infos, n);

  if(info->input)
    {
      remove_output(info->input, self, info->index); 

      /* I lose a reference to old_input. */
      /*
      LOG_DEBUG("set_nth_input",  
                "unreffing %x", (guint)info->input);
      */
      g_object_unref(info->input);
    }

  info->input = input_info->input;
  info->index = input_info->index;

  /* Add me as an output with index. */ 
  if(info->input)
    {
      add_output(info->input, self, input_info->index); 

      /* I gain a reference to source. */
      /*
      LOG_DEBUG("set_nth_input",  
                "reffing %x", (guint)info->input);
      */
      g_object_ref(info->input);
    }
}

static void 
add_output(GeglNode * self,
           GeglNode * node,
           gint index)
{
  GList * llink = NULL;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  g_return_if_fail (index >= 0 && index < self->num_outputs);
  
  llink = g_list_nth(self->outputs, index);
  llink->data = (gpointer)g_list_append((GList*)llink->data, node);
}

static void 
remove_output (GeglNode * self, 
               GeglNode * node,
               gint index)
{
  GList *llink = NULL; 
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  g_return_if_fail (index >= 0 && index < self->num_outputs);

  llink = g_list_nth(self->outputs, index); 
  llink->data = (gpointer)g_list_remove((GList*)llink->data, node);
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
    self->input_infos = allocate_input_infos_list(self, self->num_inputs);
}

static GList* 
allocate_input_infos_list(GeglNode * self, 
                          gint num_inputs)
{
  gint i;
  GList * llink = NULL;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (num_inputs >= 0, NULL);

  for(i = 0; i < num_inputs; i++)
    {
      GeglInputInfo * input_info = g_new(GeglInputInfo, 1);
      input_info->input = NULL;
      input_info->index = 0;
      llink = g_list_append(llink, input_info);
    }

  return llink;
}

static void
free_input_infos_list(GeglNode * self)
{
  GList * llink;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  llink = self->input_infos;

  while(llink)
    {
      GeglInputInfo * input_info = (GeglInputInfo*)llink->data;

      if(input_info->input)
        {
          remove_output(input_info->input, self, input_info->index); 
          /*
          LOG_DEBUG("free_input_infos_list",  
                    "unreffing %x", (guint)input_info->input);
          */
          g_object_unref(input_info->input);

        }

      input_info->input = NULL;

      g_free(input_info);

      llink = g_list_next(llink);
    }

  g_list_free(self->input_infos);
  self->input_infos = NULL;
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
    self->outputs = allocate_output_list(self, self->num_outputs);
}

static GList * 
allocate_output_list(GeglNode * self,
                     gint num_outputs)
{
  gint i;
  GList * llink = NULL;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (num_outputs >= 0, NULL);

  for(i = 0; i < num_outputs; i++)
    llink = g_list_append(llink, NULL);

  return llink;
}

static void
free_output_list(GeglNode * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  /* All contents should be gone from each index*/
  g_list_free(self->outputs);
  self->outputs = NULL;
}

/**
 * gegl_node_get_outputs:
 * @self: a #GeglNode.
 * @index_num_outputs: number of outputs for this output index.
 * @index: which output index.
 *
 * Allocates and returns an array of outputs for the
 * passed index number. 
 *
 * Returns: array of GeglNode pointers for the given index. 
 **/
GeglNode **
gegl_node_get_outputs (GeglNode * self, 
                       gint *index_num_outputs,
                       gint index)
{
  GList *outputs_list = NULL;
  GeglNode **outputs = NULL;
  gint i;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  g_return_val_if_fail (index >= 0 || index < self->num_outputs, NULL);

  outputs_list = (GList*)g_list_nth_data(self->outputs, index);
  *index_num_outputs = g_list_length(outputs_list);

  if(*index_num_outputs)
    {
      outputs = g_new(GeglNode*, *index_num_outputs);

      for(i =0 ; i < *index_num_outputs; i++) 
        outputs[i] = (GeglNode*)g_list_nth_data(outputs_list, i);
    }

  return outputs;
}

/**
 * gegl_node_get_input_infos:
 * @self: a #GeglNode.
 *
 * Allocates and returns an array of input infos.
 *
 * Returns: array of input infos. 
 **/
GeglInputInfo*
gegl_node_get_input_infos (GeglNode * self)
{
  GeglInputInfo *input_infos = NULL;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  if(self->num_inputs > 0)
    {
      gint i;
      input_infos = g_new(GeglInputInfo, self->num_inputs);

      for(i =0 ; i < self->num_inputs; i++) 
        {
          GeglInputInfo *input_info = (GeglInputInfo*)g_list_nth_data(self->input_infos, i); 
          input_infos[i].input = input_info->input; 
          input_infos[i].index = input_info->index; 
        }
    }

  return input_infos;
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

  llink = self->input_infos;

  /* Call init_traversal on my inputs */
  while(llink)
    {
      GeglInputInfo *input_info = (GeglInputInfo *)llink->data;

      if(input_info->input)
        init_traversal(input_info->input);

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
  GList *llink;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_NODE (self));

  /* If its the first node, init everything below it */
  if(init)
    init_traversal(self);

  /* Then visit all the inputs first */
  llink = self->input_infos;
  while(llink)
    {
      GeglInputInfo *input_info = (GeglInputInfo *)llink->data;
      if(input_info->input && !input_info->input->visited) 
          gegl_node_traverse_depth_first(input_info->input, visit_func, data, FALSE);

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
      GList * llink = node->input_infos;

      /* Delete this one from top of queue */
      queue = g_list_remove_link(queue,first);
      g_list_free_1(first);

      /* Loop through inputs and add to end of 
         queue if not discovered */
      while(llink)
        {
          GeglInputInfo * input_info = (GeglInputInfo *)llink->data;
          
          /* Add any undiscovered input to the queue at end,
             but skip any null nodes */
          if (input_info->input && !input_info->input->discovered)
            {
              queue = g_list_append(queue,(gpointer)input_info->input);

              /* Mark it as discovered */
              input_info->input->discovered = TRUE;
            }

          /* Next input */
          llink = g_list_next(llink);
        }

      /* Visit the node */
      visit_func(node,data);
      node->visited = TRUE;
    }
}
