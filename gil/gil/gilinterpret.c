#include <stdio.h>
#include "gilast.h"
#include "gilinterpret.h"

/* Various tree traversal functions */

static void
print_tabs(int tabs) {
   int i;
   for (i = 0; i < tabs; i++)
      printf("  ");
}


void
gil_print_ast(GNode *node,
              gint tabs)
{
  GilNodeData * data;
  if (!node) return;

  data = node->data;

  print_tabs(tabs);
  printf("(");
  switch (data->kind)
  {
    case GIL_NODE_KIND_INT_CONST:
       /* int leaf node: print "int" then the value on the one line */
       printf("int const %d", data->value.intVal);
       printf(")");
       break;
    case GIL_NODE_KIND_FLOAT_CONST:
       /* int leaf node: print "int" then the value on the one line */
       printf("float const %f", data->value.floatVal);
       printf(")");
       break;
    case GIL_NODE_KIND_ID:
       /* string leaf node: print "id" then the string on the one line */
       printf("id %s", data->value.idVal);
       printf(")");
       break;
    case GIL_NODE_KIND_OP:
        {
          GNode *n1 = g_node_nth_child(node,0);
          GNode *n2 = g_node_nth_child(node,1);
          GNode *n3 = g_node_nth_child(node,2);

          printf("%c", (char)(data->value.opVal));
          printf("\n");
          if(n1)
            {
                gil_print_ast(n1, tabs+1);
                printf("\n");
            }
          if(n2)
            {
                gil_print_ast(n2, tabs+1);
                printf("\n");
            }
          if(n3)
            {
                gil_print_ast(n3, tabs+1);
                printf("\n");
            }
          print_tabs(tabs);
          printf(")");
          break;
        }
    case GIL_NODE_KIND_STATEMENT_LIST:
       {
         int num_children = g_node_n_children(node);
         int i;
         printf("statement list");
         printf("\n");
         for(i=0; i<num_children; i++)
         {
            GNode *child = g_node_nth_child(node,i);
            gil_print_ast(child, tabs+1);
            printf("\n");
         }
         print_tabs(tabs);
         printf(")");
       }
       break;
    case GIL_NODE_KIND_BLOCK:
       {
          GNode *n1 = g_node_nth_child(node,0);

          printf("block");
          printf("\n");
          if(n1)
            {
                gil_print_ast(n1, tabs+1);
                printf("\n");
            }
           print_tabs(tabs);
           printf(")");
       }
       break;
    default:
       printf("UNKNOWN TYPE");
       break;
  }
}

#if 0
int
gil_evaluate(GNode *root)
{
    GilNodeData * data;
    if (!node) return 0;

    data = node->data;
    switch(data->kind)
    {
      default:
        return 0;

      case GIL_CONSTANT:  return data->value.intVal;
      case GIL_ID:        return sym[data->value.idVal];
      case GIL_OP:
        {
          GNode *n1 = g_node_nth_child(node,0);
          GNode *n2 = g_node_nth_child(node,1);
          GNode *n3 = g_node_nth_child(node,2);
          guint num_nodes = g_node_n_children(node);

          switch(data->value.opVal)
          {
            default:
                return 0;
            case WHILE:
                            while(gil_evaluate(n1))
                                gil_evaluate(n2);
                            return 0;

            case IF:        if (gil_evaluate(n1))
                                gil_evaluate(n2);
                            else if (num_nodes > 2)
                                gil_evaluate(n3);
                            return 0;
            case PRINT:     printf("%d\n", gil_evaluate(n1));
                            return 0;
            case ';':       gil_evaluate(n1);
                            return gil_evaluate(n2);
            case '=':       return sym[((GilNodeData*)(n1->data))->value.idVal] = gil_evaluate(n2);
            case UMINUS:    return -gil_evaluate(n1);
            case '+':       return gil_evaluate(n1) + gil_evaluate(n2);
            case '-':       return gil_evaluate(n1) - gil_evaluate(n2);
            case '*':       return gil_evaluate(n1) * gil_evaluate(n2);
            case '/':       return gil_evaluate(n1) / gil_evaluate(n2);
            case '<':       return gil_evaluate(n1) < gil_evaluate(n2);
            case '>':       return gil_evaluate(n1) > gil_evaluate(n2);
            case GE:        return gil_evaluate(n1) >= gil_evaluate(n2);
            case LE:        return gil_evaluate(n1) <= gil_evaluate(n2);
            case NE:        return gil_evaluate(n1) != gil_evaluate(n2);
            case EQ:        return gil_evaluate(n1) == gil_evaluate(n2);
          }
        }
    }
}
#endif
