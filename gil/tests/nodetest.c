#include <glib-object.h>
#include "gil.h"
#include "gil-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static GilNode *node0, *node1, *node2, *node3;

static void
test_node_g_object_new(Test *test)
{
  {
    GilNode * node = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  

    ct_test(test, node != NULL);
    ct_test(test, GIL_IS_NODE(node));
    ct_test(test, g_type_parent(GIL_TYPE_NODE) == G_TYPE_OBJECT);
    ct_test(test, !strcmp("GilNode", g_type_name(GIL_TYPE_NODE)));

    ct_test(test, GIL_IS_MOCK_NODE(node));
    ct_test(test, g_type_parent(GIL_TYPE_MOCK_NODE) == GIL_TYPE_NODE);
    ct_test(test, !strcmp("GilMockNode", g_type_name(GIL_TYPE_MOCK_NODE)));

    ct_test(test, 0 == gil_node_get_num_children(node)); 
    g_object_unref(node);
  }


  {
    GilNode * node = g_object_new (GIL_TYPE_MOCK_NODE, 
                                   "num_children", 2,
                                   NULL);  

    ct_test(test, 2 == gil_node_get_num_children(node)); 
    g_object_unref(node);
  }

  {
    /*
                       
       node
        |   
      child0 

    */

    GilNode *child0 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);
    GilNode * node = g_object_new (GIL_TYPE_MOCK_NODE, 
                                    "num_children", 1,
                                    "child0", child0, 
                                    NULL);  

    g_object_unref(child0);

    ct_test(test, child0 == gil_node_get_nth_child(node, 0)); 
    ct_test(test, 1 == gil_node_get_num_children(node)); 

    g_object_unref(node);
  }

  {
    GilNode *child0 = g_object_new(GIL_TYPE_MOCK_NODE, NULL);
    GilNode *child1 = g_object_new(GIL_TYPE_MOCK_NODE, NULL);
    GilNode * node = g_object_new (GIL_TYPE_MOCK_NODE, 
                                    "num_children", 2,
                                    "child0", child0, 
                                    "child1", child1, 
                                    NULL);  

    g_object_unref(child0);
    g_object_unref(child1);

    /*

         node
        /   \
     child0 child1

    */

    ct_test(test, child0 == gil_node_get_nth_child(node, 0)); 
    ct_test(test, child1 == gil_node_get_nth_child(node, 1)); 
    ct_test(test, 2 == gil_node_get_num_children(node)); 

    g_object_unref(node);
  }
}

static void
test_node_g_object_get(Test *test)
{
  /*

    node2 node3   node3 num_children = 1
     - -  -       node2 num_children = 2
     |  \/ 
     |  /\  
     | /  \  
     +     +      node1 num_outputs = 1
    node0 node1   node0 num_outputs = 1

  */

  {
    gint num_children; 

    g_object_get(node2, "num_children", &num_children, NULL);
    ct_test(test, 2 == num_children);

    g_object_get(node3, "num_children", &num_children, NULL);
    ct_test(test, 1 == num_children);
  }


  {
    GilNode * child0 = NULL; 
    GilNode * child1 = NULL; 

    g_object_get(node2, "child0", &child0, NULL);
    ct_test(test, child0 == node0);

    g_object_get(node2, "child1", &child1, NULL);
    ct_test(test, child1 == node1);

    g_object_unref(child0);
    g_object_unref(child1);
  }
}


static void
test_node_g_object_set(Test *test)
{
  {
    GilNode *child0 = g_object_new(GIL_TYPE_MOCK_NODE, NULL);
    GilNode *child1 = g_object_new(GIL_TYPE_MOCK_NODE, NULL);
    GilNode * node = g_object_new (GIL_TYPE_MOCK_NODE, "num_children", 2, NULL);  

    g_object_set(node, 
                 "child0", child0, 
                 "child1", child1, 
                 NULL);

    ct_test(test, child0 == gil_node_get_nth_child(node, 0)); 
    ct_test(test, child1 == gil_node_get_nth_child(node, 1)); 

    g_object_unref(node);
    g_object_unref(child0);
    g_object_unref(child1);
  }
}

static void
test_node_add_remove_nodes(Test *test)
{
  {
    /*
         n2  n3 
         |\  /|
         | \/ |
         | /\ |
         n0  n1 
    */

    GilNode *n0, *n1, *n2, *n3;

    n0 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  
    n1 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  
    n2 = g_object_new (GIL_TYPE_MOCK_NODE, "num_children", 2, NULL);  
    n3 = g_object_new (GIL_TYPE_MOCK_NODE, "num_children", 2, NULL);  

    gil_node_set_nth_child(n2, n0, 0);
    gil_node_set_nth_child(n3, n0, 0);

    gil_node_set_nth_child(n2, n1, 1);
    gil_node_set_nth_child(n3, n1, 1);

    g_object_unref(n0); 
    g_object_unref(n1);

    
    /*
         n2    Remove n3 
         |\  
         | \
         |  \ 
         n0  n1 
    */

    g_object_unref(n3);

    /*
       Remove n2 
    */

    g_object_unref(n2);
  }

  {
    /*
         n2  n3 
         |\  /|
         | \/ |
         | /\ |
         n0  n1 
    */

    GilNode *n0, *n1, *n2, *n3;

    n0 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  
    n1 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  
    n2 = g_object_new (GIL_TYPE_MOCK_NODE, "num_children", 2, NULL);  
    n3 = g_object_new (GIL_TYPE_MOCK_NODE, "num_children", 2, NULL);  

    gil_node_set_nth_child(n2, n0, 0);
    gil_node_set_nth_child(n3, n0, 0);

    gil_node_set_nth_child(n2, n1, 1);
    gil_node_set_nth_child(n3, n1, 1);

    g_object_unref(n0); 
    g_object_unref(n1);

    
    /*
         n2  n3  Remove n0 
          \   |
           \  |
            \ |
             n1 
    */

    gil_node_set_nth_child(n2, NULL, 0); 
    gil_node_set_nth_child(n3, NULL, 0); 

    ct_test(test, 2 == gil_node_get_num_children(n2)); 
    ct_test(test, NULL == gil_node_get_nth_child(n2, 0)); 
    ct_test(test, n1 == gil_node_get_nth_child(n2, 1)); 

    ct_test(test, 2 == gil_node_get_num_children(n3)); 
    ct_test(test, NULL == gil_node_get_nth_child(n3, 0)); 
    ct_test(test, n1 == gil_node_get_nth_child(n3, 1)); 

    g_object_unref(n2);
    g_object_unref(n3);

  }
}

static void
node_setup(Test *test)
{
  /*

    node2 node3   <---- node2 has 2 children, node3 has 1 child. 
     - -  -
     |  \/ 
     |  /\  
     | /  \  
     +     +       <---- node0, node1 have one output index.
    node0 node1   

  */

  node0 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  
  node1 = g_object_new (GIL_TYPE_MOCK_NODE, NULL);  

  node2 = g_object_new (GIL_TYPE_MOCK_NODE, 
                        "num_children", 2,
                        "child0", node0,
                        "child1", node1, 
                        NULL);  

  node3 = g_object_new (GIL_TYPE_MOCK_NODE, 
                        "num_children", 1,
                        "child0", node0,
                        NULL);  

}

static void
node_teardown(Test *test)
{
  g_object_unref(node0);
  g_object_unref(node1);
  g_object_unref(node2);
  g_object_unref(node3);
}

Test *
create_node_test()
{
  Test *t = ct_create("GilNodeTest");

  g_assert(ct_addSetUp(t, node_setup));
  g_assert(ct_addTearDown(t, node_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_node_g_object_new));
  g_assert(ct_addTestFun(t, test_node_g_object_get));
  g_assert(ct_addTestFun(t, test_node_g_object_set));
  g_assert(ct_addTestFun(t, test_node_add_remove_nodes));
#endif

  return t;
}
