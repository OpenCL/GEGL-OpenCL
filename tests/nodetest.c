#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglNode * output1;
static GeglNode * output2;
static GeglNode * input1;
static GeglNode * input2;
static GeglNode *A,*B,*C,*D,*E,*F,*G,*H;

static GList *
make_null_list(gint length)
{
  gint i;
  GList *list = NULL;

  /* Allocate links with NULL data */
  for(i = 0; i < length; i++) 
    list = g_list_append(list, NULL);

  return list; 
}

static void
test_node_g_object_new(Test *test)
{
  {
    GeglNode * node = g_object_new (GEGL_TYPE_NODE, NULL);  

    ct_test(test, node != NULL);
    ct_test(test, GEGL_IS_NODE(node));
    ct_test(test, g_type_parent(GEGL_TYPE_NODE) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglNode", g_type_name(GEGL_TYPE_NODE)));

    ct_test(test, 0 == gegl_node_num_inputs(node)); 
    g_object_unref(node);
  }

  {
    GeglNode *in1 = g_object_new(GEGL_TYPE_NODE, NULL);
    GeglNode *in2 = g_object_new(GEGL_TYPE_NODE, NULL);
    GeglNode * node;   
    GList * inputs = NULL; 

    inputs = g_list_append(inputs, in1);
    inputs = g_list_append(inputs, in2);

    node = g_object_new (GEGL_TYPE_NODE, "inputs", inputs, NULL);  

    g_list_free(inputs);

    /*
        node  
         |\  
         | \
         |  \ 
        in1 in2 
    */

    ct_test(test, in1 == gegl_node_get_nth_input(node, 0)); 
    ct_test(test, in2 == gegl_node_get_nth_input(node, 1)); 
    ct_test(test, 2 == gegl_node_num_inputs(node)); 
    ct_test(test, 1 == gegl_node_num_outputs(in1)); 
    ct_test(test, 1 == gegl_node_num_outputs(in2)); 

    g_object_unref(node);
    g_object_unref(in1);
    g_object_unref(in2);
  }

  {
    GList *null_inputs = make_null_list(2);
    GeglNode * node = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    GeglNode * in1 = gegl_node_get_nth_input(node, 0);
    GeglNode * in2 = gegl_node_get_nth_input(node, 1);

    g_list_free(null_inputs);

    /*
        node  
         |\  
         | \
         |  \ 
        NULL NULL 
    */

    ct_test(test, in1 == NULL); 
    ct_test(test, in2 == NULL); 
    ct_test(test, gegl_node_num_inputs(node) == 2); 

    g_object_unref(node);
  }
}

static void
test_node_g_object_get(Test *test)
{
  {
    GList *inputs = NULL; 
    GeglNode * in1 = NULL; 
    GeglNode * in2 = NULL;

    g_object_get(output1, "inputs", &inputs, NULL);

    in1 = gegl_node_get_nth_input(output1, 0);
    in2 = gegl_node_get_nth_input(output1, 1);

    ct_test(test, 2 == g_list_length(inputs));
    ct_test(test, 2 == gegl_node_num_inputs(output1));
    ct_test(test, in1 == (GeglNode*)g_list_nth_data(inputs, 0));
    ct_test(test, in2 == (GeglNode*)g_list_nth_data(inputs, 1));
    ct_test(test, in1 == input1);
    ct_test(test, in2 == input2);

    g_list_free(inputs);
  }
}

static void
test_node_g_object_set(Test *test)
{
  {
    GeglNode *in1 = g_object_new (GEGL_TYPE_NODE, NULL);  
    GeglNode *in2 = g_object_new (GEGL_TYPE_NODE, NULL);  
    GList *inputs = g_list_append(NULL, in1);
    inputs = g_list_append(inputs, in2);

    g_object_set(output1, "inputs", inputs, NULL);
    g_list_free(inputs);

    /*
        output1 
         |\  
         | \
         |  \ 
        in1 in2 
    */

    ct_test(test, 2 == gegl_node_num_inputs(output1));
    ct_test(test, in1 == gegl_node_get_nth_input(output1, 0));
    ct_test(test, in2 == gegl_node_get_nth_input(output1, 1));

    g_object_unref(in1);
    g_object_unref(in2);
  }
}

static void
test_node_get_inputs(Test *test)
{
  {
    GList *inputs = gegl_node_get_inputs(output1);
    GeglNode * in1 = gegl_node_get_nth_input(output1, 0);
    GeglNode * in2 = gegl_node_get_nth_input(output1, 1);

    ct_test(test, 2 == g_list_length(inputs));
    ct_test(test, 2 == gegl_node_num_inputs(output1));
    ct_test(test, in1 == (GeglNode*)g_list_nth_data(inputs, 0));
    ct_test(test, in2 == (GeglNode*)g_list_nth_data(inputs, 1));
    ct_test(test, in1 == input1);
    ct_test(test, in2 == input2);

    g_list_free(inputs);
  }

  {
    GList *inputs = gegl_node_get_inputs(output2);
    GeglNode * in1 = gegl_node_get_nth_input(output2, 0);

    ct_test(test, g_list_length(inputs) == 1);
    ct_test(test, gegl_node_num_inputs(output2) == 1);
    ct_test(test, in1 == input1);

    g_list_free(inputs);
  }
}

static void
test_node_set_inputs(Test *test)
{
  GeglNode *in1 = g_object_new (GEGL_TYPE_NODE, NULL);  
  GeglNode *in2 = g_object_new (GEGL_TYPE_NODE, NULL);  
  GList *inputs = g_list_append(NULL, in1);
  inputs = g_list_append(inputs, in2);

  gegl_node_set_inputs(output1, inputs);
  g_list_free(inputs);

  /*
     output1  
       |\  
       | \
       |  \ 
     in1 in2 
  */

  ct_test(test, gegl_node_num_inputs(output1) == 2);
  ct_test(test, in1 == gegl_node_get_nth_input(output1, 0));
  ct_test(test, in2 == gegl_node_get_nth_input(output1, 1));

  g_object_unref(in1);
  g_object_unref(in2);
}

static void
test_node_get_outputs(Test *test)
{
  /* outputs of input1 should be output1, output2 */
  {
    GList *outputs = gegl_node_get_outputs(input1);

    ct_test(test, g_list_length(outputs) == 2); 
    ct_test(test, gegl_node_num_outputs(input1) == 2); 
    ct_test(test, output1 == gegl_node_get_nth_output(input1,0)); 
    ct_test(test, output2 == gegl_node_get_nth_output(input1,1)); 
    g_list_free(outputs);
  }

  /* outputs of input2 -- should be output1 */
  {
    GList *outputs = gegl_node_get_outputs(input2);

    ct_test(test, g_list_length(outputs) == 1); 
    ct_test(test, gegl_node_num_outputs(input2) == 1); 
    ct_test(test, output1 == gegl_node_get_nth_output(input2, 0)); 
    g_list_free(outputs);
  }
}

static void
test_node_is_leaf_or_root(Test *test)
{
  ct_test(test, gegl_node_is_leaf(input1) &&
                !gegl_node_is_root(input1)); 

  ct_test(test, gegl_node_is_leaf(input2) &&
                !gegl_node_is_root(input2)); 

  ct_test(test, gegl_node_is_root(output1) &&
                !gegl_node_is_leaf(output1));

  ct_test(test, gegl_node_is_root(output2) &&
                !gegl_node_is_leaf(output2));
}

static void
test_node_removing_nodes(Test *test)
{
  GList * null_inputs; 
  GeglNode *I1, *I2, *O1, *O2;

  null_inputs = make_null_list(1); 
  I1 = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
  g_list_free(null_inputs);

  I2 = g_object_new (GEGL_TYPE_NODE, NULL);  

  null_inputs = make_null_list(2); 
  O1 = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
  g_list_free(null_inputs);

  null_inputs = make_null_list(2); 
  O2 = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
  g_list_free(null_inputs);

  gegl_node_set_nth_input(O1, I1, 0);
  gegl_node_set_nth_input(O1, I2, 1);

  gegl_node_set_nth_input(O2, I1, 0);
  gegl_node_set_nth_input(O2, I2, 1);

  /*
       O1  O2
       |\  /|
       | \/ |
       | /\ |
       I1  I2 
  */


  ct_test(test, gegl_node_num_inputs(O1) == 2); 
  ct_test(test, gegl_node_num_inputs(O2) == 2); 
  ct_test(test, gegl_node_num_inputs(I1) == 1); 
  ct_test(test, gegl_node_num_inputs(I2) == 0); 
  ct_test(test, gegl_node_num_outputs(I1) == 2); 
  ct_test(test, gegl_node_num_outputs(I2) == 2); 

  g_object_unref(O2);

  /*
       O1  
       |\  
       | \
       |  \ 
      I1  I2 
  */
  
  ct_test(test, gegl_node_num_outputs(I1) == 1); 
  ct_test(test, gegl_node_num_outputs(I2) == 1); 

  g_object_unref(O1);

  /*
      I1  I2 
  */
           
  ct_test(test, gegl_node_num_outputs(I1) == 0); 
  ct_test(test, gegl_node_num_outputs(I2) == 0); 
  g_object_unref(I1);
  g_object_unref(I2);
}

gboolean 
traverse_depth_first_whole_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "A", "B", "C", "D", "G", "F", "E" };  
  static gint count = 0; 
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[count]));
  count++;
  return TRUE;
} 

gboolean 
traverse_depth_first_part_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "A", "B", "C", "D" };  
  static gint count = 0; 
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[count]));
  count++;
  return TRUE;
} 


/**
  From node_setup: 

         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

**/
static void
test_depth_first_traversal(Test *test)
{
  gegl_node_traverse_depth_first (E, traverse_depth_first_whole_graph, test, TRUE); 
  gegl_node_traverse_depth_first (D, traverse_depth_first_part_graph, test, TRUE); 
}

gboolean 
traverse_breadth_first_whole_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "E", "D", "F", "B", "C", "G", "A" };  
  static gint count = 0; 
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[count]));
  count++;
  return TRUE;
} 

gboolean 
traverse_breadth_first_part_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "D", "B", "C", "A" };  
  static gint count = 0; 
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[count]));
  count++;
  return TRUE;
} 

/**
  From node_setup: 

         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

**/
static void
test_breadth_first_traversal(Test *test)
{
  gegl_node_traverse_breadth_first (E, traverse_breadth_first_whole_graph, test); 
  ct_test (test, 2 == gegl_node_shared_count(A));
  ct_test (test, 1 == gegl_node_shared_count(B));

  gegl_node_traverse_breadth_first (D, traverse_breadth_first_part_graph, test); 
  ct_test (test, 2 == gegl_node_shared_count(A));
  ct_test (test, 1 == gegl_node_shared_count(B));
}


static void
node_setup(Test *test)
{
  GList * inputs1 = NULL; 
  GList * inputs2 = NULL; 

  /*

   output1 output2    <---outputs
       |\  /
       | \/ 
       | /\ 
   input1  input2     <---inputs

  */

  /* Add input1 and input2 to output1 */
  input1 = g_object_new (GEGL_TYPE_NODE, NULL);  
  inputs1 = g_list_append(inputs1, input1); 

  input2 = g_object_new (GEGL_TYPE_NODE, NULL);  
  inputs1 = g_list_append(inputs1, input2); 

  output1 = g_object_new (GEGL_TYPE_NODE, "inputs", inputs1, NULL);  
  g_list_free(inputs1);

  
  /* Add input1 and to output2 */
  inputs2 = g_list_append(inputs2, input1);
  output2 = g_object_new (GEGL_TYPE_NODE, "inputs", inputs2, NULL);  
  g_list_free(inputs2);

  /*
         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

  */

  {
    GList *null_inputs;

    A = g_object_new (GEGL_TYPE_NODE, NULL);  

    null_inputs = make_null_list(1); 
    B = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    null_inputs = make_null_list(1); 
    C = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    null_inputs = make_null_list(2); 
    D = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    null_inputs = make_null_list(2); 
    E = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    null_inputs = make_null_list(2); 
    F = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    G = g_object_new (GEGL_TYPE_NODE, NULL);  

    null_inputs = make_null_list(1); 
    H = g_object_new (GEGL_TYPE_NODE, "inputs", null_inputs, NULL);  
    g_list_free(null_inputs);

    g_object_set_data(G_OBJECT(A), "name", "A");
    g_object_set_data(G_OBJECT(B), "name", "B");
    g_object_set_data(G_OBJECT(C), "name", "C");
    g_object_set_data(G_OBJECT(D), "name", "D");
    g_object_set_data(G_OBJECT(E), "name", "E");
    g_object_set_data(G_OBJECT(F), "name", "F");
    g_object_set_data(G_OBJECT(G), "name", "G");
    g_object_set_data(G_OBJECT(H), "name", "H");

    gegl_node_set_nth_input(B, A, 0);
    gegl_node_set_nth_input(C, A, 0);
    gegl_node_set_nth_input(D, B, 0);
    gegl_node_set_nth_input(D, C, 1);
    gegl_node_set_nth_input(E, D, 0);
    gegl_node_set_nth_input(E, F, 1);
    gegl_node_set_nth_input(F, G, 0);
    gegl_node_set_nth_input(H, A, 0);
  }
}

static void
node_teardown(Test *test)
{
  g_object_unref(output1);
  g_object_unref(output2);
  g_object_unref(input1);
  g_object_unref(input2);

  g_object_unref(A);
  g_object_unref(B);
  g_object_unref(C);
  g_object_unref(D);
  g_object_unref(E);
  g_object_unref(F);
  g_object_unref(G);
  g_object_unref(H);
}

Test *
create_node_test()
{
  Test *t = ct_create("GeglNodeTest");

  g_assert(ct_addSetUp(t, node_setup));
  g_assert(ct_addTearDown(t, node_teardown));
  g_assert(ct_addTestFun(t, test_node_g_object_new));
  g_assert(ct_addTestFun(t, test_node_g_object_get));
  g_assert(ct_addTestFun(t, test_node_g_object_set));
  g_assert(ct_addTestFun(t, test_node_get_inputs));
  g_assert(ct_addTestFun(t, test_node_set_inputs));
  g_assert(ct_addTestFun(t, test_node_get_outputs));
  g_assert(ct_addTestFun(t, test_node_is_leaf_or_root));
  g_assert(ct_addTestFun(t, test_node_removing_nodes));
  g_assert(ct_addTestFun(t, test_depth_first_traversal));
  g_assert(ct_addTestFun(t, test_breadth_first_traversal));

  return t;
}
