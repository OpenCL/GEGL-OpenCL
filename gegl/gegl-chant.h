#ifndef GEGL_CHANT_SELF
#error "GEGL_CHANT_SELF not defined"
#endif

#define GEGL_CHANT_PROPERTIES 1

/****************************************************************************/

#include <string.h>
#include <glib-object.h>

#include <gegl-plugin.h>

#ifdef GEGL_CHANT_SOURCE
  #include <gegl/gegl-operation-source.h>
  #include <gegl/gegl-extension-handler.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationSource
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationSourceClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_SOURCE
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_SOURCE_CLASS
#endif
#ifdef GEGL_CHANT_SINK
  #include <gegl/gegl-operation-sink.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationSink
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationSinkClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_SINK
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_SINK_CLASS
#endif
#ifdef GEGL_CHANT_FILTER
  #include <gegl/gegl-operation-filter.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationFilter
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationFilterClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_FILTER
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_FILTER_CLASS
#endif
#ifdef GEGL_CHANT_POINT_FILTER
  #include <gegl/gegl-operation-point-filter.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationPointFilter
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationPointFilterClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_POINT_FILTER
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_POINT_FILTER_CLASS
#endif
#ifdef GEGL_CHANT_COMPOSER
  #include <gegl/gegl-operation-composer.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationComposer
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationComposerClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_COMPOSER
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_COMPOSER_CLASS
#endif
#ifdef GEGL_CHANT_POINT_COMPOSER
  #include <gegl/gegl-operation-point-composer.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationPointComposer
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationPointComposerClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_POINT_COMPOSER
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_POINT_COMPOSER_CLASS
#endif
#ifdef GEGL_CHANT_META
  #include <gegl/gegl-operation-meta.h>
  #define GEGL_CHANT_PARENT_TypeName      GeglOperationMeta
  #define GEGL_CHANT_PARENT_TypeNameClass GeglOperationMetaClass
  #define GEGL_CHANT_PARENT_TYPE          GEGL_TYPE_OPERATION_META
  #define GEGL_CHANT_PARENT_CLASS         GEGL_OPERATION_META_CLASS
#endif

typedef struct Generated        GeglChantOperation;
typedef struct GeneratedClass   ChantClass;

struct Generated
{
  GEGL_CHANT_PARENT_TypeName  parent_instance;
#define gegl_chant_int(name, min, max, def, blurb)     gint       name;
#define gegl_chant_double(name, min, max, def, blurb)  gdouble    name;
#define gegl_chant_boolean(name, def, blurb)           gboolean   name;
#define gegl_chant_string(name, def, blurb)            gchar     *name;
#define gegl_chant_path(name, def, blurb)              gchar     *name;
#define gegl_chant_multiline(name, def, blurb)              gchar     *name;
#define gegl_chant_multiline(name, def, blurb)         gchar     *name;
#define gegl_chant_object(name, blurb)                 GObject   *name;
#define gegl_chant_pointer(name, blurb)                gpointer   name;
#define gegl_chant_color(name, def, blurb)             GeglColor *name;

#include GEGL_CHANT_SELF

/****************************************************************************/

/* undefining the chant macros before all subsequent inclusions */
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

/****************************************************************************/

  gpointer *priv;  /* free to use be implementer of operation */
};

struct GeneratedClass
{
  GEGL_CHANT_PARENT_TypeNameClass parent_class;
};

#define GEGL_CHANT_OPERATION(obj) ((GeglChantOperation*)(obj))

#include <gegl-module.h>

#ifndef GEGL_CHANT_STATIC
#  define M_DEFINE_TYPE_EXTENDED(type_name, TYPE_PARENT, flags, CODE) \
  \
static void     gegl_chant_init              (GeglChantOperation *self); \
static void     gegl_chant_class_init        (ChantClass    *klass); \
static GType    type_name##_get_type         (GTypeModule *module); \
const GeglModuleInfo * gegl_module_query     (GTypeModule *module);\
gboolean        gegl_module_register         (GTypeModule *module);\
static gpointer gegl_chant_parent_class = NULL; \
\
static void \
gegl_chant_class_intern_init (gpointer klass) \
{ \
  gegl_chant_parent_class = g_type_class_peek_parent (klass); \
  gegl_chant_class_init ((ChantClass*) klass); \
} \
\
static GType \
type_name##_get_type (GTypeModule *module) \
{ \
  static GType g_define_type_id = 0; \
  if (G_UNLIKELY (g_define_type_id == 0)) \
    { \
      static const GTypeInfo g_define_type_info = \
        { \
          sizeof (ChantClass), \
          (GBaseInitFunc) NULL, \
          (GBaseFinalizeFunc) NULL, \
          (GClassInitFunc) gegl_chant_class_intern_init, \
          (GClassFinalizeFunc) NULL, \
          NULL,   /* class_data */ \
          sizeof (GeglChantOperation), \
          0,      /* n_preallocs */ \
          (GInstanceInitFunc) gegl_chant_init, \
          NULL    /* value_table */ \
        }; \
      g_define_type_id = gegl_module_register_type (module, TYPE_PARENT,\
                                                    "GeglOpPlugIn-" #type_name,\
                                                    &g_define_type_info, \
                                                    (GTypeFlags) 0);\
    } \
  return g_define_type_id; \
}\
\
static const GeglModuleInfo modinfo =\
{\
 GEGL_MODULE_ABI_VERSION,\
 #type_name,\
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
#  define M_DEFINE_TYPE_EXTENDED(type_name, TYPE_PARENT, flags, CODE) \
  \
static void     gegl_chant_init              (GeglChantOperation   *self); \
static void     gegl_chant_class_init        (ChantClass *klass); \
static gpointer gegl_chant_parent_class = NULL; \
static void     gegl_chant_class_intern_init (gpointer klass) \
  { \
    gegl_chant_parent_class = g_type_class_peek_parent (klass); \
    gegl_chant_class_init ((ChantClass*) klass); \
  } \
\
static GType \
type_name##_get_type (GTypeModule *module) \
{ \
  static GType g_define_type_id = 0; \
  if (G_UNLIKELY (g_define_type_id == 0)) \
    { \
      static const GTypeInfo g_define_type_info = \
        { \
          sizeof (ChantClass), \
          (GBaseInitFunc) NULL, \
          (GBaseFinalizeFunc) NULL, \
          (GClassInitFunc) gegl_chant_class_intern_init, \
          (GClassFinalizeFunc) NULL, \
          NULL,   /* class_data */ \
          sizeof (GeglChantOperation), \
          0,      /* n_preallocs */ \
          (GInstanceInitFunc) gegl_chant_init, \
          NULL    /* value_table */ \
        }; \
      g_define_type_id = \
        g_type_register_static (TYPE_PARENT, "GeglOpPlugIn-" #type_name,\
                                &g_define_type_info, (GTypeFlags) flags); \
      { CODE ; } \
    } \
  return g_define_type_id; \
}
#endif

#define M_DEFINE_TYPE(t_n, T_P)   M_DEFINE_TYPE_EXTENDED (t_n, T_P, 0, )

M_DEFINE_TYPE (GEGL_CHANT_NAME, GEGL_CHANT_PARENT_TYPE)

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

#include GEGL_CHANT_SELF

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
  PROP_LAST
};

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (gobject);

  switch (property_id)
  {
#define gegl_chant_int(name, min, max, def, blurb)\
    case PROP_##name: g_value_set_int (value, self->name);break;
#define gegl_chant_double(name, min, max, def, blurb)\
    case PROP_##name: g_value_set_double (value, self->name);break;
#define gegl_chant_boolean(name, def, blurb)\
    case PROP_##name: g_value_set_boolean (value, self->name);break;
#define gegl_chant_string(name, def, blurb)\
    case PROP_##name: g_value_set_string (value, self->name);break;
#define gegl_chant_path(name, def, blurb)\
    case PROP_##name: g_value_set_string (value, self->name);break;
#define gegl_chant_multiline(name, def, blurb)\
    case PROP_##name: g_value_set_string (value, self->name);break;
#define gegl_chant_object(name, blurb)\
    case PROP_##name: g_value_set_object (value, self->name);break;
#define gegl_chant_pointer(name, blurb)\
    case PROP_##name: g_value_set_pointer (value, self->name);break;
#define gegl_chant_color(name, def, blurb)\
    case PROP_##name: g_value_set_object (value, self->name);break;

#include GEGL_CHANT_SELF

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
  self = NULL; /* silence GCC if no properties were defined */
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (gobject);

  switch (property_id)
  {
#define gegl_chant_int(name, min, max, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_int (value);\
      break;
#define gegl_chant_double(name, min, max, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_double (value);\
      break;
#define gegl_chant_boolean(name, def, blurb)\
    case PROP_##name:\
      self->name = g_value_get_boolean (value);\
      break;
#define gegl_chant_string(name, def, blurb)\
    case PROP_##name:\
      if (self->name)\
        g_free (self->name);\
      self->name = g_strdup (g_value_get_string (value));\
      break;
#define gegl_chant_path(name, def, blurb)\
    case PROP_##name:\
      if (self->name)\
        g_free (self->name);\
      self->name = g_strdup (g_value_get_string (value));\
      break;
#define gegl_chant_multiline(name, def, blurb)\
    case PROP_##name:\
      if (self->name)\
        g_free (self->name);\
      self->name = g_strdup (g_value_get_string (value));\
      break;
#define gegl_chant_object(name, blurb)\
    case PROP_##name:\
      if (self->name != NULL) \
         g_object_unref (self->name);\
      self->name = g_value_dup_object (value);\
      break;
#define gegl_chant_pointer(name, blurb)\
    case PROP_##name:\
      self->name = g_value_get_pointer (value);\
      break;
#define gegl_chant_color(name, def, blurb)\
    case PROP_##name:\
      if (self->name != NULL && G_IS_OBJECT (self->name))\
         g_object_unref (self->name);\
      self->name = g_value_get_object (value);\
      break;

#include GEGL_CHANT_SELF

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
  self = NULL; /* silence GCC if no properties were defined */
}

#ifdef GEGL_CHANT_POINT_FILTER
static gboolean process (GeglOperation *operation,
                         void          *in_buf,
                         void          *out_buf,
                         glong          samples);
#else
#ifdef GEGL_CHANT_POINT_COMPOSER
static gboolean process (GeglOperation *operation,
                         void          *in_buf, 
                         void          *aux_buf,
                         void          *out_buf,
                         glong          samples);
#else
#ifndef GEGL_CHANT_META
static gboolean process (GeglOperation *operation,
                         gpointer       context_id);
#endif
#endif
#endif

static void
gegl_chant_init (GeglChantOperation *self)
{
  self->priv = NULL;
}

#ifdef GEGL_CHANT_INIT
static void init (GeglChantOperation *self);
#endif

static void gegl_chant_destroy_notify (gpointer data)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (data);

#define gegl_chant_int(name, min, max, def, blurb)
#define gegl_chant_double(name, min, max, def, blurb)
#define gegl_chant_boolean(name, def, blurb)
#define gegl_chant_string(name, def, blurb)\
  if (self->name)\
    {\
      g_free (self->name);\
      self->name = NULL;\
    }
#define gegl_chant_path(name, def, blurb)\
  if (self->name)\
    {\
      g_free (self->name);\
      self->name = NULL;\
    }
#define gegl_chant_multiline(name, def, blurb)\
  if (self->name)\
    {\
      g_free (self->name);\
      self->name = NULL;\
    }
#define gegl_chant_object(name, blurb)\
  if (self->name)\
    {\
      g_object_unref (self->name);\
      self->name = NULL;\
    }
#define gegl_chant_pointer(name, blurb)
#define gegl_chant_color(name, def, blurb)\
  if (self->name)\
    {\
      g_object_unref (self->name);\
      self->name = NULL;\
    }

#include GEGL_CHANT_SELF

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
  self = NULL; /* shut up gcc warnings if there were no properties to clean up*/
}

static GObject *
gegl_chant_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_properties)
{
  GObject *obj;

  obj = G_OBJECT_CLASS (gegl_chant_parent_class)->constructor (
            type, n_construct_properties, construct_properties);

#ifdef GEGL_CHANT_INIT
  init (GEGL_CHANT_OPERATION (obj));
#endif

  g_object_set_data_full (obj, "chant-data", obj, gegl_chant_destroy_notify);

  return obj;
}

#ifdef GEGL_CHANT_CLASS_INIT
static void class_init (GeglOperationClass *operation_class);
#endif

#ifdef GEGL_CHANT_SOURCE
static GeglRectangle get_defined_region (GeglOperation *self);
#endif

static void
gegl_chant_class_init (ChantClass * klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass         *operation_class;

#ifndef GEGL_CHANT_META
  GEGL_CHANT_PARENT_TypeNameClass *parent_class = GEGL_CHANT_PARENT_CLASS (klass);
  parent_class->process = process;
#endif

  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  object_class->constructor  = gegl_chant_constructor;

#ifdef GEGL_CHANT_CLASS_INIT
  class_init (operation_class);
#endif

#ifdef GEGL_CHANT_SOURCE
  operation_class->get_defined_region = get_defined_region;
#endif

#define M_GEGL_CHANT_SET_NAME_EXTENDED(name) \
  gegl_operation_class_set_name (operation_class, #name);
#define M_GEGL_CHANT_SET_NAME(name)   M_GEGL_CHANT_SET_NAME_EXTENDED(name)
  M_GEGL_CHANT_SET_NAME (GEGL_CHANT_NAME);

#ifdef GEGL_CHANT_DESCRIPTION
  gegl_operation_class_set_description (operation_class, GEGL_CHANT_DESCRIPTION);
#endif

#ifdef GEGL_CHANT_CATEGORIES
  operation_class->categories = GEGL_CHANT_CATEGORIES;
#else
  operation_class->categories = "misc";
#endif

#define gegl_chant_int(name, min, max, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_int (#name, #name, blurb,\
                                                     min, max, def,\
                                                     (GParamFlags) (\
                                                     G_PARAM_READWRITE |\
                                                     G_PARAM_CONSTRUCT |\
                                                     GEGL_PAD_INPUT)));
#define gegl_chant_double(name, min, max, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_double (#name, #name, blurb,\
                                                        min, max, def,\
                                                        (GParamFlags) (\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT)));
#define gegl_chant_boolean(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_boolean (#name, #name, blurb,\
                                                         def,\
                                                         (GParamFlags) (\
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PAD_INPUT)));
#define gegl_chant_string(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_string (#name, #name, blurb,\
                                                        def,\
                                                        (GParamFlags) ( \
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT)));
#define gegl_chant_path(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   gegl_param_spec_path (#name, #name, blurb,\
                                                         FALSE, FALSE,\
                                                         def,\
                                                         (GParamFlags) ( \
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PAD_INPUT)));
#define gegl_chant_multiline(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   gegl_param_spec_multiline (#name, #name, blurb,\
                                                         def,\
                                                         (GParamFlags) ( \
                                                         G_PARAM_READWRITE |\
                                                         G_PARAM_CONSTRUCT |\
                                                         GEGL_PAD_INPUT)));
#define gegl_chant_object(name, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_object (#name, #name, blurb,\
                                                        G_TYPE_OBJECT,\
                                                        (GParamFlags) (\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT)));
#define gegl_chant_pointer(name, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   g_param_spec_pointer (#name, #name, blurb,\
                                                        (GParamFlags) (\
                                                        G_PARAM_READWRITE |\
                                                        G_PARAM_CONSTRUCT |\
                                                        GEGL_PAD_INPUT)));
#define gegl_chant_color(name, def, blurb)  \
  g_object_class_install_property (object_class, PROP_##name,\
                                   gegl_param_spec_color_from_string (#name, #name, blurb,\
                                                          def,\
                                                          (GParamFlags) (\
                                                          G_PARAM_READWRITE |\
                                                          G_PARAM_CONSTRUCT |\
                                                          GEGL_PAD_INPUT)));
#include GEGL_CHANT_SELF

#undef gegl_chant_int
#undef gegl_chant_double
#undef gegl_chant_boolean
#undef gegl_chant_string
#undef gegl_chant_path
#undef gegl_chant_multiline
#undef gegl_chant_object
#undef gegl_chant_pointer
#undef gegl_chant_color
}


/****************************************************************************/
