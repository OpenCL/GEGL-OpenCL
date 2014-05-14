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

#ifndef GEGL_OP_C_FILE
#ifdef GEGL_CHANT_C_FILE
#error   GEGL_OP_C_FILE not defined, %s/GEGL_CHANT_C_FILE/GEGL_OP_C_FILE/
#endif
#error "GEGL_OP_C_FILE not defined"
#endif

#define gegl_chant_class_init   gegl_chant_class_init_SHOULD_BE_gegl_op_class_init 

/* XXX: */
#ifdef GEGL_CHANT_TYPE_POINT_RENDER
#error "GEGL_CHANT_TYPE_POINT_RENDER should be replaced with GEGL_OP_POINT_RENDER"
#endif

#ifdef GEGL_CHANT_PROPERTIES
#error "GEGL_CHANT_PROPERTIES should be replaced with GEGL_PROPERTIES"
#endif

#include <gegl-plugin.h>

G_BEGIN_DECLS

GType gegl_op_get_type (void);
typedef struct _GeglProperties  GeglProperties;
typedef struct _GeglOp   GeglOp;

static void gegl_op_register_type       (GTypeModule *module);
static void gegl_op_init_properties     (GeglOp   *self);
static void gegl_op_class_intern_init   (gpointer     klass);
static gpointer gegl_op_parent_class = NULL;

#define GEGL_DEFINE_DYNAMIC_OPERATION(T_P)  GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED (GEGL_OP_C_FILE, GeglOp, gegl_op, T_P, 0, {})
#define GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED(C_FILE, TypeName, type_name, TYPE_PARENT, flags, CODE) \
static void     type_name##_init              (TypeName        *self);  \
static void     type_name##_class_init        (TypeName##Class *klass); \
static void     type_name##_class_finalize    (TypeName##Class *klass); \
static GType    type_name##_type_id = 0;                                \
static void     type_name##_class_chant_intern_init (gpointer klass)    \
  {                                                                     \
    gegl_op_parent_class = g_type_class_peek_parent (klass);            \
    gegl_op_class_intern_init (klass);                                  \
    type_name##_class_init ((TypeName##Class*) klass);                  \
  }                                                                     \
GType                                                                   \
type_name##_get_type (void)                                             \
  {                                                                     \
    return type_name##_type_id;                                         \
  }                                                                     \
static void                                                             \
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
                "%s", #TypeName GEGL_OP_C_FILE);                        \
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
static void
gegl_op_init (GeglOp *self)
{
  gegl_op_init_properties (self);
}

static void
gegl_op_class_finalize (GeglOpClass *self)
{
}

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
  gegl_op_register_type (module);

  return TRUE;
}
#endif

/* enum registration */
#define gegl_property_double(name, ...)
#define gegl_property_string(name, ...)
#define gegl_property_file_path(name, ...)
#define gegl_property_int(name, ...)
#define gegl_property_boolean(name, ...)
#define gegl_property_enum(name, enum, enum_name, ...)
#define gegl_property_object(name, ...)
#define gegl_property_pointer(name, ...)
#define gegl_property_format(name, ...)
#define gegl_property_color(name, ...)
#define gegl_property_curve(name, ...)
#define gegl_property_seed(name, rand_name, ...)
#define gegl_property_path(name, ...)

#define gegl_enum_start(enum_name)   typedef enum {
#define gegl_enum_value(value, nick)    value ,
#define gegl_enum_end(enum)          } enum ;

#include GEGL_OP_C_FILE

#undef gegl_enum_start
#undef gegl_enum_value
#undef gegl_enum_end

#define gegl_enum_start(enum_name)          \
GType enum_name ## _get_type (void) G_GNUC_CONST; \
GType enum_name ## _get_type (void)               \
{                                                 \
  static GType etype = 0;                         \
  if (etype == 0) {                               \
    static const GEnumValue values[] = {

#define gegl_enum_value(value, nick) \
      { value, nick, nick },

#define gegl_enum_end(enum)             \
      { 0, NULL, NULL }                             \
    };                                              \
    etype = g_enum_register_static (#enum, values); \
  }                                                 \
  return etype;                                     \
}                                                   \

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path
#undef gegl_enum_start
#undef gegl_enum_value
#undef gegl_enum_end
#define gegl_enum_start(enum_name)
#define gegl_enum_value(value, nick)
#define gegl_enum_end(enum)

/* Properties */

struct _GeglProperties
{
  gpointer user_data; /* for use by the op implementation */
#define gegl_property_double(name, ...)                gdouble     name;
#define gegl_property_boolean(name, ...)               gboolean    name;
#define gegl_property_int(name, ...)                   gint        name;
#define gegl_property_string(name, ...)                gchar      *name;
#define gegl_property_file_path(name, ...)             gchar      *name;
#define gegl_property_enum(name, enum, enum_name, ...) enum        name;
#define gegl_property_object(name, ...)                GObject    *name;
#define gegl_property_pointer(name, ...)               gpointer    name;
#define gegl_property_format(name, ...)                gpointer    name;
#define gegl_property_color(name, ...)                 GeglColor  *name;
#define gegl_property_curve(name, ...)                 GeglCurve  *name;
#define gegl_property_seed(name, rand_name, ...)       gint        name;\
                                                       GeglRandom *rand_name;
#define gegl_property_path(name, ...)                  GeglPath   *name;\
                                                       gulong path_changed_handler;

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path
};

#define GEGL_OP_OPERATION(obj) ((Operation*)(obj))

enum
{
  PROP_0,

#define gegl_property_double(name, ...)     PROP_##name,
#define gegl_property_boolean(name, ...)    PROP_##name,
#define gegl_property_int(name, ...)        PROP_##name,
#define gegl_property_string(name, ...)     PROP_##name,
#define gegl_property_file_path(name, ...)  PROP_##name,
#define gegl_property_enum(name, enum, enum_name, ...)  PROP_##name,
#define gegl_property_object(name, ...)     PROP_##name,
#define gegl_property_pointer(name, ...)    PROP_##name,
#define gegl_property_format(name, ...)     PROP_##name,
#define gegl_property_color(name, ...)      PROP_##name,
#define gegl_property_curve(name, ...)      PROP_##name,
#define gegl_property_seed(name, rand_name, ...)   PROP_##name,
#define gegl_property_path(name, ...)       PROP_##name,

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path
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
#define gegl_property_double(name,...)                        \
    case PROP_##name:                                         \
      g_value_set_double (value, properties->name);           \
      break;
#define gegl_property_boolean(name,...)                       \
    case PROP_##name:                                         \
      g_value_set_boolean (value, properties->name);          \
      break;
#define gegl_property_int(name,...)                           \
    case PROP_##name:                                         \
      g_value_set_int (value, properties->name);              \
      break;
#define gegl_property_string(name, ...)                       \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_property_file_path(name, ...)                    \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);        \
      break;
#define gegl_property_enum(name, enum, enum_name, ...)        \
    case PROP_##name:                                         \
      g_value_set_enum (value, properties->name);             \
      break;
#define gegl_property_object(name, ...)                       \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_pointer(name, ...)                      \
    case PROP_##name:                                         \
      g_value_set_pointer (value, properties->name);          \
      break;
#define gegl_property_format(name, ...)                       \
    case PROP_##name:                                         \
      g_value_set_pointer (value, properties->name);          \
      break;
#define gegl_property_color(name, ...)                        \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_curve(name, ...)                        \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_seed(name, rand_name, ...)              \
    case PROP_##name:                                         \
      g_value_set_int (value, properties->name);              \
      break;
#define gegl_property_path(name, ...)                         \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path
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
#define gegl_property_double(name, ...)                               \
    case PROP_##name:                                                 \
      properties->name = g_value_get_double (value);                  \
      break;
#define gegl_property_boolean(name, ...)                              \
    case PROP_##name:                                                 \
      properties->name = g_value_get_boolean (value);                 \
      break;
#define gegl_property_int(name, ...)                                  \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      break;
#define gegl_property_string(name, ...)                               \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define gegl_property_file_path(name, ...)                            \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define gegl_property_enum(name, enum, enum_name, ...)                \
    case PROP_##name:                                                 \
      properties->name = g_value_get_enum (value);                    \
      break;
#define gegl_property_object(name, ...)                               \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_pointer(name, ...)                              \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define gegl_property_format(name, ...)                               \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define gegl_property_color(name, ...)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_curve(name, ...)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_seed(name, rand_name, ...)                      \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      if (!properties->rand_name)                                     \
        properties->rand_name = gegl_random_new_with_seed (properties->name);\
      else                                                            \
        gegl_random_set_seed (properties->rand_name, properties->name);\
      break;
#define gegl_property_path(name, ...)                                 \
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
      break; /*XXX*/

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }

  if (properties) {}; /* silence gcc */
}

static void gegl_op_destroy_notify (gpointer data)
{
  GeglProperties *properties = GEGL_PROPERTIES (data);

#define gegl_property_double(name, ...)
#define gegl_property_boolean(name, ...)
#define gegl_property_int(name, ...)
#define gegl_property_string(name, ...)             \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define gegl_property_file_path(name, ...)          \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define gegl_property_enum(name, enum, enum_name, ...)
#define gegl_property_object(name, ...)             \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_pointer(name, ...)
#define gegl_property_format(name, ...)
#define gegl_property_color(name, ...)              \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_curve(name, ...)              \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_seed(name, rand_name, ...)    \
  if (properties->rand_name)                        \
    {                                               \
      gegl_random_free (properties->rand_name);     \
      properties->rand_name = NULL;                 \
    }
#define gegl_property_path(name, ...)               \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path

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

#define gegl_property_double(name, ...)
#define gegl_property_boolean(name, ...)
#define gegl_property_int(name, ...)
#define gegl_property_string(name, ...)
#define gegl_property_file_path(name, ...)
#define gegl_property_enum(name, enum, enum_name, ...)
#define gegl_property_object(name, ...)
#define gegl_property_pointer(name, ...)
#define gegl_property_format(name, ...)
#define gegl_property_color(name, ...)              \
    if (properties->name == NULL)                   \
    {const char *def = gegl_operation_class_get_property_key (g_type_class_peek (type), #name, "default");properties->name = gegl_color_new(def?def:"black");}
#define gegl_property_seed(name, rand_name, ...)    \
    if (properties->rand_name == NULL)              \
      {properties->rand_name = gegl_random_new_with_seed (0);}
#define gegl_property_path(name, ...)
#define gegl_property_curve(name, ...)

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path

  g_object_set_data_full (obj, "chant-data", obj, gegl_op_destroy_notify);
  properties ++; /* evil hack to silence gcc */
  return obj;
}

static void
gegl_op_class_intern_init (gpointer klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = gegl_op_constructor;

#define gegl_property_double(name, foo...)                                   \
  { GParamSpec *pspec = gegl_param_spec_double_from_vararg (#name, foo);     \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_boolean(name, foo...)                                  \
  { GParamSpec *pspec = gegl_param_spec_boolean_from_vararg (#name, foo);    \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_int(name, foo...)                                      \
  { GParamSpec *pspec = gegl_param_spec_int_from_vararg (#name, foo);        \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_string(name, foo...)                                   \
  { GParamSpec *pspec = gegl_param_spec_string_from_vararg (#name, foo);     \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_file_path(name, foo...)                                \
  { GParamSpec *pspec = gegl_param_spec_file_path_from_vararg (#name, foo);  \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_enum(name, enum, enum_name, foo... )                   \
  { GParamSpec *pspec = gegl_param_spec_enum_from_vararg (#name,             \
                                     enum_name ## _get_type (), foo);        \
    g_object_class_install_property (object_class, PROP_##name, pspec);};

#define gegl_property_object(name, foo...)                                   \
  { GParamSpec *pspec = gegl_param_spec_object_from_vararg (#name, foo);     \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_pointer(name, foo...)                                  \
  { GParamSpec *pspec = gegl_param_spec_pointer_from_vararg (#name, foo);    \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_format(name, foo...)                                   \
  { GParamSpec *pspec = gegl_param_spec_format_from_vararg (#name, foo);     \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_seed(name, rand_name, foo...)                          \
  { GParamSpec *pspec = gegl_param_spec_seed_from_vararg (#name, foo);       \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_color(name, foo...)                                    \
  { GParamSpec *pspec = gegl_param_spec_color_from_vararg (#name, foo);      \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_path(name, foo...)                                     \
  { GParamSpec *pspec = gegl_param_spec_path_from_vararg (#name, foo);       \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_curve(name, foo...)                                    \
  { GParamSpec *pspec = gegl_param_spec_curve_from_vararg (#name, foo);      \
    g_object_class_install_property (object_class, PROP_##name, pspec);};

#include GEGL_OP_C_FILE

#undef gegl_property_double
#undef gegl_property_boolean
#undef gegl_property_int
#undef gegl_property_string
#undef gegl_property_file_path
#undef gegl_property_enum
#undef gegl_property_object
#undef gegl_property_pointer
#undef gegl_property_format
#undef gegl_property_color
#undef gegl_property_curve
#undef gegl_property_seed
#undef gegl_property_path
}

static void
gegl_op_init_properties (GeglOp *self)
{
  self->properties = g_slice_new0 (GeglProperties);
}

G_END_DECLS
