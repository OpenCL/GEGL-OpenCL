#ifndef CHANT_SELF
#error "CHANT_SELF not defined"
#endif

/****************************************************************************/

#include <string.h>
#include <glib-object.h>

#include <gegl.h>

#ifdef CHANT_SUPER_CLASS_SOURCE
  #include <gegl/gegl-operation-source.h>
  #define CHANT_PARENT_TypeName      GeglOperationSource
  #define CHANT_PARENT_TypeNameClass GeglOperationSourceClass
  #define CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_SOURCE
  #define CHANT_PARENT_CLASS         GEGL_OPERATION_SOURCE_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_SINK
  #include "op_sink.h"
  #define CHANT_PARENT_TypeName      OpSink
  #define CHANT_PARENT_TypeNameClass OpSinkClass
  #define CHANT_PARENT_TYPE          TYPE_OP_SINK
  #define CHANT_PARENT_CLASS         OP_SINK_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_FILTER
  #include <gegl/gegl-operation-filter.h>
  #define CHANT_PARENT_TypeName      GeglOperationFilter
  #define CHANT_PARENT_TypeNameClass GeglOperationFilterClass
  #define CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_FILTER
  #define CHANT_PARENT_CLASS         GEGL_OPERATION_FILTER_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_POINT_FILTER
  #include "op_point_filter.h"
  #define CHANT_PARENT_TypeName      OpPointFilter
  #define CHANT_PARENT_TypeNameClass OpPointFilterClass
  #define CHANT_PARENT_TYPE          TYPE_OP_POINT_FILTER
  #define CHANT_PARENT_CLASS         OP_POINT_FILTER_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_COMPOSER
  #include <gegl/gegl-operation-composer.h>
  #define CHANT_PARENT_TypeName      GeglOperationComposer
  #define CHANT_PARENT_TypeNameClass GeglOperationComposerClass
  #define CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_COMPOSER
  #define CHANT_PARENT_CLASS         GEGL_OPERATION_COMPOSER_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_POINT_COMPOSER
  #include "op_point_composer.h"
  #define CHANT_PARENT_TypeName      OpPointComposer
  #define CHANT_PARENT_TypeNameClass OpPointComposerClass
  #define CHANT_PARENT_TYPE          TYPE_OP_POINT_COMPOSER
  #define CHANT_PARENT_CLASS         OP_POINT_COMPOSER_CLASS
#endif
#ifdef CHANT_SUPER_CLASS_AFFINE
  #include "op_affine.h"
  #define CHANT_PARENT_TypeName      OpAffine
  #define CHANT_PARENT_TypeNameClass OpAffineClass
  #define CHANT_PARENT_TYPE          TYPE_OP_AFFINE
  #define CHANT_PARENT_CLASS         OP_AFFINE_CLASS
#endif

typedef struct Generated        CHANT_TypeName;
typedef struct GeneratedClass   CHANT_GENERATED_CLASS;

struct Generated
{
  CHANT_PARENT_TypeName  parent_instance;
#define chant_int(name, min, max, def, blurb)     gint     name;
#define chant_double(name, min, max, def, blurb)  gdouble  name;
#define chant_float(name, min, max, def, blurb)   gfloat   name;
#define chant_boolean(name, def, blurb)           gboolean name;
#define chant_string(name, def, blurb)            gchar   *name;
#define chant_object(name, blurb)                 GObject *name;
#define chant_pointer(name, blurb)                gpointer name;

#include CHANT_SELF

/****************************************************************************/

/* undefining the chant macros before all subsequent inclusions */
#undef chant_int
#undef chant_double
#undef chant_float
#undef chant_boolean
#undef chant_string
#undef chant_object
#undef chant_pointer

/****************************************************************************/
};

struct GeneratedClass
{
  CHANT_PARENT_TypeNameClass parent_class;
};

#include <gegl-module.h>

#ifndef CHANT_STATIC
#  define M_DEFINE_TYPE_EXTENDED(TypeName, MAG_NAME, type_name, TYPE_PARENT, flags, CODE) \
  \
static void     chant_init              (TypeName        *self); \
static void     chant_class_init        (CHANT_GENERATED_CLASS *klass); \
static gpointer chant_parent_class = NULL; \
static void     chant_class_intern_init (gpointer klass) \
  { \
    chant_parent_class = g_type_class_peek_parent (klass); \
    chant_class_init ((CHANT_GENERATED_CLASS*) klass); \
  } \
\
GType \
type_name##_get_type (GTypeModule *module) \
  { \
    static GType g_define_type_id = 0; \
    if (G_UNLIKELY (g_define_type_id == 0)) \
      { \
            static const GTypeInfo g_define_type_info = { \
                      sizeof (CHANT_GENERATED_CLASS), \
                      (GBaseInitFunc) NULL, \
                      (GBaseFinalizeFunc) NULL, \
                      (GClassInitFunc) chant_class_intern_init, \
                      (GClassFinalizeFunc) NULL, \
                      NULL,   /* class_data */ \
                      sizeof (TypeName), \
                      0,      /* n_preallocs */ \
                      (GInstanceInitFunc) chant_init, \
                      NULL    /* value_table */ \
                    }; \
            g_define_type_id = gegl_module_register_type (module,\
                                                            TYPE_PARENT,\
                          "GeglOpPlugIn-" MAG_NAME, &g_define_type_info, 0);\
          } \
    return g_define_type_id; \
  }\
\
static const GeglModuleInfo modinfo =\
{\
 GEGL_MODULE_ABI_VERSION,\
 MAG_NAME,\
 "v0.0",\
 "(c) 2006, released under the LGPL",\
 "June 2006"\
};\
\
const GeglModuleInfo *\
gegl_module_query (GTypeModule *module)\
{\
  return &modinfo;\
}\
\
gboolean \
gegl_module_register (GTypeModule *module)\
{\
  type_name##_get_type (module);\
  return TRUE;\
}

#else
#  define M_DEFINE_TYPE_EXTENDED(TypeName, MAG_NAME, type_name, TYPE_PARENT, flags, CODE) \
  \
static void     chant_init              (TypeName        *self); \
static void     chant_class_init        (CHANT_GENERATED_CLASS *klass); \
static gpointer chant_parent_class = NULL; \
static void     chant_class_intern_init (gpointer klass) \
  { \
    chant_parent_class = g_type_class_peek_parent (klass); \
    chant_class_init ((CHANT_GENERATED_CLASS*) klass); \
  } \
\
GType \
type_name##_get_type (GTypeModule *module) \
  { \
    static GType g_define_type_id = 0; \
    if (G_UNLIKELY (g_define_type_id == 0)) \
      { \
            static const GTypeInfo g_define_type_info = { \
                      sizeof (CHANT_GENERATED_CLASS), \
                      (GBaseInitFunc) NULL, \
                      (GBaseFinalizeFunc) NULL, \
                      (GClassInitFunc) chant_class_intern_init, \
                      (GClassFinalizeFunc) NULL, \
                      NULL,   /* class_data */ \
                      sizeof (TypeName), \
                      0,      /* n_preallocs */ \
                      (GInstanceInitFunc) chant_init, \
                      NULL    /* value_table */ \
                    }; \
            g_define_type_id = g_type_register_static (TYPE_PARENT, #TypeName, &g_define_type_info, (GTypeFlags) flags); \
            { CODE ; } \
          } \
    return g_define_type_id; \
  }
#endif

#define M_DEFINE_TYPE(TN, Mn, t_n, T_P)   M_DEFINE_TYPE_EXTENDED (TN, Mn, t_n, T_P, 0, )

M_DEFINE_TYPE (CHANT_TypeName, CHANT_NAME, CHANT_type_name, CHANT_PARENT_TYPE)

enum
{
  PROP_0,
#define chant_int(name, min, max, def, blurb)     PROP_##name,
#define chant_double(name, min, max, def, blurb)  PROP_##name,
#define chant_float(name, min, max, def, blurb)   PROP_##name,
#define chant_boolean(name, def, blurb)           PROP_##name,
#define chant_string(name, def, blurb)            PROP_##name,
#define chant_object(name, blurb)                 PROP_##name,
#define chant_pointer(name, blurb)                PROP_##name,

#include CHANT_SELF

#undef chant_int
#undef chant_double
#undef chant_float
#undef chant_boolean
#undef chant_string
#undef chant_object
#undef chant_pointer
  PROP_LAST
};

static void get_property (GObject      *object,
                          guint         property_id,
                          GValue       *value,
                          GParamSpec   *pspec)
{
  CHANT_TypeName *self = (CHANT_TypeName*) object;

  switch (property_id)
  {
#define chant_int(name, min, max, def, blurb)\
    case PROP_##name: g_value_set_int (value, self->name);break;
#define chant_double(name, min, max, def, blurb)\
    case PROP_##name: g_value_set_double (value, self->name);break;
#define chant_float(name, min, max, def, blurb)\
    case PROP_##name: g_value_set_float (value, self->name);break;
#define chant_boolean(name, def, blurb)\
    case PROP_##name: g_value_set_boolean (value, self->name);break;
#define chant_string(name, def, blurb)\
    case PROP_##name: g_value_set_string (value, self->name);break;
#define chant_object(name, blurb)\
    case PROP_##name: g_value_set_object (value, self->name);break;
#define chant_pointer(name, blurb)\
    case PROP_##name: g_value_set_pointer (value, self->name);break;

#include CHANT_SELF

#undef chant_int
#undef chant_double
#undef chant_float
#undef chant_boolean
#undef chant_string
#undef chant_object
#undef chant_pointer
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  self = NULL; /* silence GCC if no properties were defined */
}

static void set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  CHANT_TypeName *self = (CHANT_TypeName*) object;

  switch (property_id)
  {
#define chant_int(name, min, max, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_int (value);\
      break;
#define chant_double(name, min, max, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_double (value);\
      break;
#define chant_float(name, min, max, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_float (value);\
      break;
#define chant_boolean(name, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_boolean (value);\
      break;
#define chant_string(name, def, blurb)\
    case PROP_##name:\
      if (self->name)\
        g_free (self->name);\
      self->name = g_strdup (g_value_get_string (value));\
      break;
#define chant_object(name, blurb)\
    case PROP_##name:\
      if (self->name != NULL) \
         g_object_unref (self->name);\
      self->name = g_value_get_object (value);\
      break;
#define chant_pointer(name, blurb)\
    case PROP_##name:\
      self->name = g_value_get_pointer (value);\
      break;

#include CHANT_SELF

#undef chant_int
#undef chant_double
#undef chant_float
#undef chant_boolean
#undef chant_string
#undef chant_object
#undef chant_pointer

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  self = NULL; /* silence GCC if no properties were defined */
}

#ifdef CHANT_SUPER_CLASS_POINT_FILTER
static gboolean evaluate (GeglOperation *operation,
                          guchar        *buf,
                          gint           n_pixels);
#else
#ifdef CHANT_SUPER_CLASS_POINT_COMPOSER
static gboolean evaluate (GeglOperation *operation,
                          guchar        *src_buf,
                          guchar        *aux_buf,
                          guchar        *out_buf,
                          gint           n_pixels);
#else
#ifdef CHANT_SUPER_CLASS_AFFINE
static void create_matrix (CHANT_TypeName *operation,
                           Matrix3         matrix);
#else
static gboolean evaluate (GeglOperation *operation,
                          const gchar   *output_prop);
#endif
#endif
#endif

static void chant_init (CHANT_TypeName *self)
{
}

#ifdef CHANT_CONSTRUCT
static void init (CHANT_TypeName *self);

static GObject *
chant_constructor (GType                  type,
                   guint                  n_construct_properties,
                   GObjectConstructParam *construct_properties)
{
  GObject *obj;

  obj = G_OBJECT_CLASS (chant_parent_class)->constructor (
            type, n_construct_properties, construct_properties);

  init ((CHANT_TypeName *) obj);

  return obj;
}
#endif

#ifdef CHANT_CLASS_CONSTRUCT
static void class_init (GeglOperationClass *operation_class);
#endif

#ifdef CHANT_SUPER_CLASS_SOURCE
static gboolean calc_have_rect (GeglOperation *self);
#endif

static void
chant_class_init (CHANT_GENERATED_CLASS * klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  CHANT_PARENT_TypeNameClass *parent_class = CHANT_PARENT_CLASS (klass);
  GeglOperationClass         *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
#ifdef CHANT_CONSTRUCT
  object_class->constructor  = chant_constructor;
#endif
#ifdef CHANT_SUPER_CLASS_AFFINE
  parent_class->create_matrix = (OpAffineCreateMatrixFunc) create_matrix;
#else
  parent_class->evaluate     = evaluate;
#endif

#ifdef CHANT_SUPER_CLASS_SOURCE
  operation_class->calc_have_rect = calc_have_rect;
#endif
  gegl_operation_class_set_name (operation_class, CHANT_NAME);
#ifdef CHANT_CLASS_CONSTRUCT
  class_init (operation_class);
#endif

#define chant_int(name, min, max, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_int (#name, #name, blurb,\
                                                     min, max, def,\
                                                     G_PARAM_READWRITE |\
                                                     G_PARAM_CONSTRUCT |\
                                                     GEGL_PAD_INPUT));
#define chant_double(name, min, max, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_double (#name, #name, blurb,\
                                                        min, max, def,\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT));
#define chant_float(name, min, max, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_float (#name, #name, blurb,\
                                                        min, max, def,\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT));
#define chant_boolean(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_boolean (#name, #name, blurb,\
                                                         def,\
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PAD_INPUT));
#define chant_string(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_string (#name, #name, blurb,\
                                                        def,\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT));
#define chant_object(name, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_object (#name, #name, blurb,\
                                                        G_TYPE_OBJECT,\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT));
#define chant_pointer(name, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_pointer (#name, #name, blurb,\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT));
#include CHANT_SELF

#undef chant_int
#undef chant_double
#undef chant_float
#undef chant_boolean
#undef chant_string
#undef chant_object
#undef chant_pointer
}

/****************************************************************************/
