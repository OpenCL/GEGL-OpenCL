/* rgegl - a ruby binding for GEGL
 *
 * rgegl is free software; you can redistribute it and/or
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
 * License along with rgegl; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * 2007 © Øyvind Kolås, Kevin Cozens.
 */

#include "config.h"
#include "rgegl.h"
#include "babl/babl.h"

#define _SELF(self) GEGL_NODE(RVAL2GOBJ(self))

/*FIXME: these are not in the public GEGL API, they should considered
 * for promotion to public, or be deprecated in other ways from the
 * binding
 */
void gegl_node_insert_before (GeglNode *, GeglNode *);

static VALUE
cnode_initialize(self)
    VALUE self;
{
    G_INITIALIZE(self, gegl_node_new());
    return Qnil;
}

static VALUE
cnode_create_child (self, operation)
  VALUE self;
  VALUE operation;
{
    GeglNode *child;

    child = gegl_node_create_child (_SELF (self), RVAL2CSTR (operation));

    return GOBJ2RVAL(G_OBJECT(child));
}

static VALUE
cnode_get_producer (self, pad_name)
  VALUE self, pad_name;
{
    GeglNode *node;
    gchar *padname=NULL;

    node = gegl_node_get_producer (_SELF (self), RVAL2CSTR (pad_name), &padname);
    if (node)
      {
        VALUE     ary;
        ary = rb_ary_new();
        rb_ary_push (ary, GOBJ2RVAL(G_OBJECT(node)));
        rb_ary_push (ary, rb_str_new2(padname));
        g_free(padname);
        return ary;
      }
    return Qnil;
}

static VALUE
cnode_get_consumers (self, pad_name)
  VALUE self, pad_name;
{
    GeglNode    **nodes;
    const gchar **pads;
    gint          count;
    VALUE         ret;
    gint          i;

    count = gegl_node_get_consumers (_SELF (self), RVAL2CSTR(pad_name), &nodes, &pads);
    if (!count)
      return Qnil;

    ret = rb_ary_new();
    for (i=0;i<count;i++)
      {
        GeglNode    *node = nodes[i];
        const gchar *pad = pads[i];
        VALUE     ary;
        ary = rb_ary_new();
        rb_ary_push (ary, GOBJ2RVAL(G_OBJECT(node)));
        rb_ary_push (ary, rb_str_new2(pad));
        rb_ary_push (ret, ary);
      }
    if (nodes)
      g_free (nodes);
    if (pads)
      g_free (pads);
    return ret;
}

static VALUE
cnode_connect_to (self, output_pad, target_node, target_pad)
  VALUE self, output_pad, target_node, target_pad;
{
    gegl_node_connect_to (_SELF (self),
                          RVAL2CSTR (output_pad),
                          RVAL2GOBJ (target_node),
                          RVAL2CSTR (target_pad));

    return self;
}

static VALUE
cnode_link (self, target_node)
  VALUE self, target_node;
{
    gegl_node_link (_SELF (self), RVAL2GOBJ (target_node));
    return target_node;
}


static VALUE
cnode_insert_before (self, to_be_inserted)
  VALUE self, to_be_inserted;
{
    gegl_node_insert_before (_SELF (self), RVAL2GOBJ (to_be_inserted));
    return Qnil;
}


static VALUE
cnode_render (self, r_rectangle, r_scale, r_format, r_flags)
  VALUE self, r_rectangle, r_scale, r_format, r_flags;
{
    VALUE    rbuf;
    gpointer buf;
    long     buflen;

    GeglRectangle *rectangle;
    gdouble        scale;
    Babl          *format;
    GeglBlitFlags  flags;

    format = babl_format (RVAL2CSTR (r_format));
    if (!format)
      {
        return Qnil;
      }
    rectangle = RVAL2BOXED (r_rectangle, GEGL_TYPE_RECTANGLE);
    scale = NUM2DBL (r_scale);
    flags = NUM2INT (r_flags);

    buflen = rectangle->width * rectangle->height *
             babl_format_get_bytes_per_pixel (format);

    rbuf = rb_str_new (NULL, buflen);
    buf = RSTRING(rbuf)->ptr;

    gegl_node_blit (_SELF(self), scale, rectangle, format, buf, 0, flags);
    return rbuf;
}

static VALUE
cnode_get_property (self, property_name)
  VALUE self, property_name;
{
    const char *name;

    GValue gval = {0,};
    GParamSpec *prop;

    if (SYMBOL_P(property_name)) {
      name = rb_id2name(SYM2ID(property_name));
    } else {
      StringValue(property_name);
      name = StringValuePtr(property_name);
    }

    prop = gegl_node_find_property (_SELF (self), name);
    if (!prop){
      return Qnil;
    }
    g_value_init(&gval, G_PARAM_SPEC_VALUE_TYPE (prop));
    gegl_node_get_property (_SELF (self), name, &gval);

    return GVAL2RVAL(&gval);
}

static VALUE
cnode_find_property (self, property_name)
  VALUE self, property_name;
{
    GParamSpec *prop;
    VALUE result;
    const char *name;

    if (SYMBOL_P(property_name)) {
      name = rb_id2name(SYM2ID(property_name));
    } else {
      StringValue(property_name);
      name = StringValuePtr(property_name);
    }

    prop = gegl_node_find_property (_SELF (self), name);
    if (!prop){
      return Qnil;
    }
    result = GOBJ2RVAL(prop);
    return result;
}

static VALUE
clist_properties (self, op_type)
  VALUE self, op_type;
{
  guint n_properties;
  GParamSpec **props;
  VALUE ary;
  int i;

  props = gegl_list_properties(RVAL2CSTR(op_type), &n_properties);
  ary = rb_ary_new();
  for (i = 0; i < n_properties; i++){
      rb_ary_push (ary, rbgobj_ruby_object_from_instance(props[i])); /*rb_str_new2(props[i]->name));*/
  }
  g_free (props);
  return ary;
}

static VALUE
clist_operations (self)
  VALUE self;
{
  guint n_operations;
  gchar **operations;
  VALUE ary;
  int i;

  operations = gegl_list_operations (&n_operations);
  ary = rb_ary_new();
  for (i = 0; i < n_operations; i++){
      rb_ary_push (ary, rb_str_new2(operations[i]));
  }
  g_free (operations);
  return ary;
}

static VALUE
cnode_get_children (self)
  VALUE self;
{
  GSList *children;
  VALUE ary;

  ary = rb_ary_new();
  for (children = gegl_node_get_children (_SELF (self));
       children;
       children=children->next)
    rb_ary_push (ary, GOBJ2RVAL(G_OBJECT(children->data)));
  return ary;
}

static VALUE
cnode_get_output_proxy (self, name)
  VALUE self, name;
{
  VALUE result;
  GeglNode *proxy;
  proxy = gegl_node_get_output_proxy (_SELF (self), RVAL2CSTR (name));
  result = GOBJ2RVAL(proxy);
  return result;
}

static VALUE
cnode_get_input_proxy (self, name)
  VALUE self, name;
{
  VALUE result;
  GeglNode *proxy;
  proxy = gegl_node_get_input_proxy (_SELF (self), RVAL2CSTR (name));
  result = GOBJ2RVAL(proxy);
  return result;
}

static VALUE
cnode_set_property (self, property_name, value)
  VALUE self, property_name, value;
{
    GValue gval = {0,};
    g_value_init(&gval, RVAL2GTYPE (value));
    rbgobj_rvalue_to_gvalue (value, &gval);
      {
    const char *name;

    if (SYMBOL_P(property_name)) {
      name = rb_id2name(SYM2ID(property_name));
    } else {
      StringValue(property_name);
      name = StringValuePtr(property_name);
    }

    gegl_node_set_property (_SELF (self), name, &gval);
      }
    return self;
}

static VALUE
cnode_get_bounding_box (self)
    VALUE self;
{
    GeglRectangle rect;

    rect = gegl_node_get_bounding_box (_SELF (self));

    return BOXED2RVAL(&rect, GEGL_TYPE_RECTANGLE);
}

static VALUE
cnode_process (self)
  VALUE self;
{
    gegl_node_process (_SELF (self));
    return self;
}

static VALUE
cnode_to_xml (self,
              path_root)
  VALUE self, path_root;
{
    gchar *xml = gegl_node_to_xml (_SELF (self), RVAL2CSTR(path_root));
    return CSTR2RVAL (xml);
}

gchar *gegl_to_dot (GeglNode *self);

static VALUE
cnode_to_dot (self)
  VALUE self;
{
  gchar *dot = gegl_to_dot (_SELF (self));
  return CSTR2RVAL (dot);
}

static VALUE
cnode_detect (self,
              x,
              y)
  VALUE self, x, y;
{
  GeglNode *node;
  VALUE result;
  node = gegl_node_detect (_SELF (self), NUM2INT (x), NUM2INT (y));
  result = GOBJ2RVAL(node);
  return result;
}

static VALUE
c_gegl_init (self)
  VALUE self;
{
  /*
  static gboolean inited=FALSE;
  if (!inited)
    {
      gegl_init (NULL, NULL);
      inited = TRUE;
    }
  */
  return Qnil;
}

static VALUE
c_gegl_exit (self)
  VALUE self;
{
  gegl_exit ();
  return Qnil;
}

static VALUE
c_gegl_parse_xml(self, str, path_root)
    VALUE self, str, path_root;
{
    GeglNode *root;
    root = gegl_node_new_from_xml (RVAL2CSTR (str), RVAL2CSTR (path_root));
    return GOBJ2RVAL(root);
}

static VALUE
cnode_new_processor(self, r_rectangle)
    VALUE self, r_rectangle;
{
    GeglProcessor *processor;
    GeglRectangle *rectangle;
    if (r_rectangle!=Qnil)
      {
        rectangle = RVAL2BOXED (r_rectangle, GEGL_TYPE_RECTANGLE);
        processor = gegl_node_new_processor (_SELF (self), rectangle);
      }
    else
      {
        processor = gegl_node_new_processor (_SELF (self), NULL);
      }
    return GOBJ2RVAL(processor);
}

void
Init_gegl_node(mGegl)
    VALUE mGegl;
{
    VALUE geglGeglNode = G_DEF_CLASS(GEGL_TYPE_NODE, "Node", mGegl);

    rb_define_method(geglGeglNode, "initialize", cnode_initialize, 0);
    rb_define_method(geglGeglNode, "render", cnode_render, 4);
    rb_define_method(geglGeglNode, "bounding_box", cnode_get_bounding_box, 0);
    rb_define_method(geglGeglNode, "create_child", cnode_create_child, 1);
    rb_define_method(geglGeglNode, "connect_to", cnode_connect_to, 3);
    rb_define_method(geglGeglNode, "producer", cnode_get_producer, 1);
    /*rb_define_method(geglGeglNode, "consumer", cnode_get_consumer, 1);*/
    rb_define_method(geglGeglNode, "consumers", cnode_get_consumers, 1);
    rb_define_method(geglGeglNode, "insert_before", cnode_insert_before, 1);
    rb_define_method(geglGeglNode, "children", cnode_get_children, 0);
    rb_define_method(geglGeglNode, "detect", cnode_detect, 2);
    rb_define_method(geglGeglNode, "dot", cnode_to_dot, 0);
    rb_define_method(geglGeglNode, "get_property", cnode_get_property, 1);
    rb_define_method(geglGeglNode, "input_proxy", cnode_get_input_proxy, 1);
    rb_define_method(geglGeglNode, "link", cnode_link, 1);
    rb_define_method(geglGeglNode, "output_proxy", cnode_get_output_proxy, 1);
    rb_define_method(geglGeglNode, "process", cnode_process, 0);
    rb_define_method(geglGeglNode, "property", cnode_find_property, 1);
    rb_define_method(geglGeglNode, "set_property", cnode_set_property, 2);
    rb_define_method(geglGeglNode, "processor", cnode_new_processor, 1);
    rb_define_method(geglGeglNode, "xml", cnode_to_xml, 1);
/*    rb_define_method(geglGeglNode, "save_cache", cnode_save_cache, 1);
    rb_define_method(geglGeglNode, "load_cache", cnode_load_cache, 1);*/

    rb_define_module_function (mGegl, "init", c_gegl_init, 0);
    rb_define_module_function (mGegl, "exit", c_gegl_exit, 0);
    rb_define_module_function (mGegl, "properties", clist_properties, 1);
    rb_define_module_function (mGegl, "operations", clist_operations, 0);
    rb_define_module_function (mGegl, "parse_xml", c_gegl_parse_xml, 2);
}
