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

#ifndef GEGL_CHANT_C_FILE
#error "GEGL_CHANT_C_FILE not defined"
#endif

#include <gegl-plugin.h>

GType operation_get_type ();
typedef struct _GeglChantO  GeglChantO;
typedef struct _GeglChant   GeglChant;

static void operation_register_type (GTypeModule *module);
static void gegl_chant_init         (GeglChant   *self);
static void gegl_chant_class_init   (gpointer     klass);

#define GEGL_DEFINE_DYNAMIC_OPERATION(T_P)  GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED (GEGL_CHANT_C_FILE, GeglChant, operation, T_P, 0, {})
#define GEGL_DEFINE_DYNAMIC_OPERATION_EXTENDED(C_FILE, TypeName, type_name, TYPE_PARENT, flags, CODE) \
  static void     type_name##_init              (TypeName        *self); \
static void     type_name##_class_init        (TypeName##Class *klass); \
static void     type_name##_class_finalize    (TypeName##Class *klass); \
static gpointer type_name##_parent_class = NULL; \
static GType    type_name##_type_id = 0; \
static void     type_name##_class_intern_init (gpointer klass) \
  { \
    type_name##_parent_class = g_type_class_peek_parent (klass); \
    type_name##_class_init ((TypeName##Class*) klass); \
    gegl_chant_class_init (klass); \
  } \
GType \
type_name##_get_type (void) \
  { \
    return type_name##_type_id; \
  } \
static void \
type_name##_register_type (GTypeModule *type_module) \
  { \
    gchar tempname[256];    \
    gchar *p;               \
    GType g_define_type_id; \
    const GTypeInfo g_define_type_info = { \
          sizeof (TypeName##Class), \
          (GBaseInitFunc) NULL, \
          (GBaseFinalizeFunc) NULL, \
          (GClassInitFunc) type_name##_class_intern_init, \
          (GClassFinalizeFunc) type_name##_class_finalize, \
          NULL,   /* class_data */ \
          sizeof (TypeName), \
          0,      /* n_preallocs */ \
          (GInstanceInitFunc) type_name##_init, \
          NULL    /* value_table */ \
        }; \
    g_snprintf (tempname, 256, "%s", #TypeName GEGL_CHANT_C_FILE);\
    for (p = &tempname[0]; *p; p++) {\
      if (*p=='.') *p='_';\
    }\
    type_name##_type_id = g_type_module_register_type (type_module, \
                                                       TYPE_PARENT, \
                                                       tempname,    \
                                                       &g_define_type_info, \
                                                       (GTypeFlags) flags); \
    g_define_type_id = type_name##_type_id; \
    { CODE ; } \
  }


#define GEGL_CHANT_PROPERTIES(op) \
    ((GeglChantO*)(((GeglChant*)(op))->properties))

/****************************************************************************/


#ifdef GEGL_CHANT_TYPE_OPERATION
#include <operation/gegl-operation.h>
struct _GeglChant
{
  GeglOperation parent_instance;
  gpointer      properties;
};

typedef struct
{
  GeglOperationClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION);
#endif

#ifdef GEGL_CHANT_TYPE_META
#include <operation/gegl-operation.h>
typedef struct
{
  GeglOperationMeta parent_instance;
  gpointer          properties;
} GeglChant;

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META);
#endif

#ifdef GEGL_CHANT_TYPE_SOURCE
#include <operation/gegl-operation-source.h>
typedef struct
{
  GeglOperationSource parent_instance;
  gpointer            properties;
} GeglChant;

typedef struct
{
  GeglOperationSourceClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SOURCE);
#endif

#ifdef GEGL_CHANT_TYPE_SINK
#include <operation/gegl-operation-sink.h>
struct _GeglChant
{
  GeglOperationSink parent_instance;
  gpointer          properties;
};

typedef struct
{
  GeglOperationSinkClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SINK);
#endif

#ifdef GEGL_CHANT_TYPE_FILTER
#include <operation/gegl-operation-filter.h>
struct _GeglChant
{
  GeglOperationFilter parent_instance;
  gpointer            properties;
};

typedef struct
{
  GeglOperationFilterClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_FILTER);
#endif

#ifdef GEGL_CHANT_TYPE_COMPOSER
#include <operation/gegl-operation-composer.h>
struct _GeglChant
{
  GeglOperationComposer parent_instance;
  gpointer              properties;
};

typedef struct
{
  GeglOperationComposerClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_COMPOSER);

#endif

#ifdef GEGL_CHANT_TYPE_POINT_FILTER
#include <operation/gegl-operation-point-filter.h>
struct _GeglChant
{
  GeglOperationPointFilter parent_instance;
  gpointer                 properties;
};

typedef struct
{
  GeglOperationPointFilterClass parent_class;
} GeglChantClass;

GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_POINT_FILTER);

#endif

#ifdef GEGL_CHANT_TYPE_AREA_FILTER
#include <operation/gegl-operation-area-filter.h>
struct _GeglChant
{
  GeglOperationAreaFilter parent_instance;
  gpointer                properties;
};

typedef struct
{
  GeglOperationAreaFilterClass parent_class;
} GeglChantClass;
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_AREA_FILTER);
#endif


#ifdef GEGL_CHANT_TYPE_POINT_COMPOSER
#include <operation/gegl-operation-point-composer.h>
struct _GeglChant
{
  GeglOperationPointComposer parent_instance;
  gpointer                   properties;
};

typedef struct
{
  GeglOperationPointComposerClass parent_class;
} GeglChantClass;
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_POINT_COMPOSER);
#endif



/* if GEGL_CHANT_CUSTOM is defined you have to provide the following
 * code or your own implementation of it
 */
#ifndef GEGL_CHANT_CUSTOM
static void
operation_init (GeglChant *self)
{
  gegl_chant_init (self);
}

static void
operation_class_finalize (GeglChantClass *self)
{
}

static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION, GEGL_CHANT_C_FILE, "v0.0", "foo and bar"
};

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  operation_register_type (module);

  return TRUE;
}
#endif


struct _GeglChantO
{
  gpointer dummy_filler; /* to avoid empty struct, can be done a bit more cleverly to
                            avoid adding it when there is actual properties*/
#define gegl_chant_int(name, min, max, def, blurb)     gint        name;
#define gegl_chant_double(name, min, max, def, blurb)  gdouble     name;
#define gegl_chant_boolean(name, def, blurb)           gboolean    name;
#define gegl_chant_string(name, def, blurb)            gchar      *name;
#define gegl_chant_path(name, def, blurb)              gchar      *name;
#define gegl_chant_multiline(name, def, blurb)         gchar      *name;
#define gegl_chant_multiline(name, def, blurb)         gchar      *name;
#define gegl_chant_object(name, blurb)                 GObject    *name;
#define gegl_chant_pointer(name, blurb)                gpointer    name;
#define gegl_chant_color(name, def, blurb)             GeglColor  *name;
#define gegl_chant_curve(name, blurb)                  GeglCurve  *name;
#define gegl_chant_vector(name, blurb)                 GeglVector *name;

#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector
};

#define GEGL_CHANT_OPERATION(obj) ((Operation*)(obj))

enum
{
  PROP_0,
#define gegl_chant_int(name, min, max, def, blurb)     PROP_##name,
#define gegl_chant_double(name, min, max, def, blurb)  PROP_##name,
#define gegl_chant_boolean(name, def, blurb)           PROP_##name,
#define gegl_chant_string(name, def, blurb)            PROP_##name,
#define gegl_chant_path(name, def, blurb)              PROP_##name,
#define gegl_chant_multiline(name, def, blurb)         PROP_##name,
#define gegl_chant_object(name, blurb)                 PROP_##name,
#define gegl_chant_pointer(name, blurb)                PROP_##name,
#define gegl_chant_color(name, def, blurb)             PROP_##name,
#define gegl_chant_curve(name, blurb)                  PROP_##name,
#define gegl_chant_vector(name, blurb)                 PROP_##name,

#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector
  PROP_LAST
};

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglChantO *properties;

  properties = GEGL_CHANT_PROPERTIES(gobject);

  switch (property_id)
  {
#define gegl_chant_int(name, min, max, def, blurb)            \
    case PROP_##name:                                         \
      g_value_set_int (value, properties->name);              \
      break;
#define gegl_chant_double(name, min, max, def, blurb)         \
    case PROP_##name:                                         \
      g_value_set_double (value, properties->name);           \
      break;
#define gegl_chant_boolean(name, def, blurb)                  \
    case PROP_##name:                                         \
      g_value_set_boolean (value, properties->name);          \
      break;
#define gegl_chant_string(name, def, blurb)                   \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_chant_path(name, def, blurb)                     \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_chant_multiline(name, def, blurb)                \
    case PROP_##name:                                         \
      g_value_set_string (value, properties->name);           \
      break;
#define gegl_chant_object(name, blurb)                        \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_chant_pointer(name, blurb)                       \
    case PROP_##name:                                         \
      g_value_set_pointer (value, properties->name);          \
      break;
#define gegl_chant_color(name, def, blurb)                    \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_chant_curve(name, blurb)                         \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;
#define gegl_chant_vector(name, blurb)                        \
    case PROP_##name:                                         \
      g_value_set_object (value, properties->name);           \
      break;/*XXX*/

#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglChantO *properties;

  properties = GEGL_CHANT_PROPERTIES(gobject);

  switch (property_id)
  {
#define gegl_chant_int(name, min, max, def, blurb)                    \
    case PROP_##name:                                                 \
      properties->name = g_value_get_int (value);                     \
      break;
#define gegl_chant_double(name, min, max, def, blurb)                 \
    case PROP_##name:                                                 \
      properties->name = g_value_get_double (value);                  \
      break;
#define gegl_chant_boolean(name, def, blurb)                          \
    case PROP_##name:                                                 \
      properties->name = g_value_get_boolean (value);                 \
      break;
#define gegl_chant_string(name, def, blurb)                           \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_strdup (g_value_get_string (value));       \
      break;
#define gegl_chant_path(name, def, blurb)                             \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_strdup (g_value_get_string (value));       \
      break;
#define gegl_chant_multiline(name, def, blurb)                        \
    case PROP_##name:                                                 \
      if (properties->name)                                           \
        g_free (properties->name);                                    \
      properties->name = g_strdup (g_value_get_string (value));       \
      break;
#define gegl_chant_object(name, blurb)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL)                                   \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_chant_pointer(name, blurb)                               \
    case PROP_##name:                                                 \
      properties->name = g_value_get_pointer (value);                 \
      break;
#define gegl_chant_color(name, def, blurb)                            \
    case PROP_##name:                                                 \
      if (properties->name != NULL && G_IS_OBJECT (properties->name)) \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_chant_curve(name, blurb)                                 \
    case PROP_##name:                                                 \
      if (properties->name != NULL && G_IS_OBJECT (properties->name)) \
         g_object_unref (properties->name);                           \
      properties->name = g_value_dup_object (value);                  \
      break;
#define gegl_chant_vector(name, blurb)                                \
    case PROP_##name:                                                 \
      if (properties->name != NULL && G_IS_OBJECT (properties->name)) \
        {/*XXX: remove old signal */                                  \
         g_object_unref (properties->name);                           \
        }                                                             \
      properties->name = g_value_dup_object (value);                  \
      g_signal_connect (G_OBJECT (properties->name), "changed",       \
       G_CALLBACK(gegl_operation_vector_prop_changed), self);         \
      break; /*XXX*/

#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void gegl_chant_destroy_notify (gpointer data)
{
  GeglChantO *properties;

  properties = GEGL_CHANT_PROPERTIES (data);

#define gegl_chant_int(name, min, max, def, blurb)
#define gegl_chant_double(name, min, max, def, blurb)
#define gegl_chant_boolean(name, def, blurb)
#define gegl_chant_string(name, def, blurb)   \
  if (properties->name)                       \
    {                                         \
      g_free (properties->name);              \
      properties->name = NULL;                \
    }
#define gegl_chant_path(name, def, blurb)     \
  if (properties->name)                       \
    {                                         \
      g_free (properties->name);              \
      properties->name = NULL;                \
    }
#define gegl_chant_multiline(name, def, blurb)\
  if (properties->name)                       \
    {                                         \
      g_free (properties->name);              \
      properties->name = NULL;                \
    }
#define gegl_chant_object(name, blurb)        \
  if (properties->name)                       \
    {                                         \
      g_object_unref (properties->name);      \
      properties->name = NULL;                \
    }
#define gegl_chant_pointer(name, blurb)
#define gegl_chant_color(name, def, blurb)    \
  if (properties->name)                       \
    {                                         \
      g_object_unref (properties->name);      \
      properties->name = NULL;                \
    }
#define gegl_chant_curve(name, blurb)         \
  if (properties->name)                       \
    {\
      g_object_unref (properties->name);      \
      properties->name = NULL;                \
    }
#define gegl_chant_vector(name, blurb)        \
  if (properties->name)                       \
    {                                         \
      g_object_unref (properties->name);      \
      properties->name = NULL;                \
    }

#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector
  g_free (properties);
}

static GObject *
gegl_chant_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_properties)
{
  GObject *obj;
  GObjectClass *parent_class =
      g_type_class_peek_parent (g_type_class_peek (type));

  obj = G_OBJECT_CLASS (parent_class)->constructor (
            type, n_construct_properties, construct_properties);

  g_object_set_data_full (obj, "chant-data", obj, gegl_chant_destroy_notify);
  return obj;
}

static void
gegl_chant_class_init (gpointer klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = gegl_chant_constructor;

/*  g_type_class_add_private (klass, sizeof (GeglChantO));*/

#define gegl_chant_int(name, min, max, def, blurb)                          \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_int (#name, #name, blurb,   \
                                                     min, max, def,         \
                                                     (GParamFlags) (        \
                                                     G_PARAM_READWRITE |    \
                                                     G_PARAM_CONSTRUCT |    \
                                                     GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_double(name, min, max, def, blurb)                       \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_double (#name, #name, blurb,\
                                                        min, max, def,      \
                                                        (GParamFlags) (     \
                                                        G_PARAM_READWRITE | \
                                                        G_PARAM_CONSTRUCT | \
                                                        GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_boolean(name, def, blurb)                                \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_boolean (#name, #name, blurb,\
                                                         def,               \
                                                         (GParamFlags) (    \
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_string(name, def, blurb)                                 \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_string (#name, #name, blurb,\
                                                        def,                \
                                                        (GParamFlags) (     \
                                                        G_PARAM_READWRITE | \
                                                        G_PARAM_CONSTRUCT | \
                                                        GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_path(name, def, blurb)                                   \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   gegl_param_spec_path (#name, #name, blurb,\
                                                         FALSE, FALSE,      \
                                                         def,               \
                                                         (GParamFlags) (    \
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_multiline(name, def, blurb)                              \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   gegl_param_spec_multiline (#name, #name, blurb,\
                                                         def,               \
                                                         (GParamFlags) (    \
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_object(name, blurb)                                      \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_object (#name, #name, blurb,\
                                                        G_TYPE_OBJECT,      \
                                                        (GParamFlags) (     \
                                                        G_PARAM_READWRITE | \
                                                        G_PARAM_CONSTRUCT | \
                                                        GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_pointer(name, blurb)                                     \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   g_param_spec_pointer (#name, #name, blurb,\
                                                        (GParamFlags) (     \
                                                        G_PARAM_READWRITE | \
                                                        G_PARAM_CONSTRUCT | \
                                                        GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_color(name, def, blurb)                                  \
  g_object_class_install_property (object_class, PROP_##name,               \
                                   gegl_param_spec_color_from_string (#name, #name, blurb,\
                                                          def,              \
                                                          (GParamFlags) (   \
                                                          G_PARAM_READWRITE |\
                                                          G_PARAM_CONSTRUCT |\
                                                          GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_curve(name, blurb)                                        \
  g_object_class_install_property (object_class, PROP_##name,                 \
                                   gegl_param_spec_curve (#name, #name, blurb,\
                                                          gegl_curve_default_curve(),\
                                                          (GParamFlags) (     \
                                                          G_PARAM_READWRITE | \
                                                          G_PARAM_CONSTRUCT | \
                                                          GEGL_PARAM_PAD_INPUT)));
#define gegl_chant_vector(name, blurb)                                        \
  g_object_class_install_property (object_class, PROP_##name,                 \
                                   gegl_param_spec_vector (#name, #name, blurb,\
                                                           NULL,              \
                                                          (GParamFlags) (     \
                                                          G_PARAM_READWRITE | \
                                                          G_PARAM_CONSTRUCT | \
                                                          GEGL_PARAM_PAD_INPUT)));
#include GEGL_CHANT_C_FILE

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
#undef gegl_chant_curve
#undef gegl_chant_vector
}


static void
gegl_chant_init (GeglChant *self)
{
  self->properties = g_new0 (GeglChantO, 1);
}

/****************************************************************************/
