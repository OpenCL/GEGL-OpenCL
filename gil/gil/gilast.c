#include <stdio.h>
#include "gilast.h"
#include "gil-node.h"

GNode *
gil_node_new(GilNodeData * data,
             gint num_children,
             ...)
{
    va_list ap;
    gint i;
    GNode *node = g_node_new (data);

    va_start(ap, num_children);
    for (i = 0; i < num_children; i++)
      g_node_append(node, va_arg(ap, GNode*));
    va_end(ap);
    return node;
}

static void test()
{
  GilNode *node = g_object_new(GIL_TYPE_NODE, NULL);
}

GNode *
gil_node_statement_list_new()
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind= GIL_NODE_KIND_STATEMENT_LIST;
   return gil_node_new(data,0);
}

GNode *
gil_node_declaration_list_new()
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind = GIL_NODE_KIND_DECLARATION_LIST;
   return gil_node_new(data,0);
}

GNode *
gil_node_block_new()
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind = GIL_NODE_KIND_BLOCK;
   return gil_node_new(data,0);
}

GNode *
gil_node_int_constant_new(gint intVal)
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind = GIL_NODE_KIND_INT_CONST;
   data->value.intVal = intVal;
   return gil_node_new(data,0);
}

GNode *
gil_node_float_constant_new(gfloat floatVal)
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind = GIL_NODE_KIND_FLOAT_CONST;
   data->value.floatVal = floatVal;
   return gil_node_new(data,0);
}

GNode *
gil_node_id_new(gchar *idVal)
{
   GilNodeData *data = g_new (GilNodeData,1);
   data->kind = GIL_NODE_KIND_ID;
   data->value.idVal = idVal;
   return gil_node_new(data,0);
}

GNode *
gil_node_op_new(gint opVal,
                gint num_children,
                ...)
{
    va_list ap;
    gint i;
    GNode *node;
    GilNodeData *data = g_new (GilNodeData,1);
    data->kind = GIL_NODE_KIND_OP;
    data->value.opVal = opVal;
    node = g_node_new (data);

    va_start(ap, num_children);
    for (i = 0; i < num_children; i++)
      g_node_append(node, va_arg(ap, GNode*));
    va_end(ap);
    return node;
}

/* Frees the data at a node. */
gboolean
gil_node_free_data(GNode *node,
                   gpointer data)
{
    g_free(node->data);
}

/* Free all data and nodes under this node. */
void
gil_node_free(GNode *node,
              gpointer data)
{
    /* Free the data at each node from here down. */
    g_node_traverse(node, G_IN_ORDER, G_TRAVERSE_ALL,
                    -1, gil_node_free_data, NULL);

    /* Now free all the nodes from here down. */
    g_node_destroy(node);
}
