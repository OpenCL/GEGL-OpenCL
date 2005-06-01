#ifndef __GIL_AST_H__
#define __GIL_AST_H__

#include <glib-2.0/glib.h>

typedef _GilStatementBlock GilStatementBlock;
struct _GilStatementBlock
{
};

typedef _GilStatementExpr GilStatementExpr;
struct _GilStatementExpr
{
};

typedef _GilStatementDecl GilStatementDecl;
struct _GilStatementDecl
{
};

/* statement kinds */
typedef enum
{
  GIL_STATEMENT_BLOCK,
  GIL_STATEMENT_EXPR,
  GIL_STATEMENT_DECL,
} GilStatementKind;

typedef struct _GilStatement GilStatement;
struct _GilStatement
{	
   GilStatementKind kind;   /* the kind of node */
   union
   {
     GilStatementBlock block;
     GilStatementExpr expr;
     GilStatementDecl decl;
   }u;
};

GNode *gil_statement_block_new();
GNode *gil_statement_expr_new();
GNode *gil_statement_decl_new();
GNode *gil_statement_free();

/* A generic constructor */
GNode *gil_node_new(GilNodeData *node_data, gint nops, ...);

/* Free nodes and data */
void gil_node_free(GNode *node, gpointer data);
gboolean gil_node_free_data(GNode *node, gpointer data);

#endif
