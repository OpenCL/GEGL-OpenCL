#ifndef __GIL_AST_H__
#define __GIL_AST_H__

#include <glib.h>

/* Node kinds */
typedef enum 
{ 
  GIL_NODE_KIND_INT_CONST, 
  GIL_NODE_KIND_FLOAT_CONST, 
  GIL_NODE_KIND_ID, 
  GIL_NODE_KIND_OP,             
  GIL_NODE_KIND_STATEMENT_LIST, 
  GIL_NODE_KIND_DECLARATION_LIST, 
  GIL_NODE_KIND_BLOCK 
} GilNodeKind;

/* Values for each node */
typedef union _GilNodeValue GilNodeValue;
union _GilNodeValue
{
   gint intVal;       /*int value*/
   gfloat floatVal;   /*float value*/
   gchar *idVal;      /*name of identifier*/
   gint opVal;        /*eg MUL,DIV,PLUS,MINUS, while, if-then, etc*/
};

/* Data for the GNodes - a kind and a value */
typedef struct _GilNodeData GilNodeData; 
struct _GilNodeData 
{	
   GilNodeKind kind;        /* the kind of node */
   GilNodeValue value;      /* union of node values */
};

/* Constructors for the nodes */
GNode *gil_node_int_constant_new(gint intVal);
GNode *gil_node_float_constant_new(gfloat floatVal);
GNode *gil_node_id_new(gchar *idVal);
GNode *gil_node_op_new(gint opVal, gint nops, ...);
GNode *gil_node_statement_list_new(void);
GNode *gil_node_decl_list_new(void);
GNode *gil_node_block_new(void);

/* A generic constructor */
GNode *gil_node_new(GilNodeData *node_data, gint nops, ...);

/* Free nodes and data */
void gil_node_free(GNode *node, gpointer data); 
gboolean gil_node_free_data(GNode *node, gpointer data); 

#endif
