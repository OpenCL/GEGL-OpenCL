/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Jon Nordby <jononor@gmail.com>
 */

/*
GeglOperationMetaJson : GeglOperationMeta
    Base class for all ops, paramatrizable to the specific json data
    Keeps the parsed json structure in memory as class_data
    on class_init() registers GOBject properties for exposed ports
    on attach() instantiates the nodes in subgraph, and connects them

GeglModuleJson : 
    on register(), walks directories in GEGL_PATH
    for each .json file found, registers

Internal operations
    operations/json/something.json
Installed to, and loaded at runtime from
    $(GEGL_PATH)/myop.json

dropshadow a good initial testcase?
*/

//#define GEGL_OP_NAME json

#include <json-glib/json-glib.h>
#include <gegl-plugin.h>

// For module paths
#include <gegl-init-private.h>
#include <gegldatafiles.h>
/* For forcing module to be persistent */
#include <geglmodule.h>

/* JsonOp: Meta-operation base class for ops defined by .json file */
#include <operation/gegl-operation-meta-json.h>
typedef struct _JsonOp
{
  GeglOperationMetaJson parent_instance;
  JsonObject *json_root;
  GHashTable *nodes; // gchar* -> GeglNode *, owned by parent node
} JsonOp;

typedef struct
{
  GeglOperationMetaJsonClass parent_class;
  JsonObject *json_root;
  GHashTable *properties; // guint property_id -> PropertyTarget
} JsonOpClass;

typedef struct
{
  gchar *node;
  gchar *port;
} PropertyTarget;

static PropertyTarget *
property_target_new(gchar *node, gchar *port)
{
    PropertyTarget *self = g_new(PropertyTarget, 1);
    self->node = node;
    self->port = port;
    return self;
}

static void
property_target_free(PropertyTarget *self)
{
    g_free(self->node);
    g_free(self->port);
    g_free(self);
}

static gchar *
replace_char_inline(gchar *str, gchar from, gchar to) {
    for (int i=0; i<strlen(str); i++) {
        str[i] = (str[i] == from) ? to : str[i];
    }
    return str;
}

static gchar *
component2geglop(const gchar *name) {
    gchar *dup;
    if (!name) {
      return NULL;
    }
    dup = g_ascii_strdown(name, -1);
    replace_char_inline(dup, '/', ':');
    return dup;
}

static gchar *
component2gtypename(const gchar *name) {
    gchar *dup;
    if (!name) {
      return NULL;
    }
    dup = g_ascii_strdown(name, -1);
    replace_char_inline(dup, '/', '_');
    return dup;
}

static gboolean
gvalue_from_string(GValue *value, GType target_type, GValue *dest_value) {
    const gchar *iip;
    /* Custom conversion from string */
    if (!G_VALUE_HOLDS_STRING(value)) {
        return FALSE;
    }
    iip = g_value_get_string(value);
    if (g_type_is_a(target_type, G_TYPE_DOUBLE)) {
        gdouble d = g_ascii_strtod(iip, NULL);
        g_value_set_double(dest_value, d);
    } else if (g_type_is_a(target_type, G_TYPE_INT)) {
        gint i = g_ascii_strtoll(iip, NULL, 10);
        g_value_set_int(dest_value, i);
    } else if (g_type_is_a(target_type, G_TYPE_INT64)) {
        gint64 i = g_ascii_strtoll(iip, NULL, 10);
        g_value_set_int64(dest_value, i);
    } else if (g_type_is_a(target_type, GEGL_TYPE_COLOR)) {
        GeglColor *color = gegl_color_new(iip);
        if (!color || !GEGL_IS_COLOR(color)) {
            return FALSE;
        }
        g_value_set_object(dest_value, color);
    } else if (g_type_is_a(target_type, G_TYPE_ENUM)) {
        GEnumClass *klass = (GEnumClass *)g_type_class_ref(target_type);
        GEnumValue *val = g_enum_get_value_by_nick(klass, iip);
        g_return_val_if_fail(val, FALSE);

        g_value_set_enum(dest_value, val->value);
        g_type_class_unref((gpointer)klass);
    } else if (g_type_is_a(target_type, G_TYPE_BOOLEAN)) {
        gboolean b = g_ascii_strcasecmp("true", iip) == 0;
        g_value_set_boolean(dest_value, b);
    } else {
        return FALSE;
    }
    return TRUE;
}

static gboolean
set_prop(GeglNode *t, const gchar *port, GParamSpec *paramspec, GValue *value)  {
    GType target_type = G_PARAM_SPEC_VALUE_TYPE(paramspec);
    GValue dest_value = {0,};
    gboolean success;

    g_value_init(&dest_value, target_type);

    success = g_param_value_convert(paramspec, value, &dest_value, FALSE);
    if (success) {
        gegl_node_set_property(t, port, &dest_value);
        return TRUE;
    }

    if (gvalue_from_string(value, target_type, &dest_value)) {
        g_param_value_validate(paramspec, &dest_value);
        gegl_node_set_property(t, port, &dest_value);
        return TRUE;
    }
    return FALSE;
}

static GParamSpec *
copy_param_spec(GParamSpec *in, const gchar *name) {

  const gchar * blurb = g_param_spec_get_blurb(in);
  GParamSpec *out = NULL;

  GParamFlags flags = G_PARAM_READWRITE;

  // TODO: handle more things
  if (G_IS_PARAM_SPEC_FLOAT(in)) {
    GParamSpecFloat *f = G_PARAM_SPEC_FLOAT(in);
    out = g_param_spec_double(name, name, blurb, f->minimum, f->maximum, f->default_value, flags);
  } else if (G_IS_PARAM_SPEC_DOUBLE(in)) {
    GParamSpecDouble *d = G_PARAM_SPEC_DOUBLE(in);
    out = g_param_spec_double(name, name, blurb, d->minimum, d->maximum, d->default_value, flags);
  } else if (G_IS_PARAM_SPEC_INT(in)) {
    GParamSpecInt *i = G_PARAM_SPEC_INT(in);
    out = g_param_spec_int(name, name, blurb, i->minimum, i->maximum, i->default_value, flags);
  } else if (G_IS_PARAM_SPEC_UINT(in)) {
    GParamSpecUInt *u = G_PARAM_SPEC_UINT(in);
    out = g_param_spec_int(name, name, blurb, u->minimum, u->maximum, u->default_value, flags);
  } else if (G_IS_PARAM_SPEC_LONG(in)) {
    GParamSpecLong *l = G_PARAM_SPEC_LONG(in);
    out = g_param_spec_int(name, name, blurb, l->minimum, l->maximum, l->default_value, flags);
  } else if (GEGL_IS_PARAM_SPEC_COLOR(in)) {
    GeglColor *default_value = gegl_param_spec_color_get_default(in);
    out = gegl_param_spec_color(name, name, blurb, default_value, flags);
  } else {
    g_critical("json: Unknown param spec type for property %s", g_param_spec_get_nick(in));
  }
  return out;
}

static guint
install_properties(JsonOpClass *json_op_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (json_op_class);
    JsonObject *root = json_op_class->json_root;
    guint prop = 1;

    // Exported ports
    if (json_object_has_member(root, "inports")) {
        JsonObject *inports = json_object_get_object_member(root, "inports");
        GList *inport_names = json_object_get_members(inports);
        GList *l;
        for (l = inport_names; l != NULL; l = l->next) {
            const gchar *name = l->data;
            JsonObject *conn = json_object_get_object_member(inports, name);
            const gchar *proc = json_object_get_string_member(conn, "process");
            const gchar *port = json_object_get_string_member(conn, "port");
            JsonObject *processes = json_object_get_object_member(root, "processes");
            JsonObject *p = json_object_get_object_member(processes, proc);
            const gchar *component = json_object_get_string_member(p, "component");

            {
              GParamSpec *target_spec = NULL;
              gchar *opname = component2geglop(component);
              // HACK: should avoid instantiating node to determine prop
              GeglNode *n = gegl_node_new();
              g_assert(n);
              gegl_node_set(n, "operation", opname, NULL);
              target_spec = gegl_node_find_property(n, port);
              if (target_spec) {
                GParamSpec *spec = copy_param_spec(target_spec, name);
                PropertyTarget *t = property_target_new(g_strdup(proc), g_strdup(port));
                g_hash_table_insert(json_op_class->properties, GINT_TO_POINTER(prop), t);
                g_object_class_install_property (object_class, prop, spec);
                prop++;
              }
              g_object_unref(n);
              g_free(opname);
            }
        }

        g_list_free(inport_names);
    }

/*
    if (json_object_has_member(root, "outports")) {
        JsonObject *outports = json_object_get_object_member(root, "outports");
        GList *outport_names = json_object_get_members(outports);
        for (int i=0; i<g_list_length(outport_names); i++) {
            const gchar *name = g_list_nth_data(outport_names, i);
            JsonObject *conn = json_object_get_object_member(outports, name);
            const gchar *proc = json_object_get_string_member(conn, "process");
            const gchar *port = json_object_get_string_member(conn, "port");
            graph_add_port(self, GraphOutPort, name, proc, port);
        }
    }
*/
  return prop-1;
}

static GObject *
constructor (GType                  type,
            guint                  n_construct_properties,
            GObjectConstructParam *construct_properties)
{
  gpointer klass = g_type_class_peek(GEGL_TYPE_OPERATION_META_JSON);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GObject *obj = gobject_class->constructor(type, n_construct_properties, construct_properties);
  JsonOpClass *json_op_class = G_TYPE_INSTANCE_GET_CLASS(obj, type, JsonOpClass);
  JsonOp *json_op = (JsonOp *)obj;
  json_op->json_root = json_op_class->json_root; // to avoid looking up via class/gtype
  return obj;
}

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  JsonOpClass * json_op_class = (JsonOpClass *)G_OBJECT_GET_CLASS(gobject);
  JsonOp * self = (JsonOp *)(gobject);
  GeglNode *node;

  PropertyTarget *target = g_hash_table_lookup(json_op_class->properties, GINT_TO_POINTER(property_id));
  if (!target) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
    return;
  }

  node = g_hash_table_lookup(self->nodes, target->node);
  if (!node) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
    return;
  }

  gegl_node_get_property(node, target->port, value);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  JsonOpClass *json_op_class = (JsonOpClass *)G_OBJECT_GET_CLASS(gobject);
  JsonOp *self = (JsonOp *) gobject;
  PropertyTarget *target;
  GeglNode *node;

  g_assert(self);

  target = g_hash_table_lookup(json_op_class->properties, GINT_TO_POINTER(property_id));
  if (!target) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
    return;
  }

  node = g_hash_table_lookup(self->nodes, target->node);
  if (!node) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
    return;
  }

  gegl_node_set_property(node, target->port, value);
}

static void
attach (GeglOperation *operation)
{
  JsonOp *self = (JsonOp *)operation;
  GeglNode  *gegl = operation->node;
  JsonArray *connections;
  GList *l;

  // Processes
  JsonObject *root = self->json_root;
  JsonObject *processes = json_object_get_object_member(root, "processes");

  GList *process_names = json_object_get_members(processes);
  for (l = process_names; l != NULL; l = l->next) {
      const gchar *name = l->data;
      JsonObject *proc = json_object_get_object_member(processes, name);
      const gchar *component = json_object_get_string_member(proc, "component");
      gchar *opname = component2geglop(component);

      GeglNode *node = gegl_node_new_child (gegl, "operation", opname, NULL);
      gegl_operation_meta_watch_node (operation, node);
      g_assert(node);
      g_hash_table_insert(self->nodes, (gpointer)g_strdup(name), (gpointer)node);
      g_free(opname);
  }
  g_list_free(process_names);

  // Connections
  connections = json_object_get_array_member(root, "connections");
  g_assert(connections);
  for (int i=0; i<json_array_get_length(connections); i++) {
      JsonObject *conn = json_array_get_object_element(connections, i);
      JsonObject *tgt = json_object_get_object_member(conn, "tgt");
      const gchar *tgt_proc = json_object_get_string_member(tgt, "process");
      const gchar *tgt_port = json_object_get_string_member(tgt, "port");
      GeglNode *tgt_node = g_hash_table_lookup(self->nodes, tgt_proc);
      JsonNode *srcnode;

      g_assert(tgt_node);

      srcnode = json_object_get_member(conn, "src");
      if (srcnode) {
          // Connection
          JsonObject *src = json_object_get_object_member(conn, "src");
          const gchar *src_proc = json_object_get_string_member(src, "process");
          const gchar *src_port = json_object_get_string_member(src, "port");
          GeglNode *src_node = g_hash_table_lookup(self->nodes, src_proc);

          g_assert(src_node);

          gegl_node_connect_to (src_node, src_port, tgt_node, tgt_port);
      } else {
          // IIP
          JsonNode *datanode = json_object_get_member(conn, "data");
          GValue value = G_VALUE_INIT;
          GParamSpec *paramspec;

          g_assert(JSON_NODE_HOLDS_VALUE(datanode));
          json_node_get_value(datanode, &value);
          paramspec = gegl_node_find_property(tgt_node, tgt_port);

          set_prop(tgt_node, tgt_port, paramspec, &value);
          g_value_unset(&value);
      }
  }


  // Exported ports
  if (json_object_has_member(root, "inports")) {
      JsonObject *inports = json_object_get_object_member(root, "inports");
      GList *inport_names = json_object_get_members(inports);
      for (l = inport_names; l != NULL; l = l->next) {
          const gchar *name = l->data;
          JsonObject *conn = json_object_get_object_member(inports, name);
          const gchar *proc = json_object_get_string_member(conn, "process");
          const gchar *port = json_object_get_string_member(conn, "port");
          GeglNode *node = g_hash_table_lookup(self->nodes, proc);

          g_assert(node);

          if (g_strcmp0(name, "input") == 0) {
              GeglNode *input = gegl_node_get_input_proxy (gegl, "input");
              gegl_node_connect_to (input, "output", node, "input");
          } else {
            gegl_operation_meta_redirect (operation, name, node, port);
          }
      }

      g_list_free(inport_names);
  }

  if (json_object_has_member(root, "outports")) {
      JsonObject *outports = json_object_get_object_member(root, "outports");
      GList *outport_names = json_object_get_members(outports);
      for (l = outport_names; l != NULL; l = l->next) {
          const gchar *name = l->data;
          JsonObject *conn = json_object_get_object_member(outports, name);
          const gchar *proc = json_object_get_string_member(conn, "process");
          const gchar *port = json_object_get_string_member(conn, "port");
          GeglNode *node = g_hash_table_lookup(self->nodes, proc);
          g_assert(node);

          if (g_strcmp0(name, "output") == 0) {
            GeglNode *proxy = gegl_node_get_output_proxy (gegl, "output");
            gegl_node_connect_to (node, port, proxy, "input");
          } else {
            g_warning("Unsupported output '%s' exported in .json file", name);
          }
      }

      g_list_free(outport_names);
  }

}

static void
json_op_init (JsonOp *self)
{
  self->nodes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static void
finalize (GObject *gobject)
{
  JsonOp *self = (JsonOp *)(gobject);
//  JsonOpClass *json_op_class = (JsonOpClass *)G_OBJECT_GET_CLASS(gobject);

  g_hash_table_unref (self->nodes);

// FIXME: causes infinite loop GEGL_OPERATION_CLASS(json_op_class)->finalize(gobject);
}

/* json_op_class */
static const gchar *
metadata_get_property(JsonObject *root, const gchar *prop) {
  if (json_object_has_member(root, "properties")) {
      JsonObject *properties = json_object_get_object_member(root, "properties");
      if (json_object_has_member(properties, prop)) {
        return json_object_get_string_member(properties, prop);
      }
  }
  return NULL;
}

static void
json_op_class_init (gpointer klass, gpointer class_data)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  JsonOpClass *json_op_class = (JsonOpClass *) (klass);
  const gchar *description;
  gchar *name;

  json_op_class->json_root = (JsonObject *) (class_data);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = constructor;
  object_class->finalize = finalize;

  operation_class->attach = attach;

  json_op_class->properties = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                    NULL, (GDestroyNotify)property_target_free);
  install_properties(json_op_class);

  description = metadata_get_property(json_op_class->json_root, "description");
  name = component2geglop(metadata_get_property(json_op_class->json_root, "name"));

  gegl_operation_class_set_keys (operation_class,
    "name",        (name) ? name : g_strdup_printf("json:%s", G_OBJECT_CLASS_NAME(object_class)),
    "categories",  "meta:json",
    "description",  (description) ? description : "",
    NULL);

  g_free(name);
}

static void
json_op_class_finalize (JsonOpClass *self)
{
  g_hash_table_unref(self->properties);
}


static GType                                                             
json_op_register_type (GTypeModule *type_module, const gchar *name, gpointer klass_data)                    
{
    gint flags = 0;
    const GTypeInfo g_define_type_info =                                
    {
      sizeof (JsonOpClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) json_op_class_init,
      (GClassFinalizeFunc) json_op_class_finalize,
      klass_data,
      sizeof (JsonOp),
      0,      /* n_preallocs */
      (GInstanceInitFunc) json_op_init,
      NULL    /* value_table */
    };
                              
    return g_type_module_register_type (type_module, GEGL_TYPE_OPERATION_META_JSON, name,
                                        &g_define_type_info, (GTypeFlags) flags);
}


static GType
json_op_register_type_for_file (GTypeModule *type_module, const gchar *filepath)
{
    GType ret = 0;
    GError *error = NULL;
    JsonParser *parser = json_parser_new();
    const gboolean success = json_parser_load_from_file(parser, filepath, &error);

    if (success) {
        JsonNode *root_node = json_node_copy (json_parser_get_root (parser));
        JsonObject *root = json_node_get_object (root_node);
        const gchar *name;
        gchar *type_name;

        g_assert(root_node);

        name = metadata_get_property(root, "name");
        type_name = (name) ? component2gtypename(name) : component2gtypename(filepath);
        ret = json_op_register_type(type_module, type_name, root);
        g_free(type_name);
    }

//    g_object_unref(parser);
    return ret;
}

/* JSON operation enumeration */
static void
load_file(const GeglDatafileData *file_data, gpointer user_data)
{
    GTypeModule *module = (GTypeModule *)user_data;
    if (!g_str_has_suffix(file_data->filename, ".json")) {
        return;
    }

    json_op_register_type_for_file(module, file_data->filename);
}

static void
load_path(gchar *path, gpointer user_data)
{
    gegl_datafiles_read_directories (path, G_FILE_TEST_EXISTS, load_file, user_data);
}

static void
json_register_operations(GTypeModule *module)
{
  GSList *paths = gegl_get_default_module_paths();
  g_slist_foreach(paths, (GFunc)load_path, module);
  g_slist_free_full(paths, g_free);
}


#ifndef GEGL_OP_BUNDLE
/*** Module registration ***/
static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION
};
#endif

/* prototypes added to silence warnings from gcc for -Wmissing-prototypes*/
gboolean                gegl_module_register (GTypeModule *module);
const GeglModuleInfo  * gegl_module_query    (GTypeModule *module);


#ifdef GEGL_OP_BUNDLE

G_MODULE_EXPORT void
gegl_op_json_register_type (GTypeModule *module);

G_MODULE_EXPORT void
gegl_op_json_register_type (GTypeModule *module)
{
#else

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
#endif
  /* Ensure the module, or shared libs it pulls in is not unloaded
   * This because when GTypes are registered (like for json-glib),
   *  all referenced data must stay in memory until process exit */
  g_module_make_resident (GEGL_MODULE (module)->module);

  json_register_operations (module);
#ifndef GEGL_OP_BUNDLE
  return TRUE;
#endif
}

