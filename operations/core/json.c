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
} JsonOp;

typedef struct
{
  GeglOperationMetaJsonClass parent_class;
} JsonOpClass;


// FIXME: needed?
/*
GType                                                                   
json_op_get_type (void)                                             
{                                                                     
    return json_op_type_id;                                         
}                                                                     
*/

static void
json_op_class_init (gpointer klass, gpointer class_data)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

    // TODO: store the class data, so it can be accessed

/*
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = gegl_chant_constructor;
*/

/* TODO: go over exported ports and create properties for them

#define gegl_chant_int(name, nick, min, max, def, blurb)                     \
  g_object_class_install_property (object_class, PROP_##name,                \
                                   g_param_spec_int (#name, nick, blurb,     \
                                                     min, max, def,          \
                                                     (GParamFlags) (         \
                                                     G_PARAM_READWRITE |     \
                                                     G_PARAM_CONSTRUCT |     \
                                                     GEGL_PARAM_PAD_INPUT)));
*/
}

static void
json_op_class_finalize (JsonOpClass *self)
{
}     


static void
json_op_init (JsonOp *self)
{

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
    // TODO: parse json, pass to register_type
    gpointer class_data = NULL;
    // FIXME: unhardoce name, look up in json structure
    return json_op_register_type(type_module, "dropshadow_json", class_data);
}

/*
static void
graph_load_json(Graph *self, JsonParser *parser) {

    JsonNode *rootnode = json_parser_get_root(parser);
    g_assert(JSON_NODE_HOLDS_OBJECT(rootnode));
    JsonObject *root = json_node_get_object(rootnode);

    // Processes
    JsonObject *processes = json_object_get_object_member(root, "processes");
    g_assert(processes);

    GList *process_names = json_object_get_members(processes);
    for (int i=0; i<g_list_length(process_names); i++) {
        const gchar *name = g_list_nth_data(process_names, i);
        JsonObject *proc = json_object_get_object_member(processes, name);
        const gchar *component = json_object_get_string_member(proc, "component");
        graph_add_node(self, name, component);
    }

    //g_free(process_names); crashes??

    // Connections
    JsonArray *connections = json_object_get_array_member(root, "connections");
    g_assert(connections);
    for (int i=0; i<json_array_get_length(connections); i++) {
        JsonObject *conn = json_array_get_object_element(connections, i);

        JsonObject *tgt = json_object_get_object_member(conn, "tgt");
        const gchar *tgt_proc = json_object_get_string_member(tgt, "process");
        const gchar *tgt_port = json_object_get_string_member(tgt, "port");

        JsonNode *srcnode = json_object_get_member(conn, "src");
        if (srcnode) {
            // Connection
            JsonObject *src = json_object_get_object_member(conn, "src");
            const gchar *src_proc = json_object_get_string_member(src, "process");
            const gchar *src_port = json_object_get_string_member(src, "port");

            graph_add_edge(self, src_proc, src_port, tgt_proc, tgt_port);
        } else {
            // IIP
            JsonNode *datanode = json_object_get_member(conn, "data");
            GValue value = G_VALUE_INIT;
            g_assert(JSON_NODE_HOLDS_VALUE(datanode));
            json_node_get_value(datanode, &value);

            graph_add_iip(self, tgt_proc, tgt_port, &value);
            g_value_unset(&value);
        }
    }

    // Exported ports
    if (json_object_has_member(root, "inports")) {
        JsonObject *inports = json_object_get_object_member(root, "inports");
        GList *inport_names = json_object_get_members(inports);
        for (int i=0; i<g_list_length(inport_names); i++) {
            const gchar *name = g_list_nth_data(inport_names, i);
            JsonObject *conn = json_object_get_object_member(inports, name);
            const gchar *proc = json_object_get_string_member(conn, "process");
            const gchar *port = json_object_get_string_member(conn, "port");
            graph_add_port(self, GraphInPort, name, proc, port);
        }
    }

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
}
*/

/* JSON operation enumeration */
#define JSON_OP_DIR "/home/jon/contrib/code/imgflo-server/runtime/dependencies/gegl/operations/json"
static void
json_register_operations(GTypeModule *module)
{
    // FIXME: unhardcode, follow GEGL_PATH properly
    json_op_register_type_for_file (module, JSON_OP_DIR "/dropshadow.json");
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
