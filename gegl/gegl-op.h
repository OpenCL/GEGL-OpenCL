/* gegl-chant contains incantations to that produce the boilerplate
 * needed to write GEGL operation plug-ins. It abstracts away inheritance
 * by giving a limited amount of base classes, and reduced creation of
 * a properties struct and registration of properties for that structure to
 * a minimum amount of code through use of the C preprocessor. You should
 * look at the operations implemented using chanting (they #include this file)
 * to see how it is used in practice.
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
 * 2006-2008 © Øyvind Kolås.
 */


#ifdef GEGL_OP_C_SOURCE
#define __GEGL_HEADER(x) #x
#define _GEGL_HEADER(x) __GEGL_HEADER(x)
#define GEGL_HEADER(x) _GEGL_HEADER(x)

#define GEGL_OP_C_FILE GEGL_HEADER(GEGL_OP_C_SOURCE)
#ifndef GEGL_OP_NO_SOURCE
#define GEGL_OP_C_FILE_SOURCE GEGL_HEADER(GEGL_OP_C_SOURCE.h)
#endif
#endif

#ifndef GEGL_OP_NAME
#define GEGL_OP_NAME op
#error "define GEGL_OP_NAME"

#endif

#ifndef GEGL_OP_C_FILE
#ifdef GEGL_CHANT_C_FILE
#error   GEGL_OP_C_FILE not defined, %s/GEGL_CHANT_C_FILE/GEGL_OP_C_FILE/
#endif
#error "GEGL_OP_C_FILE not defined"
#endif

/* XXX: */
#ifdef GEGL_CHANT_TYPE_POINT_RENDER
#error "GEGL_CHANT_TYPE_POINT_RENDER should be replaced with GEGL_OP_POINT_RENDER"
#endif

#ifdef GEGL_CHANT_PROPERTIES
#error "GEGL_CHANT_PROPERTIES should be replaced with GEGL_PROPERTIES"
#endif

#include <gegl-plugin.h>

G_BEGIN_DECLS

typedef struct _GeglProperties  GeglProperties;
typedef struct _GeglOp   GeglOp;


static void gegl_op_init_properties     (GeglOp   *self);
static void gegl_op_class_intern_init   (gpointer     klass);
static gpointer gegl_op_parent_class = NULL;

#define define_register_type_(a) \
void gegl_op_##a##_register_type     (GTypeModule *module);\
GType gegl_op_##a##_get_type (void);
#define define_register_type(a) define_register_type_(a)
define_register_type(GEGL_OP_NAME)
#undef define_register_type
#undef define_register_type_

#define GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED(C_FILE, TypeName, type_name, TYPE_PARENT, flags, CODE) \
static void     type_name##_init              (TypeName        *self);  \
static void     gegl_op_class_init            (TypeName##Class *klass); \
static void     type_name##_class_finalize    (TypeName##Class *klass); \
static GType    type_name##_type_id = 0;                                \
static void     type_name##_class_chant_intern_init (gpointer klass)    \
  {                                                                     \
    gegl_op_parent_class = g_type_class_peek_parent (klass);            \
    gegl_op_class_intern_init (klass);                                  \
    gegl_op_class_init ((TypeName##Class*) klass);                      \
  }                                                                     \
GType                                                            \
type_name##_get_type (void)                                             \
  {                                                                     \
    return type_name##_type_id;                                         \
  }                                                                     \
void                                                             \
type_name##_register_type (GTypeModule *type_module)                    \
  {                                                                     \
    gchar  tempname[256];                                               \
    gchar *p;                                                           \
    const GTypeInfo g_define_type_info =                                \
    {                                                                   \
      sizeof (TypeName##Class),                                         \
      (GBaseInitFunc) NULL,                                             \
      (GBaseFinalizeFunc) NULL,                                         \
      (GClassInitFunc) type_name##_class_chant_intern_init,             \
      (GClassFinalizeFunc) type_name##_class_finalize,                  \
      NULL,   /* class_data */                                          \
      sizeof (TypeName),                                                \
      0,      /* n_preallocs */                                         \
      (GInstanceInitFunc) type_name##_init,                             \
      NULL    /* value_table */                                         \
    };                                                                  \
    g_snprintf (tempname, sizeof (tempname),                            \
                "%s", #TypeName C_FILE);                                \
    for (p = tempname; *p; p++)                                         \
      if (*p=='.') *p='_';                                              \
                                                                        \
    type_name##_type_id = g_type_module_register_type (type_module,     \
                                                       TYPE_PARENT,     \
                                                       tempname,        \
                                                       &g_define_type_info, \
                                                       (GTypeFlags) flags); \
    { CODE ; }                                                          \
  }

#define GEGL_DEFINE_DYNAMIC_OPERATION_(T_P, C_FILE, opname) GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED(C_FILE, GeglOp, gegl_op_##opname, T_P, 0, {})
#define GEGL_DEFINE_DYNAMIC_OPERATION__(a,b,c) GEGL_DEFINE_DYNAMIC_OPERATION_(a,b,c)
#define GEGL_DEFINE_DYNAMIC_OPERATION(T_P) GEGL_DEFINE_DYNAMIC_OPERATION__(T_P, GEGL_OP_C_FILE, GEGL_OP_NAME)


#define GEGL_PROPERTIES(op) ((GeglProperties *) (((GeglOp*)(op))->properties))

#define MKCLASS(a)  MKCLASS2(a)
#define MKCLASS2(a) a##Class

#ifdef GEGL_OP_POINT_FILTER
#define GEGL_OP_Parent GeglOperationPointFilter
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_POINT_FILTER
#endif

#ifdef GEGL_OP_POINT_COMPOSER
#define GEGL_OP_Parent GeglOperationPointComposer
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_POINT_COMPOSER
#endif

#ifdef GEGL_OP_POINT_COMPOSER3
#define GEGL_OP_Parent GeglOperationPointComposer3
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_POINT_COMPOSER3
#endif

#ifdef GEGL_OP_POINT_RENDER
#define GEGL_OP_Parent GeglOperationPointRender
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_POINT_RENDER
#endif

#ifdef GEGL_OP_AREA_FILTER
#define GEGL_OP_Parent GeglOperationAreaFilter
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_AREA_FILTER
#endif

#ifdef GEGL_OP_FILTER
#define GEGL_OP_Parent GeglOperationFilter
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_FILTER
#endif

#ifdef GEGL_OP_TEMPORAL
#define GEGL_OP_Parent GeglOperationTemporal
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_TEMPORAL
#endif

#ifdef GEGL_OP_SOURCE
#define GEGL_OP_Parent GeglOperationSource
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_SOURCE
#endif

#ifdef GEGL_OP_SINK
#define GEGL_OP_Parent GeglOperationSink
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_SINK
#endif

#ifdef GEGL_OP_COMPOSER
#define GEGL_OP_Parent GeglOperationComposer
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_COMPOSER
#endif

#ifdef GEGL_OP_COMPOSER3
#define GEGL_OP_Parent GeglOperationComposer3
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_COMPOSER3
#endif

#ifdef GEGL_OP_META
#include <operation/gegl-operation-meta.h>
#define GEGL_OP_Parent GeglOperationMeta
#define GEGL_OP_PARENT GEGL_TYPE_OPERATION_META
#endif


#ifdef GEGL_OP_Parent
struct _GeglOp
{
  GEGL_OP_Parent parent_instance;
  gpointer       properties;
};

typedef struct
{
  MKCLASS(GEGL_OP_Parent)  parent_class;
} GeglOpClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_OP_PARENT)

#else
//#error  "no parent class specified"
#endif


#define GEGL_OP(obj)  ((GeglOp*)(obj))

/* if GEGL_OP_CUSTOM is defined you have to provide the following
 * code or your own implementation of it
 */
#ifndef GEGL_OP_CUSTOM

#define define_init_(a) \
  static void gegl_op_##a##_init (GeglOp *self) { gegl_op_init_properties (self); }\
  static void gegl_op_##a##_class_finalize (GeglOpClass *self) { }
#define define_init(a) define_init_(a)
define_init(GEGL_OP_NAME)
#undef define_init
#undef define_init_

static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION
};

/* prototypes added to silence warnings from gcc for -Wmissing-prototypes*/
gboolean                gegl_module_register (GTypeModule *module);
const GeglModuleInfo  * gegl_module_query    (GTypeModule *module);

#ifndef GEGL_OP_BUNDLE
G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
#define do_reg_(a) gegl_op_##a##_register_type (module)
#define do_reg(b) do_reg_(b)
  do_reg(GEGL_OP_NAME);
#undef do_reg
  return TRUE;
}
#endif
#endif


#define description(blurb)
#define value_range(min,max)
#define ui_range(min,max)
#define ui_gamma(gamma)
#define ui_steps(small_increment, big_increment)
#define ui_meta(key,val)
#define ui_digits(digits)

#define ITEM(name,label,def_val, type)
#define ITEM2(name,label,def_val,type) ITEM(name,label,def_val,type)

/* enum registration */

#define property_double(name, label, def_val)     ITEM(name,label,def_val,double)
#define property_int(name, label, def_val)        ITEM(name,label,def_val,int)
#define property_string(name, label, def_val)     ITEM(name,label,def_val,string)
#define property_file_path(name, label, def_val)  ITEM(name,label,def_val,string)
#define property_uri(name, label, def_val)        ITEM(name,label,def_val,string)
#define property_boolean(name, label, def_val)    ITEM(name,label,def_val,boolean)
#define property_pointer(name, label, def_val)    ITEM(name,label,def_val,pointer)
#define property_object(name, label, def_val)     ITEM2(name,label,def_val,object)
#define property_enum(name, label, enm, enum_name, def_val)  ITEM(name,label,def_val,pointer)
#define property_seed(name, label, rand_name)     ITEM(name,label,def_val,int)
#define property_format(name, label, def_val)     ITEM(name,label,def_val,pointer)
#define property_curve(name, label, def_val)      ITEM2(name,label,def_val,object)
#define property_path(name, label, def_val)       ITEM2(name,label,def_val,object)
#define property_color(name, label, def_val)      ITEM2(name,label,def_val,object)
#define property_audio_fragment(name, label, def_val)      ITEM2(name,label,NULL,object)

#define enum_start(enum_name)   typedef enum {
#define enum_value(value, nick, name)    value ,
#define enum_end(enum)          } enum ;

#include GEGL_OP_C_FILE

#undef enum_start
#undef enum_value
#undef enum_end

#ifdef GEGL_OP_C_FILE_SOURCE
#include GEGL_OP_C_FILE_SOURCE
#endif

#ifdef GETTEXT_PACKAGE
static const gchar *gegl_op_gettext_package G_GNUC_UNUSED = GETTEXT_PACKAGE;
#else
static const gchar *gegl_op_gettext_package G_GNUC_UNUSED = NULL;
#endif

#define enum_start(enum_name)          \
static GType enum_name ## _get_type (void) G_GNUC_CONST; \
static GType enum_name ## _get_type (void)               \
{                                                 \
  static GType etype = 0;                         \
  if (etype == 0) {                               \
    static GEnumValue values[] = {

#define enum_value(value, nick, name)                \
      { value, name, nick },

#define enum_end(enum)                                            \
      { 0, NULL, NULL }                                           \
    };                                                            \
    if (gegl_op_gettext_package)                                  \
      {                                                           \
        gint i;                                                   \
        for (i = 0; i < G_N_ELEMENTS (values); i++)               \
          if (values[i].value_name)                               \
            values[i].value_name =                                \
              dgettext (GETTEXT_PACKAGE, values[i].value_name);   \
      }                                                           \
    etype = g_enum_register_static (#enum, values);               \
  }                                                               \
  return etype;                                                   \
}

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_format
#undef property_enum
#undef property_seed
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path
#undef enum_start
#undef enum_value
#undef enum_end
#define enum_start(enum_name)
#define enum_value(value, nick, name)
#define enum_end(enum)

/* Properties */

struct _GeglProperties
{
  gpointer user_data; /* for use by the op implementation */
#define property_double(name, label, def_val)          gdouble     name;
#define property_int(name, label, def_val)             gint        name;
#define property_string(name, label, def_val)          gchar      *name;
#define property_boolean(name, label, def_val)         gboolean    name;
#define property_file_path(name, label, def_val)       gchar      *name;
#define property_uri(name, label, def_val)             gchar      *name;
#define property_object(name, label, def_val)          GObject    *name;
#define property_curve(name, label, def_val)           GeglCurve  *name;
#define property_color(name, label, def_val)           GeglColor  *name;
#define property_audio_fragment(name, label, def_val)  GeglAudioFragment  *name;
#define property_path(name, label, def_val)            GeglPath   *name; gulong path_changed_handler;
#define property_pointer(name, label, def_val)         gpointer    name;
#define property_format(name, label, def_val)          gpointer    name;
#define property_enum(name, label, enum, enum_name, def_val) enum        name;
#define property_seed(name, label, rand_name)          gint        name;\
                                                       GeglRandom *rand_name;

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_format
#undef property_enum
#undef property_seed
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path
};

#define GEGL_OP_OPERATION(obj) ((Operation*)(obj))

enum
{
  PROP_0,

#undef ITEM
#define ITEM(name,label,def_val, type)             PROP_##name,
#define ITEM2(name,label,def_val, type)            ITEM(name,label,def_val,type)

#define property_double(name, label, def_val)     ITEM(name,label,def_val,double)
#define property_int(name, label, def_val)        ITEM(name,label,def_val,int)
#define property_string(name, label, def_val)     ITEM(name,label,def_val,string)
#define property_file_path(name, label, def_val)  ITEM(name,label,def_val,string)
#define property_uri(name, label, def_val)        ITEM(name,label,def_val,string)
#define property_boolean(name, label, def_val)    ITEM(name,label,def_val,boolean)
#define property_object(name, label, def_val)     ITEM2(name,label,def_val,object)
#define property_curve(name, label, def_val)      ITEM2(name,label,def_val,object)
#define property_color(name, label, def_val)      ITEM2(name,label,def_val,object)
#define property_audio_fragment(name, label, def_val) ITEM2(name,label,def_val,object)
#define property_path(name, label, def_val)       ITEM2(name,label,def_val,object)
#define property_pointer(name, label, def_val)    ITEM(name,label,def_val,pointer)
#define property_format(name, label, def_val)     ITEM(name,label,def_val,pointer)
#define property_enum(name, label, enm, enum_name, def_val) ITEM(name,label,def_val,enum)
#define property_seed(name, label, rand_name)     ITEM(name,label,def_val,int)

#include GEGL_OP_C_FILE

#undef ITEM
#define ITEM(name,label,def_val, type) \
    case PROP_##name:                                         \
      g_value_set_##type (value, properties->name);           \
      break;
#define ITEM2(name,label,def_val, type)            ITEM(name,label,def_val,type)

  PROP_LAST
};

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglProperties *properties;

  properties = GEGL_PROPERTIES(gobject);

  switch (property_id)
  {

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_format
#undef property_enum
#undef property_seed
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
  if (properties) {}; /* silence gcc */
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglProperties *properties;

  properties = GEGL_PROPERTIES(gobject);

  switch (property_id)
  {
#define property_double(name, label, def_val)                         \
    case PROP_##name:                                                 \
      properties->name = g_value_get_double (value);                  \
      break;
#define property_int(name, label, def_val)                            \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      break;
#define property_string(name, label, def_val)                         \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define property_boolean(name, label, def_val)                        \
    case PROP_##name:                                                 \
      properties->name = g_value_get_boolean (value);                 \
      break;
#define property_file_path(name, label, def_val)                      \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define property_uri(name, label, def_val)                            \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define property_object(name, label, def_val)                         \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define property_curve(name, label, def_val)                          \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define property_color(name, label, def_val)                          \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define property_audio_fragment(name, label, def_val)                 \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define property_path(name, label, def_val)                           \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
        {                                                             \
          if (properties->path_changed_handler)                       \
            g_signal_handler_disconnect (G_OBJECT (properties->name), \
                                         properties->path_changed_handler); \
          properties->path_changed_handler = 0;                       \
          g_object_unref (properties->name);                          \
        }                                                             \
      properties->name = g_value_dup_object (value);                  \
      if (properties->name != NULL)                                   \
        {                                                             \
          properties->path_changed_handler =                          \
            g_signal_connect (G_OBJECT (properties->name), "changed", \
                              G_CALLBACK(path_changed), gobject);     \
         }                                                            \
      break;
#define property_pointer(name, label, def_val)                        \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define property_format(name, label, def_val)                         \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define property_enum(name, label, enum, enum_name, def_val)          \
    case PROP_##name:                                                 \
      properties->name = g_value_get_enum (value);                    \
      break;
#define property_seed(name, label, rand_name)                         \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      if (!properties->rand_name)                                     \
        properties->rand_name = gegl_random_new_with_seed (properties->name);\
      else                                                            \
        gegl_random_set_seed (properties->rand_name, properties->name);\
      break;

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path
#undef property_format
#undef property_enum
#undef property_seed

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }

  if (properties) {}; /* silence gcc */
}

static void gegl_op_destroy_notify (gpointer data)
{
  GeglProperties *properties = GEGL_PROPERTIES (data);

#define property_double(name, label, def_val)
#define property_int(name, label, def_val) 
#define property_string(name, label, ...)      \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define property_boolean(name, label, def_val)
#define property_file_path(name, label, def_val)    \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define property_uri(name, label, def_val)          \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define property_object(name, label, def_val)       \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define property_curve(name, label, def_val)       \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define property_color(name, label, def_val)       \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define property_audio_fragment(name, label, def_val) \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define property_path(name, label, def_val)         \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define property_pointer(name, label, ...)
#define property_format(name, label, ...)
#define property_enum(name, label, ...)
#define property_seed(name, label, ...)

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_format
#undef property_enum
#undef property_seed
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path

  g_slice_free (GeglProperties, properties);
}

static GObject *
gegl_op_constructor (GType                  type,
                     guint                  n_construct_properties,
                     GObjectConstructParam *construct_properties)
{
  GObject *obj;
  GeglProperties *properties;

  obj = G_OBJECT_CLASS (gegl_op_parent_class)->constructor (
            type, n_construct_properties, construct_properties);

  /* create dummy colors and vectors */
  properties = GEGL_PROPERTIES (obj);

#define property_double(name, label, def_val)
#define property_int(name, label, def_val)
#define property_string(name, label, def_val)
#define property_boolean(name, label, def_val)
#define property_file_path(name, label, def_val)
#define property_uri(name, label, def_val)
#define property_object(name, label, def_val)
#define property_pointer(name, label, def_val)
#define property_format(name, label, def_val)
#define property_curve(name, label, def_val)
#define property_audio_fragment(name, label, def_val)\
    if (properties->name == NULL)                   \
    {properties->name = gegl_audio_fragment_new(48000, 2, 0, 8192);}
#define property_color(name, label, def_val)\
    if (properties->name == NULL)                   \
    {properties->name = gegl_color_new(def_val?def_val:"black");}
#define property_path(name, label, def_val)
#define property_enum(name, ...)
#define property_seed(name, label, rand_name)    \
    if (properties->rand_name == NULL)              \
      {properties->rand_name = gegl_random_new_with_seed (0);}

#include GEGL_OP_C_FILE

#undef property_double
#undef property_int
#undef property_string
#undef property_boolean
#undef property_file_path
#undef property_uri
#undef property_object
#undef property_pointer
#undef property_format
#undef property_enum
#undef property_seed
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_path

  g_object_set_data_full (obj, "chant-data", obj, gegl_op_destroy_notify);
  properties ++; /* evil hack to silence gcc */
  return obj;
}

static gboolean has_key (GParamSpec *pspec,
                         const gchar *key,
                         const gchar *value)
{
  const char *set_value = gegl_param_spec_get_property_key (pspec, key);
  if (set_value && g_str_equal (value, set_value))
    return TRUE;
  return FALSE;
}

static void
param_spec_update_ui (GParamSpec *pspec,
                      gboolean    ui_range_set,
                      gboolean    ui_steps_set,
                      gboolean    ui_digits_set)
{
  if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      GeglParamSpecDouble *upspec = GEGL_PARAM_SPEC_DOUBLE (pspec);
      GParamSpecDouble    *vpspec = G_PARAM_SPEC_DOUBLE (pspec);

      if (!ui_steps_set)
        {
          if (!ui_range_set)
          {
            upspec->ui_maximum = vpspec->maximum;
            upspec->ui_minimum = vpspec->minimum;
          }

          if (has_key (pspec, "unit", "degree"))
            {
              upspec->ui_step_small = 1.0;
              upspec->ui_step_big = 15.0;
            }
          else if (upspec->ui_maximum <= 5.0)
            {
              upspec->ui_step_small = 0.001;
              upspec->ui_step_big   = 0.100;
            }
          else if (upspec->ui_maximum <= 50)
            {
              upspec->ui_step_small = 0.01;
              upspec->ui_step_big   = 1.00;
            }
          else if (upspec->ui_maximum <= 500)
            {
              upspec->ui_step_small = 1.0;
              upspec->ui_step_big   = 10.0;
            }
          else if (upspec->ui_maximum <= 5000)
            {
              upspec->ui_step_small = 1.0;
              upspec->ui_step_big   = 100.0;
            }
        }

      if (!ui_digits_set)
      {
        if (has_key (pspec, "unit", "degrees"))
        {
          upspec->ui_digits = 2;
        }
        else if (upspec->ui_maximum <= 5.0)
          {
            upspec->ui_digits = 4;
          }
        if (upspec->ui_maximum <= 50.0)
          {
            upspec->ui_digits = 3;
          }
        else if (upspec->ui_maximum <= 500)
          {
            upspec->ui_digits = 2;
          }
        else
          {
            upspec->ui_digits = 1;
          }
      }
    }
  else if (GEGL_IS_PARAM_SPEC_INT (pspec))
    {
      GeglParamSpecInt *upspec = GEGL_PARAM_SPEC_INT (pspec);
      GParamSpecInt    *vpspec = G_PARAM_SPEC_INT (pspec);

      if (!ui_steps_set)
        {
          if (!ui_range_set)
          {
            upspec->ui_maximum = vpspec->maximum;
            upspec->ui_minimum = vpspec->minimum;
          }

         if (upspec->ui_maximum <= 5)
           {
             upspec->ui_step_small = 1;
             upspec->ui_step_big   = 2;
           }
         else if (upspec->ui_maximum <= 50)
           {
             upspec->ui_step_small = 1;
             upspec->ui_step_big   = 5;
           }
         else if (upspec->ui_maximum <= 500)
           {
             upspec->ui_step_small = 1;
             upspec->ui_step_big   = 10;
           }
         else if (upspec->ui_maximum <= 5000)
           {
             upspec->ui_step_small = 1;
             upspec->ui_step_big   = 100;
           }
        }
    }
}

static void
gegl_op_class_intern_init (gpointer klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  int current_prop = -1;
  gboolean G_GNUC_UNUSED ui_range_set = FALSE;
  gboolean G_GNUC_UNUSED ui_steps_set = FALSE;
  gboolean G_GNUC_UNUSED ui_digits_set = FALSE;
  GParamFlags flags G_GNUC_UNUSED = (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | GEGL_PARAM_PAD_INPUT);

#ifdef GEGL_OP_C_FILE_SOURCE
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  gegl_operation_class_set_keys (operation_class,
    "source", op_c_source,
    NULL);
#endif

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = gegl_op_constructor;

  {
    GParamSpec *pspec = NULL;

#undef description
#undef value_range
#undef ui_range
#undef ui_steps
#undef ui_gamma
#undef ui_meta
#undef ui_digits

#define REGISTER_IF_ANY \
    if (pspec && current_prop >=0) {\
      param_spec_update_ui (pspec, ui_range_set, ui_steps_set, ui_digits_set);\
      g_object_class_install_property (object_class, current_prop, pspec);\
      pspec = NULL; current_prop = -1;\
      ui_range_set = ui_steps_set = ui_digits_set = FALSE;\
    }

#define description(blurb) \
    pspec->_blurb = g_strdup (blurb);
#define value_range(min,max) \
    vpspec->minimum = min; vpspec->maximum = max; \
    upspec->ui_minimum = min; upspec->ui_maximum = max;
#define ui_range(min,max) \
    upspec->ui_minimum = min; upspec->ui_maximum = max;\
    ui_range_set = TRUE;
#define ui_steps(step_small,step_big) \
    upspec->ui_step_small = step_small; upspec->ui_step_big = step_big; \
    ui_steps_set = TRUE;
#define ui_gamma(gamma) \
    upspec->ui_gamma = gamma;
#define ui_meta(key,val) \
    gegl_param_spec_set_property_key(pspec, key, val);
#define ui_digits(digits) \
    upspec->ui_digits = digits; \
    ui_digits_set = TRUE;

#define property_double(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_double (#name, label, NULL,-G_MAXDOUBLE,G_MAXDOUBLE,def_val,-100,100,1.0,flags);\
     GeglParamSpecDouble *upspec G_GNUC_UNUSED = GEGL_PARAM_SPEC_DOUBLE (pspec);\
     GParamSpecDouble    *vpspec G_GNUC_UNUSED = G_PARAM_SPEC_DOUBLE (pspec);\
     current_prop = PROP_##name ;

#define property_int(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_int (#name, label, NULL,G_MININT,G_MAXINT,def_val,-100,100,1.0,flags);\
     GeglParamSpecInt *upspec G_GNUC_UNUSED = GEGL_PARAM_SPEC_INT (pspec);\
     GParamSpecInt    *vpspec G_GNUC_UNUSED = G_PARAM_SPEC_INT (pspec);\
     current_prop = PROP_##name ;

#define property_string(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       g_param_spec_string (#name, label, NULL, def_val, flags);\
     current_prop = PROP_##name ;

#define property_boolean(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       g_param_spec_boolean (#name, label, NULL, def_val, flags);\
     current_prop = PROP_##name ;

#define property_file_path(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_file_path (#name, label, NULL, FALSE, FALSE, def_val, flags);\
     current_prop = PROP_##name ;

#define property_uri(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_uri (#name, label, NULL, FALSE, FALSE, def_val, flags);\
     current_prop = PROP_##name ;

#define property_object(name, label, type) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       g_param_spec_object (#name, label, NULL, type, flags);\
     current_prop = PROP_##name ;

#define property_curve(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{  GeglCurve *_gegl_op_default_curve = gegl_curve_new_default (); \
    GParamSpec *pspec = \
       gegl_param_spec_curve (#name, label, NULL, _gegl_op_default_curve, flags);\
     current_prop = PROP_##name ;\
     g_object_unref (_gegl_op_default_curve);\

#define property_color(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_color_from_string (#name, label, NULL, def_val, flags);\
     current_prop = PROP_##name ;

#define property_audio_fragment(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_audio_fragment (#name, label, NULL, flags);\
     current_prop = PROP_##name ;

#define property_path(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_path (#name, label, NULL, def_val, flags);\
     current_prop = PROP_##name ;

#define property_pointer(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       g_param_spec_pointer (#name, label, NULL, flags);\
     current_prop = PROP_##name ;

#define property_format(name, label, def_val) \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = \
       gegl_param_spec_format (#name, label, NULL, flags);\
     current_prop = PROP_##name ;

#define property_enum(name, label, enum, enum_name, def_val)                   \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = gegl_param_spec_enum (#name, label, NULL, enum_name ##_get_type(), def_val, flags);       \
    current_prop = PROP_##name ;

#define property_seed(name, label, rand_name)    \
    REGISTER_IF_ANY  \
  }{ GParamSpec *pspec = gegl_param_spec_seed (#name, label, NULL, flags);       \
    current_prop = PROP_##name ;

#include GEGL_OP_C_FILE

    REGISTER_IF_ANY
  }

#undef REGISTER_IF_ANY
#undef description
#undef value_range
#undef ui_range
#undef ui_steps
#undef ui_gamma
#undef ui_meta
#undef property_double
#undef property_int
#undef property_string
#undef property_enum
#undef property_seed
#undef property_pointer
#undef property_path
#undef property_curve
#undef property_color
#undef property_audio_fragment
#undef property_object
#undef property_format

}

static void
gegl_op_init_properties (GeglOp *self)
{
  self->properties = g_slice_new0 (GeglProperties);
}

G_END_DECLS
