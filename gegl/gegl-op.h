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
#define gegl_property_double(name, label, ...)
#define gegl_property_string(name, label, ...)
#define gegl_property_file_path(name, label, ...)
#define gegl_property_int(name, label, ...)
#define gegl_property_boolean(name, label, ...)
#define gegl_property_enum(name, label, enum, enum_name, ...)
#define gegl_property_object(name, label, ...)
#define gegl_property_pointer(name, label, ...)
#define gegl_property_format(name, label, ...)
#define gegl_property_color(name, label, ...)
#define gegl_property_curve(name, label, ...)
#define gegl_property_seed(name, label, rand_name, ...)
#define gegl_property_path(name, label, ...)

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
#define gegl_property_double(name, label, ...)         gdouble     name;
#define gegl_property_boolean(name, label, ...)        gboolean    name;
#define gegl_property_int(name, label, ...)            gint        name;
#define gegl_property_string(name, label, ...)         gchar      *name;
#define gegl_property_file_path(name, label, ...)      gchar      *name;
#define gegl_property_enum(name, label, enum, enum_name, ...) enum        name;
#define gegl_property_object(name, label, ...)         GObject    *name;
#define gegl_property_pointer(name, label, ...)        gpointer    name;
#define gegl_property_format(name, label, ...)         gpointer    name;
#define gegl_property_color(name, label, ...)          GeglColor  *name;
#define gegl_property_curve(name, label, ...)          GeglCurve  *name;
#define gegl_property_seed(name, label, rand_name, ...)gint        name;\
                                                       GeglRandom *rand_name;
#define gegl_property_path(name, label, ...)                  GeglPath   *name;\
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

#define gegl_property_double(name, label, ...)     PROP_##name,
#define gegl_property_boolean(name, label, ...)    PROP_##name,
#define gegl_property_int(name, label, ...)        PROP_##name,
#define gegl_property_string(name, label, ...)     PROP_##name,
#define gegl_property_file_path(name, label, ...)  PROP_##name,
#define gegl_property_enum(name, label, enum, enum_name, ...)  PROP_##name,
#define gegl_property_object(name, label, ...)     PROP_##name,
#define gegl_property_pointer(name, label, ...)    PROP_##name,
#define gegl_property_format(name, label, ...)     PROP_##name,
#define gegl_property_color(name, label, ...)      PROP_##name,
#define gegl_property_curve(name, label, ...)      PROP_##name,
#define gegl_property_seed(name, label, rand_name, ...)   PROP_##name,
#define gegl_property_path(name, label, ...)       PROP_##name,

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
#define gegl_property_double(name, label,...)                 \
    case PROP_##name:                                         \
      g_value_set_double (value, properties->name);           \
      break;
#define gegl_property_boolean(name, label,...)                \
    case PROP_##name:                                         \
      g_value_set_boolean (value, properties->name);          \
      break;
#define gegl_property_int(name, label,...)                    \
    case PROP_##name:                                         \
      g_value_set_int (value, properties->name);              \
      break;
#define gegl_property_string(name, label, ...)                \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_property_file_path(name, label, ...)             \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_property_enum(name, label, enum, enum_name, ...) \
    case PROP_##name:                                         \
      g_value_set_enum (value, properties->name);             \
      break;
#define gegl_property_object(name, label, ...)                \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_pointer(name, label, ...)               \
    case PROP_##name:                                         \
      g_value_set_pointer (value, properties->name);          \
      break;
#define gegl_property_format(name, label, ...)                \
    case PROP_##name:                                         \
      g_value_set_pointer (value, properties->name);          \
      break;
#define gegl_property_color(name, label, ...)                 \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_curve(name, label, ...)                 \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_property_seed(name, label, rand_name, ...)       \
    case PROP_##name:                                         \
      g_value_set_int (value, properties->name);              \
      break;
#define gegl_property_path(name, label, ...)                  \
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
#define gegl_property_double(name, label, ...)                               \
    case PROP_##name:                                                 \
      properties->name = g_value_get_double (value);                  \
      break;
#define gegl_property_boolean(name, label, ...)                              \
    case PROP_##name:                                                 \
      properties->name = g_value_get_boolean (value);                 \
      break;
#define gegl_property_int(name, label, ...)                                  \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      break;
#define gegl_property_string(name, label, ...)                               \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define gegl_property_file_path(name, label, ...)                            \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_value_dup_string (value);                  \
      break;
#define gegl_property_enum(name, label, enum, enum_name, ...)                \
    case PROP_##name:                                                 \
      properties->name = g_value_get_enum (value);                    \
      break;
#define gegl_property_object(name, label, ...)                               \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_pointer(name, label, ...)                              \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define gegl_property_format(name, label, ...)                               \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define gegl_property_color(name, label, ...)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_curve(name, label, ...)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_property_seed(name, label, rand_name, ...)                      \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      if (!properties->rand_name)                                     \
        properties->rand_name = gegl_random_new_with_seed (properties->name);\
      else                                                            \
        gegl_random_set_seed (properties->rand_name, properties->name);\
      break;
#define gegl_property_path(name, label, ...)                          \
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

#define gegl_property_double(name, label, ...)
#define gegl_property_boolean(name, label, ...)
#define gegl_property_int(name, label, ...)
#define gegl_property_string(name, label, ...)      \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define gegl_property_file_path(name, label, ...)   \
  if (properties->name)                             \
    {                                               \
      g_free (properties->name);                    \
      properties->name = NULL;                      \
    }
#define gegl_property_enum(name, label, enum, enum_name, ...)
#define gegl_property_object(name, label, ...)      \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_pointer(name, label, ...)
#define gegl_property_format(name, label, ...)
#define gegl_property_color(name, label, ...)       \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_curve(name, label, ...)       \
  if (properties->name)                             \
    {                                               \
      g_object_unref (properties->name);            \
      properties->name = NULL;                      \
    }
#define gegl_property_seed(name, label, rand_name, ...)    \
  if (properties->rand_name)                        \
    {                                               \
      gegl_random_free (properties->rand_name);     \
      properties->rand_name = NULL;                 \
    }
#define gegl_property_path(name, label, ...)        \
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

#define gegl_property_double(name, label, ...)
#define gegl_property_boolean(name, label, ...)
#define gegl_property_int(name, label, ...)
#define gegl_property_string(name, label, ...)
#define gegl_property_file_path(name, label, ...)
#define gegl_property_enum(name, label, enum, enum_name, ...)
#define gegl_property_object(name, label, ...)
#define gegl_property_pointer(name, label, ...)
#define gegl_property_format(name, label, ...)
#define gegl_property_color(name, label, ...)              \
    if (properties->name == NULL)                   \
    {const char *def = gegl_operation_class_get_property_key (g_type_class_peek (type), #name, "default");properties->name = gegl_color_new(def?def:"black");}
#define gegl_property_seed(name, label, rand_name, ...)    \
    if (properties->rand_name == NULL)              \
      {properties->rand_name = gegl_random_new_with_seed (0);}
#define gegl_property_path(name, label, ...)
#define gegl_property_curve(name, label, ...)

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

#define gegl_property_double(name, label, foo...)                              \
  { GParamSpec *pspec = gegl_param_spec_double_from_vararg (#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_boolean(name, label, foo...)                             \
  { GParamSpec *pspec = gegl_param_spec_boolean_from_vararg(#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_int(name, label, foo...)                                 \
  { GParamSpec *pspec = gegl_param_spec_int_from_vararg (#name, label, foo);   \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_string(name, label, foo...)                              \
  { GParamSpec *pspec = gegl_param_spec_string_from_vararg (#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_file_path(name, label, foo...)                           \
  { GParamSpec *pspec =gegl_param_spec_file_path_from_vararg(#name,label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_enum(name, label, enum, enum_name, foo... )              \
  { GParamSpec *pspec = gegl_param_spec_enum_from_vararg (#name, label,        \
                                     enum_name ## _get_type (), foo);          \
    g_object_class_install_property (object_class, PROP_##name, pspec);};

#define gegl_property_object(name, label, foo...)                              \
  { GParamSpec *pspec = gegl_param_spec_object_from_vararg (#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_pointer(name, label, foo...)                             \
  { GParamSpec *pspec = gegl_param_spec_pointer_from_vararg(#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_format(name, label, foo...)                              \
  { GParamSpec *pspec = gegl_param_spec_format_from_vararg (#name, label, foo);\
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_seed(name, label, rand_name, foo...)                     \
  { GParamSpec *pspec = gegl_param_spec_seed_from_vararg (#name, label, foo);  \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_color(name, label, foo...)                               \
  { GParamSpec *pspec = gegl_param_spec_color_from_vararg (#name, label, foo); \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_path(name, label, foo...)                                \
  { GParamSpec *pspec = gegl_param_spec_path_from_vararg (#name, label, foo);  \
    g_object_class_install_property (object_class, PROP_##name, pspec);};
#define gegl_property_curve(name, label, foo...)                               \
  { GParamSpec *pspec = gegl_param_spec_curve_from_vararg (#name, label, foo); \
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
