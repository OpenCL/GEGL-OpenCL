#include "gil-visitor.h"
#include "gil-node.h"

static void class_init (GilVisitorClass * klass);
static void init (GilVisitor * self, GilVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_node(GilVisitor * self, GilNode *node);
static void node_info_value_destroy(gpointer data);

static gpointer parent_class = NULL;

GType
gil_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (G_TYPE_OBJECT, 
                                     "GilVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GilVisitorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->visit_node = visit_node; 
}

static void 
init (GilVisitor * self, 
      GilVisitorClass * klass)
{
  self->visits_list = NULL;
  self->visits_objects_list = NULL;
  self->nodes_hash = g_hash_table_new_full(g_direct_hash,
                                           g_direct_equal,
                                           NULL,
                                           node_info_value_destroy); 
}

static void
finalize(GObject *gobject)
{  
  GilVisitor * self = GIL_VISITOR(gobject);

  g_list_free(self->visits_list);
  g_list_free(self->visits_objects_list);
  g_hash_table_destroy(self->nodes_hash);
     
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

GilNodeInfo *
gil_visitor_node_lookup(GilVisitor * self,
                         GilNode *node)
{
  GilNodeInfo * node_info;
  g_return_val_if_fail(self, NULL);
  g_return_val_if_fail(node, NULL);

  node_info = g_hash_table_lookup(self->nodes_hash, node);

  return node_info;
}

void
gil_visitor_node_insert(GilVisitor *self,
                         GilNode *node)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);

  if(!node_info)
    {
      node_info = g_new(GilNodeInfo, 1);
      node_info->visited = FALSE;
      node_info->discovered = FALSE;
      node_info->shared_count = 0;
      g_hash_table_insert(self->nodes_hash, node, node_info);
    }
  else
    {
      g_warning("Node already in nodes hash table");
    }
}

gboolean
gil_visitor_get_visited(GilVisitor *self,
                         GilNode *node)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->visited;
}

void
gil_visitor_set_visited(GilVisitor *self,
                         GilNode *node,
                         gboolean visited)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->visited = visited;
}

gboolean
gil_visitor_get_discovered(GilVisitor *self,
                                 GilNode *node)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->discovered;
}

void
gil_visitor_set_discovered(GilVisitor *self,
                            GilNode *node,
                            gboolean discovered)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->discovered = discovered;
}

gint
gil_visitor_get_shared_count(GilVisitor *self,
                              GilNode *node)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->shared_count;
}

void
gil_visitor_set_shared_count(GilVisitor *self,
                              GilNode *node,
                              gint shared_count)
{
  GilNodeInfo * node_info = gil_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->shared_count = shared_count;
}

GList *
gil_visitor_get_visits_list(GilVisitor *self)
{
  return self->visits_list;
}

GList *
gil_visitor_get_visits_objects_list(GilVisitor *self)
{
  return self->visits_objects_list;
}

static void
node_info_value_destroy(gpointer data)
{
  GilNodeInfo *node_info = (GilNodeInfo *)data;
  g_free(node_info);
}


void      
gil_visitor_visit_node(GilVisitor * self,
                        GilNode *node)
{
  GilVisitorClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GIL_IS_NODE(node));

  klass = GIL_VISITOR_GET_CLASS(self);

  if(klass->visit_node)
    (*klass->visit_node)(self, node);
}

static void      
visit_node(GilVisitor * self,
           GilNode *node)
{
 gchar* name = (gchar*)gil_node_get_name(node);
 self->visits_list = g_list_append(self->visits_list, name);
 self->visits_objects_list = g_list_append(self->visits_objects_list, node);
}
