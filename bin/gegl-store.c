/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */


#if 1
#include "gegl-plugin.h"  /* is this neccesary? FIXME: should just be gegl.h
                           
                           gegl_node_get_connected_to
                           gegl_node_get_operation
                           GeglPad
                           gegl_node_get_pad
                           gegl_pad_get_connections
                           GeglConnection
                           gegl_connection_get_sink_pad
                           gegl_pad_get_name
                           gegl_connection_get_sink_node
                           */
#endif
#if 0
#include "gegl.h"
#endif

#include "gegl-store.h"
#include <string.h>

GeglNode *gegl_children (GeglNode *item)
{
  return gegl_node_get_connected_to (item, "aux");
}

GeglNode *gegl_next_sibling (GeglNode *item)
{
  if (!strcmp (gegl_node_get_operation (item), "clone"))
    return NULL;
  return gegl_node_get_connected_to (item, "input");
}

GeglNode *gegl_previous_sibling (GeglNode *item)
{
  GeglPad *pad;
  if (!item)
    return NULL;

  
  pad = gegl_node_get_pad (item, "output");
  if (!pad)
    return NULL;
    {
      GList *pads = gegl_pad_get_connections (pad);
      if (pads)
        {
          GeglConnection *connection = pads->data;
          GeglPad *pad = gegl_connection_get_sink_pad (connection);
          if (!strcmp (gegl_pad_get_name (pad), "input"))
            return gegl_connection_get_sink_node (connection);
        }
    }
  return NULL;
}

GeglNode *gegl_parent (GeglNode *item)
{
  GeglPad *pad;
  GeglNode *iter = item;

  if (!item)
    return NULL;
  while (gegl_previous_sibling (iter))
    iter = gegl_previous_sibling (iter);

  pad = gegl_node_get_pad (iter, "output");
  
  if (!pad)
    return NULL;
  iter = NULL;
    {
      GList *pads = gegl_pad_get_connections (pad);
      if (pads)
        {
          GeglConnection *connection = pads->data;
          GeglPad *pad = gegl_connection_get_sink_pad (connection);
          if (!strcmp (gegl_pad_get_name (pad), "aux"))
            iter = gegl_connection_get_sink_node (connection);
        }
    }
  return iter;
}

/****/


static void gegl_store_init (GeglStore *pkg_tree);

static void gegl_store_class_init (GeglStoreClass *klass);

static void gegl_store_tree_model_init (GtkTreeModelIface *iface);

static void gegl_store_finalize (GObject * object);

static GtkTreeModelFlags gegl_store_get_flags (GtkTreeModel *tree_model);

static gint gegl_store_get_n_columns (GtkTreeModel *tree_model);

static GType gegl_store_get_column_type (GtkTreeModel *tree_model,
                                          gint index);

static gboolean gegl_store_get_iter (GtkTreeModel *tree_model,
                                      GtkTreeIter *iter, GtkTreePath *path);

static GtkTreePath *gegl_store_get_path (GtkTreeModel *tree_model,
                                          GtkTreeIter *iter);

static void gegl_store_get_value (GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gint column, GValue * value);

static gboolean gegl_store_iter_next (GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);

static gboolean gegl_store_iter_children (GtkTreeModel *tree_model,
                                           GtkTreeIter *iter,
                                           GtkTreeIter *parent);

static gboolean gegl_store_iter_has_child (GtkTreeModel *tree_model,
                                            GtkTreeIter *iter);

static gint gegl_store_iter_n_children (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);

static gboolean gegl_store_iter_nth_child (GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *parent, gint n);

static gboolean gegl_store_iter_parent (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *child);


static GObjectClass *parent_class = NULL;

GType
gegl_store_get_type (void)
{
  static GType gegl_store_type = 0;

  if (gegl_store_type)
    return gegl_store_type;

  {
    static const GTypeInfo gegl_store_info = {
      sizeof (GeglStoreClass),
      NULL,                   /* base_init */
      NULL,                   /* base_finalize */

      (GClassInitFunc) gegl_store_class_init,        /* class_init */
      NULL,                   /* class_finalize */
      NULL,                   /* class_data */

      sizeof (GeglStore),    /* instance size */
      0,                      /* n_preallocs */
      (GInstanceInitFunc) gegl_store_init,   /* instance_init */
      NULL                    /* value_table */
    };
    static const GInterfaceInfo tree_model_info = {
      (GInterfaceInitFunc) gegl_store_tree_model_init,
      NULL,
      NULL
    };
    gegl_store_type = g_type_register_static (G_TYPE_OBJECT, "GeglStore",
                                               &gegl_store_info,
                                               (GTypeFlags) 0);
    g_type_add_interface_static (gegl_store_type, GTK_TYPE_TREE_MODEL,
                                 &tree_model_info);
  }

  return gegl_store_type;
}

static void
gegl_store_class_init (GeglStoreClass *klass)
{
  GObjectClass *object_class;

  parent_class = (GObjectClass *) g_type_class_peek_parent (klass);
  object_class = (GObjectClass *) klass;

  object_class->finalize = gegl_store_finalize;
}

static void
gegl_store_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = gegl_store_get_flags;
  iface->get_n_columns = gegl_store_get_n_columns;
  iface->get_column_type = gegl_store_get_column_type;
  iface->get_iter = gegl_store_get_iter;
  iface->get_path = gegl_store_get_path;
  iface->get_value = gegl_store_get_value;
  iface->iter_next = gegl_store_iter_next;
  iface->iter_children = gegl_store_iter_children;
  iface->iter_has_child = gegl_store_iter_has_child;
  iface->iter_n_children = gegl_store_iter_n_children;
  iface->iter_nth_child = gegl_store_iter_nth_child;
  iface->iter_parent = gegl_store_iter_parent;
}

static void
gegl_store_init (GeglStore *gegl_store)
{
  gegl_store->n_columns = GEGL_STORE_N_COLUMNS;

  gegl_store->column_types[GEGL_STORE_COL_ITEM] = G_TYPE_POINTER;
  gegl_store->column_types[GEGL_STORE_COL_TYPE] = G_TYPE_UINT;
  gegl_store->column_types[GEGL_STORE_COL_TYPESTR] = G_TYPE_STRING;
  gegl_store->column_types[GEGL_STORE_COL_ICON] = G_TYPE_STRING;
  gegl_store->column_types[GEGL_STORE_COL_NAME] = G_TYPE_STRING;
  gegl_store->column_types[GEGL_STORE_COL_SUMMARY] = G_TYPE_STRING;

  gegl_store->stamp = g_random_int ();
}

static void
gegl_store_finalize (GObject * object)
{
/*  GeglStore *gegl_store = GEGL_STORE(object); */
  (*parent_class->finalize) (object);
}

static    GtkTreeModelFlags
gegl_store_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (IS_GEGL_STORE (tree_model), (GtkTreeModelFlags) 0);

  return (GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
gegl_store_get_n_columns (GtkTreeModel *tree_model)
{
  return GEGL_STORE_N_COLUMNS;
}


static    GType
gegl_store_get_column_type (GtkTreeModel *tree_model, gint index)
{
  g_return_val_if_fail (IS_GEGL_STORE (tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index < GEGL_STORE (tree_model)->n_columns
                        && index >= 0, G_TYPE_INVALID);

  return GEGL_STORE (tree_model)->column_types[index];
}

static inline gboolean
is_visible (GeglNode *item)
{
  return TRUE;
}


static gboolean
gegl_store_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter *iter, GtkTreePath *path)
{
  GeglStore *gegl_store;
  GeglNode  *item;
  gint      *indices, n, depth;

  g_assert (IS_GEGL_STORE (tree_model));
  g_assert (path != NULL);

  gegl_store = GEGL_STORE (tree_model);

  indices = gtk_tree_path_get_indices (path);
  depth = gtk_tree_path_get_depth (path);

  item = gegl_store->root;

  for (n = 0; n < depth && item; n++)
    {
      int    sibling_no = indices[n];
      if(n!=0)
        item = gegl_children (item);

      while (item && sibling_no)
        {
          sibling_no--;
          item = gegl_next_sibling (item);
        }

      if (!item)
        {
          return FALSE;
        }
    }

  iter->stamp = gegl_store->stamp;
  iter->user_data = item;

  return TRUE;
}

static int
prev_siblings (GeglNode *item)
{
  int       cnt = 0;
  while (item)
    {
      item = gegl_previous_sibling (item);
      if (item)
        cnt++;
    }
  return cnt;
}

#include <stdio.h>

static GtkTreePath *
gegl_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GtkTreePath *path;
  GeglStore   *gegl_store;
  GeglNode    *gegl_item;
  g_return_val_if_fail (IS_GEGL_STORE (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->user_data != NULL, NULL);

  gegl_store = GEGL_STORE (tree_model);

  gegl_item = iter->user_data;

  path = gtk_tree_path_new ();

  while (gegl_item)
    {
      gtk_tree_path_prepend_index (path, prev_siblings (gegl_item));
      gegl_item = gegl_parent (gegl_item);
    }
  return path;
}

#include <string.h>

static void
gegl_store_get_value (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter,
                      gint          column,
                      GValue       *value)
{
  GeglStore *gegl_store;
  GeglNode  *item;

  g_return_if_fail (IS_GEGL_STORE (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (column < GEGL_STORE (tree_model)->n_columns);

  g_value_init (value, GEGL_STORE (tree_model)->column_types[column]);

  gegl_store = GEGL_STORE (tree_model);

  item = iter->user_data;
  if (!item)
    return;

  switch (column)
    {
    case GEGL_STORE_COL_NAME:
      {
        char *name;
        gegl_node_get (item, "name", &name, NULL);

        if (name && name[0])
          {
            g_value_set_string (value, name);
            g_free (name);
          }
        else
          {
            char *type;
            gegl_node_get (item, "operation", &type, NULL);
            g_value_set_string (value, type);
          }
      }
      break;
    case GEGL_STORE_COL_TYPESTR:
        {
          char *type;
          gegl_node_get (item, "operation", &type, NULL);
          g_value_set_string (value, type);
        }
      break;
    case GEGL_STORE_COL_TYPE:
      /*g_value_set_uint (value, gegl_type (item));*/
      g_value_set_uint (value, 42);
      break;
    case GEGL_STORE_COL_ITEM:
      g_value_set_pointer (value, item);
      break;
#if 0
    case GEGL_STORE_COL_ICON:
      switch (0)/*gegl_type (item))*/
        {
        case GeglGraph:
          g_value_set_string (value, STOCK_GRAPH);
          break;
        case GeglClip:
          g_value_set_string (value, STOCK_CLIP);
          break;
        case GeglSequence:
          g_value_set_string (value, STOCK_VIDEO);
          break;
        case GeglOp:
          {
            char     *type = NULL;
            int       input_pads, output_pads;

            type = gegl_get_property (item, "type");
/* *INDENT-OFF* */

#define if_str_eq(a,b) if(!strcmp ((a),(b)))

        if (type)
          {
        if_str_eq (type, "apply_mask")
        {
          g_value_set_string (value, STOCK_APPLY_MASK);
          return;
        }
        else
        if_str_eq (type, "avi")
        {
          g_value_set_string (value, STOCK_AVI);
          return;
        }
        else
        if_str_eq (type, "bcontrast")
        {
          g_value_set_string (value, STOCK_BCONTRAST);
          return;
        }
        else
        if_str_eq (type, "blank")
        {
          g_value_set_string (value, STOCK_BLANK);
          return;
        }
        else
        if_str_eq (type, "boxblur")
        {
          g_value_set_string (value, STOCK_BLUR);
          return;
        }
        else
        if_str_eq (type, "clone")
        {
          g_value_set_string (value, STOCK_CLONE);
          return;
        }
        else
        if_str_eq (type, "colorbalance")
        {
          g_value_set_string (value, STOCK_COLORBALANCE);
          return;
        }
        else
        if_str_eq (type, "curve")
        {
          g_value_set_string (value, STOCK_CURVE);
          return;
        }
        else
        if_str_eq (type, "display")
        {
          g_value_set_string (value, STOCK_DISPLAY);
          return;
        }
        else
        if_str_eq (type, "hflip")
        {
          g_value_set_string (value, STOCK_HFLIP);
          return;
        }
        else
        if_str_eq (type, "histogram")
        {
          g_value_set_string (value, STOCK_HISTOGRAM);
          return;
        }
        else
        if_str_eq (type, "invert")
        {
          g_value_set_string (value, STOCK_INVERT);
          return;
        }
        else
        if_str_eq (type, "luminance")
        {
          g_value_set_string (value, STOCK_LUMINANCE);
          return;
        }
        else
        if_str_eq (type, "noise")
        {
          g_value_set_string (value, STOCK_NOISE);
          return;
        }
        else
        if_str_eq (type, "opacity")
        {
          g_value_set_string (value, STOCK_OPACITY);
          return;
        }
        else
        if_str_eq (type, "over")
        {
          g_value_set_string (value, STOCK_OVER);
          return;
        }
        else
        if_str_eq (type, "png")
        {
          g_value_set_string (value, STOCK_PNG);
          return;
        }
        else
        if_str_eq (type, "scale")
        {
          g_value_set_string (value, STOCK_SCALE);
          return;
        }
        else
        if_str_eq (type, "text")
        {
          g_value_set_string (value, STOCK_TEXT);
          return;
        }
        else
        if_str_eq (type, "transform")
        {
          g_value_set_string (value, STOCK_TRANSFORM);
          return;
        }
        else
        if_str_eq (type, "threshold")
        {
          g_value_set_string (value, STOCK_THRESHOLD);
          return;
        }
        free (type);
          }

/* *INDENT-ON* */
#undef if_str_eq

            input_pads = gegl_get_property_int (item, "_input_pads");
            output_pads = gegl_get_property_int (item, "_output_pads");

            if (!input_pads)
              {
                g_value_set_string (value, STOCK_SOURCE);
              }
            else if (!output_pads)
              {
                g_value_set_string (value, STOCK_SINK);
              }
            else
              {
                g_value_set_string (value, STOCK_FILTER);
              }
          }
          break;
        default:
          g_value_set_string (value, STOCK_UNKNOWN);
        }
#endif
      g_value_set_string (value, "fnord");
      break;
    case GEGL_STORE_COL_SUMMARY:
#if 0
      if (gegl_properties (item))
        {
          GString  *str = g_string_new ("");
          Gegl    *property = gegl_properties (item);

          while (property)
            {
              int       surpress = 0;
              char     *prop_name = gegl_get_property (property, "name");
              char     *prop_value = gegl_get_property (item, prop_name);

              if (prop_name[0] == '_')
                {
                  surpress = 1;
                }
              else if (prop_value
                       && gegl_get_default_property (item, prop_name)
                       &&
                       !strcmp (gegl_get_default_property (item, prop_name),
                                prop_value))
                {
                  surpress = 1;
                }

              if (!surpress)
                {
                  g_string_append (str, prop_name);
                  g_string_append (str, "='");
                  g_string_append (str, prop_value);
                  g_string_append (str, "' ");
                }
              property = gegl_next_sibling (property);
              if (prop_name)
                free (prop_name);
              if (prop_value)
                free (prop_value);
            }
          g_value_set_string (value, str->str);
          g_string_free (str, TRUE);

        }
      else
#endif
        {
          g_value_set_string (value, "fnord");
        }
      break;
    }
}

static gboolean
gegl_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GeglStore *gegl_store;
  GeglNode  *item;


  g_return_val_if_fail (IS_GEGL_STORE (tree_model), FALSE);
  gegl_store = GEGL_STORE (tree_model);

  if (!iter)
    return FALSE;

  item = GEGL_NODE (iter->user_data);

  g_assert (item);

  if (!item)
    return FALSE;

  item = gegl_next_sibling (item);

  if (!item)
    return FALSE;

  if (iter)
    {
      iter->stamp = gegl_store->stamp;
      iter->user_data = item;
    }

  return TRUE;
}

static gboolean
gegl_store_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter *iter, GtkTreeIter *parent)
{
  GeglStore *gegl_store;
  //g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);
  GeglNode  *item;

  g_return_val_if_fail (IS_GEGL_STORE (tree_model), FALSE);
  gegl_store = GEGL_STORE (tree_model);

  if (!parent)
    {
      if (iter)
        {
          iter->stamp = gegl_store->stamp;
          iter->user_data = gegl_store->root;
          return TRUE;
        }
      return TRUE;
    }

  item = GEGL_NODE (parent->user_data);

  if (!item)
    return FALSE;

  if (parent)
    item = gegl_children (item);

  while (item)
    {
      if (iter)
        {
          iter->stamp = gegl_store->stamp;
          iter->user_data = item;
        }
      return TRUE;
      item = gegl_next_sibling (item);
    }

  return FALSE;
}

static gint
gegl_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GeglNode  *item;
  gint       count = 0;

  if (!iter)
    {                           /* top level indicated */
      GeglStore *gegl_store;

      gegl_store = GEGL_STORE (tree_model);
      item = gegl_store->root;
      while (item)
        {
          count++;
          item = gegl_next_sibling (item);
        }
      return count;
    }
  else
    {
      item = GEGL_NODE (iter->user_data);
    }

  if (!item)
    return 0;

  item = gegl_children (item);
  while (item)
    {
      count++;
      item = gegl_next_sibling (item);
    }

  return count;
}

static gboolean
gegl_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  return gegl_store_iter_children (tree_model, NULL, iter);
}


static gboolean
gegl_store_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
  GeglStore *gegl_store;
  GeglNode  *item = NULL;
  gegl_store = GEGL_STORE (tree_model);

  if (!parent)
    {                           /* top level */
      item = gegl_store->root;
    }
  else
    {
      item = GEGL_NODE (parent->user_data);
    }



  if (item && gegl_children (item))
    {
      if(parent)
        item = gegl_children (item);
      while (item && n)
        {
          if (is_visible (item))
            n--;
          item = gegl_next_sibling (item);
        }
    }
  else
    {
      item = NULL;
    }

  if (item)
    {
      if (iter)
        {
          iter->user_data = item;
          iter->stamp = gegl_store->stamp;
        }
      return TRUE;
    }
  return FALSE;
}

static gboolean
gegl_store_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter *iter, GtkTreeIter *child)
{
  GeglNode    *item;
  GeglStore *gegl_store;

  gegl_store = GEGL_STORE (tree_model);

  if (!child)
    return FALSE;

  item = child->user_data;

  if (!item)
    return FALSE;

  if (gegl_parent (item))
    {
      if (iter)
        {
          iter->user_data = gegl_parent (item);
          iter->stamp = gegl_store->stamp;
        }
      return TRUE;
    }

  return FALSE;
}


GeglStore *
gegl_store_new (void)
{
  GeglStore *gegl_store;

  gegl_store = (GeglStore *) g_object_new (TYPE_GEGL_STORE, NULL);

  g_assert (gegl_store != NULL);

  return gegl_store;
}

void
gegl_store_set_gegl (GeglStore *gegl_store,
                     GeglNode  *gegl)
{
  g_return_if_fail (IS_GEGL_STORE (gegl_store));
  gegl_store->gegl = gegl;
  gegl_store->root = gegl_graph_output (gegl, "output");
}
