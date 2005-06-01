#include "config.h"

#include "gil-node.h"
#include "gil-visitor.h"
#include <stdio.h>

enum
{
  PROP_0,
  PROP_NAME,
  PROP_LAST
};

static void class_init (GilNodeClass * klass);
static void init(GilNode *self, GilNodeClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void accept(GilNode * node, GilVisitor * visitor);

static void allocate_children(GilNode *self, gint num_children);
static void free_children(GilNode * self);

static gpointer parent_class = NULL;

GType
gil_node_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilNodeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilNode),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "GilNode",
                                     &typeInfo,
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void
class_init (GilNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->accept = accept;

  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The Node's name",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  return;
}

static void
init (GilNode * self,
      GilNodeClass * klass)
{
  self->children = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  GilNode *self = GIL_NODE (gobject);

  free_children(self);

  if(self->name)
   g_free(self->name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GilNode * node = GIL_NODE(gobject);
  switch (prop_id)
  {
    case PROP_NAME:
      gil_node_set_name(node, g_value_get_string(value));
      break;
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GilNode * node = GIL_NODE(gobject);
  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string(value, gil_node_get_name(node));
      break;
    default:
      break;
  }
}

void
gil_node_set_name (GilNode * self,
                   const gchar * name)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_NODE (self));

  self->name = g_strdup(name);
}

G_CONST_RETURN gchar*
gil_node_get_name (GilNode * self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GIL_IS_NODE (self), NULL);

  return self->name;
}

GilNode *
gil_node_get_nth_child (GilNode * self,
                        gint n)
{
  GList *llink = NULL;
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < g_list_length(self->children));

  llink = g_list_nth(self->children, n);
  return (GilNode*)llink->data;
}

void
gil_node_set_nth_child(GilNode * self,
                       GilNode * child,
                       gint n)
{
  GList *llink = NULL;
  GilNode * old_child;
  g_return_if_fail(self != NULL);
  g_return_if_fail(n >= 0 && n < g_list_length(self->children));

  llink = g_list_nth(self->children, n);
  old_child = (GilNode*)llink->data;

  if(old_child)
    g_object_unref(old_child);

  if(child)
    g_object_ref(child);

  llink->data = child;
}

gint
gil_node_get_num_children (GilNode * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GIL_IS_NODE (self), -1);

  return g_list_length(self->children);
}

void
gil_node_set_num_children (GilNode * self,
                           gint num_children)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_NODE (self));

  allocate_children(self, num_children);
}

static void
allocate_children(GilNode *self,
                gint num_children)
{
  gint i;

  for(i = 0; i < num_children; i++)
    gil_node_append_child(self, NULL);
}

static void
free_children(GilNode * self)
{
  gint i;
  gint length = g_list_length(self->children);

  for(i = 0; i < length; i++)
    {
      GilNode * node = (GilNode*)g_list_nth_data(self->children, i);
      if(node)
        g_object_unref(node);
    }

  g_list_free(self->children);
}

void
gil_node_append_child(GilNode *self,
                      GilNode *child)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_NODE (self));

  self->children  = g_list_append(self->children, child);

  if(child)
    g_object_ref(child);
}

void
gil_node_accept(GilNode * self,
                 GilVisitor * visitor)
{
  GilNodeClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_NODE (self));
  g_return_if_fail (visitor != NULL);
  g_return_if_fail (GIL_IS_VISITOR(visitor));

  klass = GIL_NODE_GET_CLASS(self);

  if(klass->accept)
    (*klass->accept)(self, visitor);
}

static void
accept (GilNode * node,
        GilVisitor * visitor)
{
  gil_visitor_visit_node(visitor, node);
}
