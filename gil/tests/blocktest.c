#include <glib-object.h>
#include "gil.h"
#include "gil-mock-dfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static GilNode *const_2, *const_1pt5; 
static GilNode *A, *B, *C;
static GilNode *mult, *plus;
static GilNode *statement1, *statement2;

static void
test_block_g_object_new(Test *test)
{
  {
    GilBlock * block = g_object_new (GIL_TYPE_BLOCK, NULL);  

    ct_test(test, block != NULL);
    ct_test(test, GIL_IS_BLOCK(block));
    ct_test(test, g_type_parent(GIL_TYPE_BLOCK) == GIL_TYPE_STATEMENT);
    ct_test(test, !strcmp("GilBlock", g_type_name(GIL_TYPE_BLOCK)));

    g_object_unref(block);
  }
}

static void
test_block_add_children(Test *test)
{
  /***

     {
       A = B + 1.5; 
       C = 2 * A;
     }

     parse tree for above: 

       block
      /     \ 
 statement1 statement2
  /  \       /  \ 
 A    +     C    * 
     / \        / \ 
    B  1.5     2   A (<--A same as other A)


  ***/

  {
    GilNode *block = g_object_new (GIL_TYPE_BLOCK, NULL);  

    gil_node_append_child(block, statement1);
    gil_node_append_child(block, statement2);

    ct_test(test, 2  == gil_node_get_num_children(block));

    g_object_unref(block);
  }
}

static void
test_block_dfs_visitor_traverse(Test *test)
{
  GilNode * block;

  {
    block = g_object_new (GIL_TYPE_BLOCK, NULL);  

    gil_node_append_child(block, statement1);
    gil_node_append_child(block, statement2);
  }

  /***

     {
       A = B + 1.5; 
       C = 2 * A;
     }

       block
      /     \ 
 statement1 statement2
  /  \       /  \ 
 A    +     C    * 
     / \        / \ 
    B  1.5     2   A 

  ***/

  /* See if we do a dfs traversal we get what we expect */
  {
    gint i;
    GilNode * visit_objects[] = {A, B, const_1pt5, plus, statement1, 
                                 C, const_2, mult, statement2, block};  

    GilMockDfsVisitor *mock_dfs_visitor = g_object_new(GIL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_objects_list;

    gil_dfs_visitor_traverse(GIL_DFS_VISITOR(mock_dfs_visitor), block); 
    visits_objects_list = 
      gil_visitor_get_visits_objects_list(GIL_VISITOR(mock_dfs_visitor));

    for(i = 0; i < g_list_length(visits_objects_list); i++)
      {
        GilNode *visit_object = g_list_nth_data(visits_objects_list, i);
        ct_test (test, visit_objects[i] == visit_object);
      }

    g_object_unref(mock_dfs_visitor);
  }

  g_object_unref(block);
}


static void
block_setup(Test *test)
{
    /* 
         A = B + 1.5; 
         C = 2 * A;
    */ 


    const_1pt5 = g_object_new(GIL_TYPE_CONSTANT, 
                             "type", GIL_FLOAT, 
                             "float", 1.5, 
                             NULL); 

    B = g_object_new(GIL_TYPE_VARIABLE, "name", "B", NULL); 

    plus = g_object_new (GIL_TYPE_BINARY_OP, 
                         "op", GIL_PLUS, 
                         "left_operand", B,
                         "right_operand", const_1pt5,
                         NULL);  

    A = g_object_new(GIL_TYPE_VARIABLE, "name", "A", NULL); 
    statement1 = g_object_new (GIL_TYPE_EXPR_STATEMENT, 
                               "left_expr", A,
                               "right_expr", plus,
                               NULL);  


    const_2 = g_object_new(GIL_TYPE_CONSTANT, 
                           "type", GIL_INT, 
                           "int", 2, 

                           NULL); 
    mult = g_object_new (GIL_TYPE_BINARY_OP, 
                         "op", GIL_MULT, 
                         "left_operand", const_2,
                         "right_operand", A,
                         NULL);  

    C = g_object_new(GIL_TYPE_VARIABLE, "name", "C", NULL); 
    statement2 = g_object_new (GIL_TYPE_EXPR_STATEMENT, 
                               "left_expr", C,
                               "right_expr", mult,
                               NULL);  
}


static void
block_teardown(Test *test)
{
  g_object_unref(statement1);
  g_object_unref(statement2);
  g_object_unref(A);
  g_object_unref(plus);
  g_object_unref(mult);
  g_object_unref(B);
  g_object_unref(C);
  g_object_unref(const_2);
  g_object_unref(const_1pt5);
}

Test *
create_block_test()
{
  Test *t = ct_create("GilBlockTest");

  g_assert(ct_addSetUp(t, block_setup));
  g_assert(ct_addTearDown(t, block_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_block_g_object_new));
  g_assert(ct_addTestFun(t, test_block_add_children));
  g_assert(ct_addTestFun(t, test_block_dfs_visitor_traverse));
#endif

  return t;
}
