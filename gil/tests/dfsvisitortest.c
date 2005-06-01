#include <glib-object.h>
#include "gil.h"
#include "gil-mock-node.h"
#include "gil-mock-dfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static GilNode *A,*B,*C,*D,*E,*F,*G;
static GilNode *O,*P,*Q,*R,*S;

static void
test_dfs_visitor_g_object_new(Test *test)
{
  {
    GilMockDfsVisitor * mock_dfs_visitor = g_object_new (GIL_TYPE_MOCK_DFS_VISITOR, NULL);

    ct_test(test, mock_dfs_visitor != NULL);
    ct_test(test, GIL_IS_MOCK_DFS_VISITOR(mock_dfs_visitor));
    ct_test(test, g_type_parent(GIL_TYPE_MOCK_DFS_VISITOR) == GIL_TYPE_DFS_VISITOR);
    ct_test(test, !strcmp("GilMockDfsVisitor", g_type_name(GIL_TYPE_MOCK_DFS_VISITOR)));

    g_object_unref(mock_dfs_visitor);
  }
}

/**

  From setup:

         E
        / \
       D   F
      /\    \
     B  C    G
     | /
     A

**/
static void
test_dfs_visitor_traverse(Test *test)
{
  gchar * name;
  {
    gint i;
    gchar * visit_names[] = {"A", "B", "C", "D", "G", "F", "E"};
    GilMockDfsVisitor *mock_dfs_visitor = g_object_new(GIL_TYPE_MOCK_DFS_VISITOR, NULL);
    GList * visits_list;

    gil_dfs_visitor_traverse(GIL_DFS_VISITOR(mock_dfs_visitor), E);
    visits_list = gil_visitor_get_visits_list(GIL_VISITOR(mock_dfs_visitor));

    for(i = 0; i < g_list_length(visits_list); i++)
      {
        name = (gchar*)g_list_nth_data(visits_list, i);
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }

  {
    gint i;
    gchar * visit_names[] = { "A", "B", "C", "D" };
    GilMockDfsVisitor *mock_dfs_visitor = g_object_new(GIL_TYPE_MOCK_DFS_VISITOR, NULL);
    GList * visits_list;

    gil_dfs_visitor_traverse(GIL_DFS_VISITOR(mock_dfs_visitor), D);
    visits_list = gil_visitor_get_visits_list(GIL_VISITOR(mock_dfs_visitor));

    for(i = 0; i < g_list_length(visits_list); i++)
      {
        gchar *name = (gchar*)g_list_nth_data(visits_list, i);
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

/**
    O
    |\
    | P
    | |
    | Q
    |/
    R
    |
    S

**/
static void
test_dfs_visitor_traverse_diamond(Test *test)
{
  g_print("test_dump_visitor_traverse_diamond\n");
  {
    gint i;
    gchar * visit_names [] = {"S", "R", "Q", "P", "O"};
    GilMockDfsVisitor *mock_dfs_visitor = g_object_new(GIL_TYPE_MOCK_DFS_VISITOR, NULL);
    GList * visits_list;

    gil_dfs_visitor_traverse (GIL_DFS_VISITOR(mock_dfs_visitor), O);
    visits_list = gil_visitor_get_visits_list(GIL_VISITOR(mock_dfs_visitor));

    for(i = 0; i < g_list_length(visits_list); i++)
      {
        gchar *name = (gchar*)g_list_nth_data(visits_list, i);
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

static void
dfs_visitor_setup(Test *test)
{

  /*
         E
        / \
       D   F
      /\    \
     B  C    G
     | /
     A

  */

  {
    A = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "A",
                      NULL);
    B = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "B",
                      "num_children", 1,
                      NULL);
    C = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "C",
                      "num_children", 1,
                      NULL);
    D = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "D",
                      "num_children", 2,
                      NULL);
    E = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "E",
                      "num_children", 2,
                      NULL);

    F = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "F",
                      "num_children", 1,
                      NULL);
    G = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "G",
                      NULL);


    gil_node_set_nth_child(B, A, 0);
    gil_node_set_nth_child(C, A, 0);
    gil_node_set_nth_child(D, B, 0);
    gil_node_set_nth_child(D, C, 1);
    gil_node_set_nth_child(E, D, 0);
    gil_node_set_nth_child(E, F, 1);
    gil_node_set_nth_child(F, G, 0);

  }

/**
    O
    |\
    | P
    | |
    | Q
    |/
    R
    |
    S

**/
  {
    O = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "O",
                      "num_children", 2,
                      NULL);

    P = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "P",
                      "num_children", 1,
                      NULL);

    Q = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "Q",
                      "num_children", 1,
                      NULL);

    R = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "R",
                      "num_children", 1,
                      NULL);

    S = g_object_new (GIL_TYPE_MOCK_NODE,
                      "name", "S",
                      NULL);

    gil_node_set_nth_child(O, R, 0);
    gil_node_set_nth_child(O, P, 1);

    gil_node_set_nth_child(P, Q, 0);
    gil_node_set_nth_child(Q, R, 0);
    gil_node_set_nth_child(R, S, 0);
  }
}

static void
dfs_visitor_teardown(Test *test)
{
  g_object_unref(A);
  g_object_unref(B);
  g_object_unref(C);
  g_object_unref(D);
  g_object_unref(E);
  g_object_unref(F);
  g_object_unref(G);

  g_object_unref(O);
  g_object_unref(P);
  g_object_unref(Q);
  g_object_unref(R);
  g_object_unref(S);
}

Test *
create_dfs_visitor_test()
{
  Test *t = ct_create("GilDfsVisitorTest");

  g_assert(ct_addSetUp(t, dfs_visitor_setup));
  g_assert(ct_addTearDown(t, dfs_visitor_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_dfs_visitor_g_object_new));
  g_assert(ct_addTestFun(t, test_dfs_visitor_traverse));
  g_assert(ct_addTestFun(t, test_dfs_visitor_traverse_diamond));
#endif


  return t;
}
