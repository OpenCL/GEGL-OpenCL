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
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 *
 * Original contents copied from gimp/app/core/gimpparamspecs.h
 * (c) 1995-2006 Spencer Kimball, Peter Mattis and others.
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include "gegl-paramspecs.h"


/*
 * GEGL_TYPE_INT32
 */

GType
gegl_int32_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GeglInt32", &info, 0);
    }

  return type;
}


/*
 * GEGL_TYPE_PARAM_INT32
 */

static void   gegl_param_int32_class_init (GParamSpecClass *klass);
static void   gegl_param_int32_init (GParamSpec *pspec);

GType
gegl_param_int32_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                         NULL,
        (GClassInitFunc) gegl_param_int32_class_init,
        NULL,                                         NULL,
        sizeof (GeglParamSpecInt32),
        0,
        (GInstanceInitFunc) gegl_param_int32_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GeglParamInt32", &info, 0);
    }

  return type;
}

static void
gegl_param_int32_class_init (GParamSpecClass *klass)
{
  klass->value_type = GEGL_TYPE_INT32;
}

static void
gegl_param_int32_init (GParamSpec *pspec)
{
}

GParamSpec *
gegl_param_spec_int32 (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gint         minimum,
                       gint         maximum,
                       gint         default_value,
                       GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (minimum >= G_MININT32, NULL);
  g_return_val_if_fail (maximum <= G_MAXINT32, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_INT32,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GEGL_TYPE_INT16
 */

GType
gegl_int16_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GeglInt16", &info, 0);
    }

  return type;
}


/*
 * GEGL_TYPE_PARAM_INT16
 */

static void   gegl_param_int16_class_init (GParamSpecClass *klass);
static void   gegl_param_int16_init (GParamSpec *pspec);

GType
gegl_param_int16_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                         NULL,
        (GClassInitFunc) gegl_param_int16_class_init,
        NULL,                                         NULL,
        sizeof (GeglParamSpecInt16),
        0,
        (GInstanceInitFunc) gegl_param_int16_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GeglParamInt16", &info, 0);
    }

  return type;
}

static void
gegl_param_int16_class_init (GParamSpecClass *klass)
{
  klass->value_type = GEGL_TYPE_INT16;
}

static void
gegl_param_int16_init (GParamSpec *pspec)
{
}

GParamSpec *
gegl_param_spec_int16 (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gint         minimum,
                       gint         maximum,
                       gint         default_value,
                       GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (minimum >= G_MININT16, NULL);
  g_return_val_if_fail (maximum <= G_MAXINT16, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_INT16,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GEGL_TYPE_INT8
 */

GType
gegl_int8_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_UINT, "GeglInt8", &info, 0);
    }

  return type;
}


/*
 * GEGL_TYPE_PARAM_INT8
 */

static void   gegl_param_int8_class_init (GParamSpecClass *klass);
static void   gegl_param_int8_init (GParamSpec *pspec);

GType
gegl_param_int8_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                        NULL,
        (GClassInitFunc) gegl_param_int8_class_init,
        NULL,                                        NULL,
        sizeof (GeglParamSpecInt8),
        0,
        (GInstanceInitFunc) gegl_param_int8_init
      };

      type = g_type_register_static (G_TYPE_PARAM_UINT,
                                     "GeglParamInt8", &info, 0);
    }

  return type;
}

static void
gegl_param_int8_class_init (GParamSpecClass *klass)
{
  klass->value_type = GEGL_TYPE_INT8;
}

static void
gegl_param_int8_init (GParamSpec *pspec)
{
}

GParamSpec *
gegl_param_spec_int8 (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      guint        minimum,
                      guint        maximum,
                      guint        default_value,
                      GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (maximum <= G_MAXUINT8, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_INT8,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GEGL_TYPE_PARAM_STRING
 */

static void       gegl_param_string_class_init (GParamSpecClass *klass);
static void       gegl_param_string_init (GParamSpec *pspec);
static gboolean   gegl_param_string_validate (GParamSpec *pspec,
                                              GValue     *value);

GType
gegl_param_string_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                          NULL,
        (GClassInitFunc) gegl_param_string_class_init,
        NULL,                                          NULL,
        sizeof (GeglParamSpecString),
        0,
        (GInstanceInitFunc) gegl_param_string_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "GeglParamString", &info, 0);
    }

  return type;
}

static void
gegl_param_string_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_STRING;
  klass->value_validate = gegl_param_string_validate;
}

static void
gegl_param_string_init (GParamSpec *pspec)
{
  GeglParamSpecString *sspec = GEGL_PARAM_SPEC_STRING (pspec);

  sspec->no_validate = FALSE;
  sspec->null_ok     = FALSE;
}

static gboolean
gegl_param_string_validate (GParamSpec *pspec,
                            GValue     *value)
{
  GeglParamSpecString *sspec  = GEGL_PARAM_SPEC_STRING (pspec);
  gchar               *string = value->data[0].v_pointer;

  if (string)
    {
      gchar *s;

      if (!sspec->no_validate &&
          !g_utf8_validate (string, -1, (const gchar **) &s))
        {
          for (; *s; s++)
            if (*s < ' ')
              *s = '?';

          return TRUE;
        }
    }
  else if (!sspec->null_ok)
    {
      value->data[0].v_pointer = g_strdup ("");
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gegl_param_spec_string (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        gboolean     no_validate,
                        gboolean     null_ok,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  GeglParamSpecString *sspec;

  sspec = g_param_spec_internal (GEGL_TYPE_PARAM_STRING,
                                 name, nick, blurb, flags);

  if (sspec)
    {
      g_free (G_PARAM_SPEC_STRING (sspec)->default_value);
      G_PARAM_SPEC_STRING (sspec)->default_value = g_strdup (default_value);

      sspec->no_validate = no_validate ? TRUE : FALSE;
      sspec->null_ok     = null_ok     ? TRUE : FALSE;
    }

  return G_PARAM_SPEC (sspec);
}

/*
 * GEGL_TYPE_PARAM_FILE_PATH
 */

static void       gegl_param_file_path_class_init (GParamSpecClass *klass);
static void       gegl_param_file_path_init       (GParamSpec *pspec);
static gboolean   gegl_param_file_path_validate   (GParamSpec *pspec,
                                                   GValue     *value);

GType
gegl_param_file_path_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                        NULL,
        (GClassInitFunc) gegl_param_file_path_class_init,
        NULL,                                        NULL,
        sizeof (GeglParamSpecString),
        0,
        (GInstanceInitFunc) gegl_param_file_path_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "GeglParamFilePath", &info, 0);
    }

  return type;
}

static void
gegl_param_file_path_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_STRING;
  klass->value_validate = gegl_param_file_path_validate;
}

static void
gegl_param_file_path_init (GParamSpec *pspec)
{
  GeglParamSpecFilePath *sspec = GEGL_PARAM_SPEC_FILE_PATH (pspec);

  sspec->no_validate = FALSE;
  sspec->null_ok     = FALSE;
}

static gboolean
gegl_param_file_path_validate (GParamSpec *pspec,
                               GValue     *value)
{
  GeglParamSpecFilePath *sspec = GEGL_PARAM_SPEC_FILE_PATH (pspec);
  gchar                 *path  = value->data[0].v_pointer;

  if (path)
    {
      gchar *s;

      if (!sspec->no_validate &&
          !g_utf8_validate (path, -1, (const gchar **) &s))
        {
          for (; *s; s++)
            if (*s < ' ')
              *s = '?';

          return TRUE;
        }
    }
  else if (!sspec->null_ok)
    {
      value->data[0].v_pointer = g_strdup ("");
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gegl_param_spec_file_path (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           gboolean     no_validate,
                           gboolean     null_ok,
                           const gchar *default_value,
                           GParamFlags  flags)
{
  GeglParamSpecFilePath *sspec;

  sspec = g_param_spec_internal (GEGL_TYPE_PARAM_FILE_PATH,
                                 name, nick, blurb, flags);

  if (sspec)
    {
      g_free (G_PARAM_SPEC_STRING (sspec)->default_value);
      G_PARAM_SPEC_STRING (sspec)->default_value = g_strdup (default_value);

      sspec->no_validate = no_validate ? TRUE : FALSE;
      sspec->null_ok     = null_ok     ? TRUE : FALSE;
    }

  return G_PARAM_SPEC (sspec);
}


/*
 * GEGL_TYPE_PARAM_MULTILINE
 */

static void       gegl_param_multiline_class_init (GParamSpecClass *klass);
static void       gegl_param_multiline_init (GParamSpec *pspec);
static gboolean   gegl_param_multiline_validate (GParamSpec *pspec,
                                                 GValue     *value);

GType
gegl_param_multiline_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                             NULL,
        (GClassInitFunc) gegl_param_multiline_class_init,
        NULL,                                             NULL,
        sizeof (GeglParamSpecString),
        0,
        (GInstanceInitFunc) gegl_param_multiline_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "GeglParamMultiline", &info, 0);
    }

  return type;
}

static void
gegl_param_multiline_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_STRING;
  klass->value_validate = gegl_param_multiline_validate;
}

static void
gegl_param_multiline_init (GParamSpec *pspec)
{
  GeglParamSpecMultiline *sspec = GEGL_PARAM_SPEC_MULTILINE (pspec);

  sspec->no_validate = FALSE;
  sspec->null_ok     = FALSE;
}

static gboolean
gegl_param_multiline_validate (GParamSpec *pspec,
                               GValue     *value)
{
  GeglParamSpecMultiline *sspec     = GEGL_PARAM_SPEC_MULTILINE (pspec);
  gchar                  *multiline = value->data[0].v_pointer;

  if (multiline)
    {
      gchar *s;

      if (!sspec->no_validate &&
          !g_utf8_validate (multiline, -1, (const gchar **) &s))
        {
          for (; *s; s++)
            if (*s < ' ')
              *s = '?';

          return TRUE;
        }
    }
  else if (!sspec->null_ok)
    {
      value->data[0].v_pointer = g_strdup ("");
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gegl_param_spec_multiline (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           const gchar *default_value,
                           GParamFlags  flags)
{
  GeglParamSpecMultiline *sspec;

  sspec = g_param_spec_internal (GEGL_TYPE_PARAM_MULTILINE,
                                 name, nick, blurb, flags);

  if (sspec)
    {
      g_free (G_PARAM_SPEC_STRING (sspec)->default_value);
      G_PARAM_SPEC_STRING (sspec)->default_value = g_strdup (default_value);
/*
      sspec->no_validate = no_validate ? TRUE : FALSE;
      sspec->null_ok     = null_ok     ? TRUE : FALSE;*/
    }

  return G_PARAM_SPEC (sspec);
}




/*
 * GEGL_TYPE_PARAM_ENUM
 */

static void       gegl_param_enum_class_init (GParamSpecClass *klass);
static void       gegl_param_enum_init       (GParamSpec      *pspec);
static void       gegl_param_enum_finalize   (GParamSpec      *pspec);
static gboolean   gegl_param_enum_validate   (GParamSpec      *pspec,
                                              GValue          *value);

GType
gegl_param_enum_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL,                                        NULL,
        (GClassInitFunc) gegl_param_enum_class_init,
        NULL,                                        NULL,
        sizeof (GeglParamSpecEnum),
        0,
        (GInstanceInitFunc) gegl_param_enum_init
      };

      type = g_type_register_static (G_TYPE_PARAM_ENUM,
                                     "GeglParamEnum", &info, 0);
    }

  return type;
}

static void
gegl_param_enum_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_ENUM;
  klass->finalize       = gegl_param_enum_finalize;
  klass->value_validate = gegl_param_enum_validate;
}

static void
gegl_param_enum_init (GParamSpec *pspec)
{
  GeglParamSpecEnum *espec = GEGL_PARAM_SPEC_ENUM (pspec);

  espec->excluded_values = NULL;
}

static void
gegl_param_enum_finalize (GParamSpec *pspec)
{
  GeglParamSpecEnum *espec = GEGL_PARAM_SPEC_ENUM (pspec);
  GParamSpecClass   *parent_class;

  parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_ENUM));

  g_slist_free (espec->excluded_values);

  parent_class->finalize (pspec);
}

static gboolean
gegl_param_enum_validate (GParamSpec *pspec,
                          GValue     *value)
{
  GeglParamSpecEnum *espec = GEGL_PARAM_SPEC_ENUM (pspec);
  GParamSpecClass   *parent_class;
  GSList            *list;

  parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_ENUM));

  if (parent_class->value_validate (pspec, value))
    return TRUE;

  for (list = espec->excluded_values; list; list = g_slist_next (list))
    {
      if (GPOINTER_TO_INT (list->data) == value->data[0].v_long)
        {
          value->data[0].v_long = G_PARAM_SPEC_ENUM (pspec)->default_value;
          return TRUE;
        }
    }

  return FALSE;
}

GParamSpec *
gegl_param_spec_enum (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      GType        enum_type,
                      gint         default_value,
                      GParamFlags  flags)
{
  GeglParamSpecEnum *espec;
  GEnumClass        *enum_class;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  g_return_val_if_fail (g_enum_get_value (enum_class, default_value) != NULL,
                        NULL);

  espec = g_param_spec_internal (GEGL_TYPE_PARAM_ENUM,
                                 name, nick, blurb, flags);

  G_PARAM_SPEC_ENUM (espec)->enum_class    = enum_class;
  G_PARAM_SPEC_ENUM (espec)->default_value = default_value;
  G_PARAM_SPEC (espec)->value_type         = enum_type;

  return G_PARAM_SPEC (espec);
}

void
gegl_param_spec_enum_exclude_value (GeglParamSpecEnum *espec,
                                    gint               value)
{
  g_return_if_fail (GEGL_IS_PARAM_SPEC_ENUM (espec));
  g_return_if_fail (g_enum_get_value (G_PARAM_SPEC_ENUM (espec)->enum_class,
                                      value) != NULL);

  espec->excluded_values = g_slist_prepend (espec->excluded_values,
                                            GINT_TO_POINTER (value));
}
