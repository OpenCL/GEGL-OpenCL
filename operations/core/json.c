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

#include <json-glib/json-glib.h>
#include <gegl-plugin.h>

/* JsonOp: Meta-operation base class for ops defined by .json file */
#include <operation/gegl-operation-meta-json.h>
typedef struct _JsonOp
{
  GeglOperationMetaJson parent_instance;
  JsonObject *json_root;
  GHashTable *nodes;
} JsonOp;

typedef struct
{
  GeglOperationMetaJsonClass parent_class;
  JsonObject *json_root;
} JsonOpClass;


// FIXME: needed?
/*
GType                                                                   
json_op_get_type (void)                                             
{                                                                     
    return json_op_type_id;                                         
}                                                                     
*/

gchar *
component2geglop(const gchar *name) {
    gchar *dup = g_strdup(name);
    gchar *sep = g_strstr_len(dup, -1, "/");
    if (sep) {
        *sep = ':';
    }
    g_ascii_strdown(dup, -1);
    return dup;
}

static void
install_properties(JsonOpClass *json_op_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (json_op_class);
    JsonObject *root = json_op_class->json_root;

    guint prop = 1;

    g_print("%s: %p\n", __PRETTY_FUNCTION__, json_op_class->json_root);

    // Exported ports
    if (json_object_has_member(root, "inports")) {
        JsonObject *inports = json_object_get_object_member(root, "inports");
        GList *inport_names = json_object_get_members(inports);
        for (int i=0; i<g_list_length(inport_names); i++) {
            const gchar *name = g_list_nth_data(inport_names, i);
            JsonObject *conn = json_object_get_object_member(inports, name);
            const gchar *proc = json_object_get_string_member(conn, "process");
            const gchar *port = json_object_get_string_member(conn, "port");
            GParamSpec *spec = NULL;

            g_print("adding property %s, pointing to %s %s\n", name, port, proc);

            // TODO: look up property on the class/op the port points to and use that paramspec
            spec = g_param_spec_int (name, name, "DUMMY description", 0, 1000, 1,
                                        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT | GEGL_PARAM_PAD_INPUT));
            g_object_class_install_property (object_class, prop++, spec);
        }
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
  switch (property_id)
  {
    default:
//      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
  {
    default:
//      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
attach (GeglOperation *operation)
{
  JsonOp *self = (JsonOp *)operation;
  GeglNode  *gegl = operation->node;
  GeglNode  *input = gegl_node_get_input_proxy (gegl, "input");
  GeglNode  *output = gegl_node_get_output_proxy (gegl, "output");

  // Processes
  JsonObject *root = self->json_root;
  JsonObject *processes = json_object_get_object_member(root, "processes");

  GList *process_names = json_object_get_members(processes);
  for (int i=0; i<g_list_length(process_names); i++) {
      const gchar *name = g_list_nth_data(process_names, i);
      JsonObject *proc = json_object_get_object_member(processes, name);
      const gchar *component = json_object_get_string_member(proc, "component");
      gchar *opname = component2geglop(component);
      g_print("creating node %s with operation %s\n", name, opname);
      GeglNode *node = gegl_node_new_child (gegl, "operation", opname, NULL);
      gegl_operation_meta_watch_node (operation, node);
      g_hash_table_insert(self->nodes, (gpointer)g_strdup(name), (gpointer)node);
      g_free(opname);
  }
  g_list_free(process_names);

  // Connections
  JsonArray *connections = json_object_get_array_member(root, "connections");
  g_assert(connections);
  for (int i=0; i<json_array_get_length(connections); i++) {
      JsonObject *conn = json_array_get_object_element(connections, i);
      JsonObject *tgt = json_object_get_object_member(conn, "tgt");
      const gchar *tgt_proc = json_object_get_string_member(tgt, "process");
      const gchar *tgt_port = json_object_get_string_member(tgt, "port");
      GeglNode *tgt_node = g_hash_table_lookup(self->nodes, tgt_proc);
      g_assert(tgt_node);

      JsonNode *srcnode = json_object_get_member(conn, "src");
      if (srcnode) {
          // Connection
          JsonObject *src = json_object_get_object_member(conn, "src");
          const gchar *src_proc = json_object_get_string_member(src, "process");
          const gchar *src_port = json_object_get_string_member(src, "port");
          GeglNode *src_node = g_hash_table_lookup(self->nodes, src_proc);

          g_print("connecting %s,%s to %s,%s\n", src_proc, src_port, tgt_port, tgt_proc);
          g_assert(src_node);

          gegl_node_connect_to (src_node, src_port, tgt_node, tgt_port);
      } else {
          // IIP
          JsonNode *datanode = json_object_get_member(conn, "data");
          GValue value = G_VALUE_INIT;
          g_assert(JSON_NODE_HOLDS_VALUE(datanode));
          json_node_get_value(datanode, &value);
          // TODO: set property value
          g_value_unset(&value);
      }
  }

  // TODO: go over the exported ports and redirect them
  // gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-x");

}

static void
json_op_class_init (gpointer klass, gpointer class_data)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  JsonOpClass *json_op_class = (JsonOpClass *) (klass);
  json_op_class->json_root = (JsonObject *) (class_data);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = constructor;

  operation_class->attach = attach;

  install_properties(json_op_class);

  // FIXME: unharcode, look up in properties
  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:dropshadow2",
    "categories",  "effects:light",
    "description", "Creates a dropshadow effect on the input buffer",
    NULL);

}

static void
json_op_class_finalize (JsonOpClass *self)
{
}     


static void
json_op_init (JsonOp *self)
{
  self->nodes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref); // FIXME: free
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

    g_print("%s: %s\n", __PRETTY_FUNCTION__, filepath);

    if (success) {
        JsonNode *root_node = json_node_copy (json_parser_get_root (parser));
        JsonObject *root = json_node_get_object (root_node);
        g_assert(root_node);
        g_print("%s: %p\n", __PRETTY_FUNCTION__, root_node);
        // FIXME: unhardoce name, look up in json structure, fallback to basename
        ret = json_op_register_type(type_module, "dropshadow_json", root);
    }

//    g_object_unref(parser);
    return ret;
}

/* JSON operation enumeration */
#define JSON_OP_DIR "/home/jon/contrib/code/imgflo-server/runtime/dependencies/gegl/operations/json"
static void
json_register_operations(GTypeModule *module)
{
    // FIXME: unhardcode, follow GEGL_PATH properly
    json_op_register_type_for_file (module, JSON_OP_DIR "/dropshadow2.json");
}


/*** Module registration ***/
static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION
};

/* prototypes added to silence warnings from gcc for -Wmissing-prototypes*/
gboolean                gegl_module_register (GTypeModule *module);
const GeglModuleInfo  * gegl_module_query    (GTypeModule *module);

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  json_register_operations (module);

  return TRUE;
}
