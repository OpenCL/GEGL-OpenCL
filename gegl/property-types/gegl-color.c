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
 * Copyright 2006 Martin Nordholts <enselic@hotmail.com>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-color.h"


enum
{
  PROP_0,
  PROP_STRING
};

typedef struct _GeglColorPrivate GeglColorPrivate;
typedef struct _ColorNameEntity ColorNameEntity;

struct _GeglColorPrivate
{
  gfloat rgba_color[4];
};

struct _ColorNameEntity
{
  const gchar *color_name;
  const gfloat rgba_color[4];
};

static gboolean  parse_float_argument_list (GeglColor *color,
                                            GScanner  *scanner,
                                            gint       num_arguments);
static gboolean  parse_color_name (GeglColor   *color,
                                   const gchar *color_string);
static gboolean  parse_hex (GeglColor   *color,
                            const gchar *color_string);
static void      set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec);
static void      get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec);

/* These color names are based on those defined in the HTML 4.01 standard. See
 * http://www.w3.org/TR/html4/types.html#h-6.5
 */
static const ColorNameEntity color_names[] =
{
  { "black",   { 0.f,      0.f,      0.f,      1.f } },
  { "silver",  { 0.75294f, 0.75294f, 0.75294f, 1.f } },
  { "gray",    { 0.50196f, 0.50196f, 0.50196f, 1.f } },
  { "white",   { 1.f,      1.f,      1.f,      1.f } },
  { "maroon",  { 0.50196f, 0.f,      0.f,      1.f } },
  { "red",     { 1.f,      0.f,      0.f,      1.f } },
  { "purple",  { 0.50196f, 0.f,      0.50196f, 1.f } },
  { "fuchsia", { 1.f,      0.f,      1.f,      1.f } },
  { "green",   { 0.f,      0.50196f, 0.f,      1.f } },
  { "lime",    { 0.f,      1.f,      0.f,      1.f } },
  { "olive",   { 0.50196f, 0.50196f, 0.f,      1.f } },
  { "yellow",  { 1.f,      1.f,      01.f,     1.f } },
  { "navy",    { 0.f,      0.f,      0.50196f, 1.f } },
  { "blue",    { 0.f,      0.f,      1.f,      1.f } },
  { "teal",    { 0.f,      0.50196f, 0.50196f, 1.f } },
  { "aqua",    { 0.f,      1.f,      1.f,      1.f } },
  { "none",    { 0.f,      0.f,      0.f,      0.f } }
};

/* Copied into GeglColor:s instances when parsing a color from a string fails. */
static const gfloat parsing_error_color[4] = { 0.f, 1.f, 1.f, 0.67f };

/* Copied into all GeglColor:s at their instantiation. */
static const gfloat init_color[4] = { 1.f, 1.f, 1.f, 1.f };

G_DEFINE_TYPE (GeglColor, gegl_color, G_TYPE_OBJECT)

#define GEGL_COLOR_GET_PRIVATE(o)    (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEGL_TYPE_COLOR, GeglColorPrivate))

static void
gegl_color_init (GeglColor *self)
{
  GeglColorPrivate *priv = GEGL_COLOR_GET_PRIVATE (self);

  memcpy (priv->rgba_color, init_color, sizeof (init_color));
}

static void
gegl_color_class_init (GeglColorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_STRING,
                                   g_param_spec_string ("string",
                                                        "String",
                                                        "A String representation of the GeglColor",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GeglColorPrivate));
}

static gboolean
parse_float_argument_list (GeglColor *color,
                           GScanner  *scanner,
                           gint       num_arguments)
{
  GTokenType        token_type;
  GTokenValue       token_value;
  GeglColorPrivate *priv;
  gint              i;

  priv = GEGL_COLOR_GET_PRIVATE (color);

  /* Make sure there is a leading '(' */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_LEFT_PAREN)
    {
      return FALSE;
    }

  /* Iterate through the arguments and copy each value
   * to the rgba_color array of GeglColor.
   */
  for (i = 0; i < num_arguments; ++i)
    {
      if (g_scanner_get_next_token (scanner) != G_TOKEN_FLOAT)
        {
          return FALSE;
        }

      token_value         = g_scanner_cur_value (scanner);
      priv->rgba_color[i] = token_value.v_float;

      /* Verify that there is a ',' after each float, except the last one */
      if (i < (num_arguments - 1))
        {
          token_type = g_scanner_get_next_token (scanner);
          if (token_type != G_TOKEN_COMMA)
            {
              return FALSE;
            }
        }
    }

  /* Make sure there is a traling ')' and that that is the last token. */
  if (g_scanner_get_next_token (scanner) == G_TOKEN_RIGHT_PAREN &&
      g_scanner_get_next_token (scanner) == G_TOKEN_EOF)
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_color_name (GeglColor   *color,
                  const gchar *color_string)
{
  GeglColorPrivate *priv = GEGL_COLOR_GET_PRIVATE (color);

  gint              num_color_names = sizeof (color_names) / sizeof (color_names[0]);
  gint              i;

  for (i = 0; i < num_color_names; ++i)
    {
      if (g_ascii_strcasecmp (color_names[i].color_name, color_string) == 0)
        {
          memcpy (priv->rgba_color, color_names[i].rgba_color, sizeof (color_names[i].rgba_color));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
parse_hex (GeglColor   *color,
           const gchar *color_string)
{
  gint              i;
  gsize             string_length = strlen (color_string);
  GeglColorPrivate *priv          = GEGL_COLOR_GET_PRIVATE (color);

  if (string_length == 7 ||  /* #rrggbb   */
      string_length == 9)    /* #rrggbbaa */
    {
      gint num_iterations = (string_length - 1) / 2;
      for (i = 0; i < num_iterations; ++i)
        {
          if (g_ascii_isxdigit (color_string[2 * i + 1]) &&
              g_ascii_isxdigit (color_string[2 * i + 2]))
            {
              priv->rgba_color[i] = (g_ascii_xdigit_value (color_string[2 * i + 1]) << 4 |
                                     g_ascii_xdigit_value (color_string[2 * i + 2])) / 255.f;
            }
          else
            {
              return FALSE;
            }
        }

      /* Successful #rrggbb(aa) parsing! */
      return TRUE;
    }
  else if (string_length == 4 ||  /* #rgb  */
           string_length == 5)    /* #rgba */
    {
      gint num_iterations = string_length - 1;
      for (i = 0; i < num_iterations; ++i)
        {
          if (g_ascii_isxdigit (color_string[i + 1]))
            {
              priv->rgba_color[i] = (g_ascii_xdigit_value (color_string[i + 1]) << 4 |
                                     g_ascii_xdigit_value (color_string[i + 1])) / 255.f;
            }
          else
            {
              return FALSE;
            }
        }

      /* Successful #rgb(a) parsing! */
      return TRUE;
    }

  /* String was of unsupported length. */
  return FALSE;
}

#if 0
const gfloat *
gegl_color_float4 (GeglColor *self)
{
  GeglColorPrivate *priv;
  g_return_val_if_fail (GEGL_IS_COLOR (self), NULL);
  priv = GEGL_COLOR_GET_PRIVATE (self);
  return &priv->rgba_color[0];
}
#endif

void
gegl_color_get_rgba4f (GeglColor   *color,
                       gfloat      *rgba)
{
  GeglColorPrivate *priv;
  gint i;

  g_return_if_fail (GEGL_IS_COLOR (color));

  priv = GEGL_COLOR_GET_PRIVATE (color);

  for (i=0; i< 4; i++)
    rgba[i] = priv->rgba_color[i];
}

void
gegl_color_set_rgba (GeglColor *self,
                     gdouble    r,
                     gdouble    g,
                     gdouble    b,
                     gdouble    a)
{
  GeglColorPrivate *priv;

  g_return_if_fail (GEGL_IS_COLOR (self));

  priv = GEGL_COLOR_GET_PRIVATE (self);

  priv->rgba_color[0] = r;
  priv->rgba_color[1] = g;
  priv->rgba_color[2] = b;
  priv->rgba_color[3] = a;
}

void
gegl_color_get_rgba (GeglColor *self,
                     gdouble   *r,
                     gdouble   *g,
                     gdouble   *b,
                     gdouble   *a)
{
  GeglColorPrivate *priv;

  g_return_if_fail (GEGL_IS_COLOR (self));

  priv = GEGL_COLOR_GET_PRIVATE (self);

  *r = priv->rgba_color[0];
  *g = priv->rgba_color[1];
  *b = priv->rgba_color[2];
  *a = priv->rgba_color[3];
}

static void
gegl_color_set_from_string (GeglColor   *self,
                            const gchar *color_string)
{
  GScanner         *scanner;
  GTokenType        token_type;
  GTokenValue       token_value;
  gboolean          color_parsing_successfull;
  GeglColorPrivate *priv;

  scanner                               = g_scanner_new (NULL);
  scanner->config->cpair_comment_single = "";
  g_scanner_input_text (scanner, color_string, strlen (color_string));

  priv = GEGL_COLOR_GET_PRIVATE (self);

  token_type  = g_scanner_get_next_token (scanner);
  token_value = g_scanner_cur_value (scanner);

  if (token_type == G_TOKEN_IDENTIFIER &&
      g_ascii_strcasecmp (token_value.v_identifier, "rgb") == 0)
    {
      color_parsing_successfull = parse_float_argument_list (self, scanner, 3);
      priv->rgba_color[3]       = 1.f;
    }
  else if (token_type == G_TOKEN_IDENTIFIER &&
           g_ascii_strcasecmp (token_value.v_identifier, "rgba") == 0)
    {
      color_parsing_successfull = parse_float_argument_list (self, scanner, 4);
    }
  else if (token_type == '#') /* FIXME: Verify that this is a safe way to check for '#' */
    {
      color_parsing_successfull = parse_hex (self, color_string);
    }
  else if (token_type == G_TOKEN_IDENTIFIER)
    {
      color_parsing_successfull = parse_color_name (self, color_string);
    }
  else
    {
      color_parsing_successfull = FALSE;
    }

  if (!color_parsing_successfull)
    {
      memcpy (priv->rgba_color,
              parsing_error_color,
              sizeof (parsing_error_color));
      g_warning ("Parsing of color string \"%s\" into GeglColor failed! "
                 "Using transparent cyan instead",
                 color_string);
    }

  g_scanner_destroy (scanner);
}

static gchar *
gegl_color_get_string (GeglColor *color)
{
  GeglColorPrivate *priv = GEGL_COLOR_GET_PRIVATE (color);

  if (priv->rgba_color[3] == 1.0)
    {
      gchar buf [3][G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[0]);
      g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[1]);
      g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[2]);
      return g_strdup_printf ("rgb(%s, %s, %s)", buf[0], buf[1], buf[2]);
    }
  else
    {
      gchar buf [4][G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[0]);
      g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[1]);
      g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[2]);
      g_ascii_formatd (buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%1.4f", priv->rgba_color[3]);
      return g_strdup_printf ("rgba(%s, %s, %s, %s)", buf[0], buf[1], buf[2], buf[3]);
    }
}


static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColor *color = GEGL_COLOR (gobject);

  switch (property_id)
    {
      case PROP_STRING:
        gegl_color_set_from_string (color, g_value_get_string (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglColor *color = GEGL_COLOR (gobject);

  switch (property_id)
    {
      case PROP_STRING:
      {
        gchar *string = gegl_color_get_string (color);
        g_value_set_string (value, string);
        g_free (string);
      }
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

GeglColor *
gegl_color_new (const gchar *string)
{
  if (string)
    return g_object_new (GEGL_TYPE_COLOR, "string", string, NULL);

  return g_object_new (GEGL_TYPE_COLOR, NULL);
}

/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglColor as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_COLOR, GeglParamColor))
#define GEGL_IS_PARAM_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_COLOR))

typedef struct _GeglParamColor GeglParamColor;

struct _GeglParamColor
{
  GParamSpec parent_instance;

  GeglColor *default_color;
};

static void
gegl_param_color_init (GParamSpec *self)
{
  GEGL_PARAM_COLOR (self)->default_color = NULL;
}

GeglColor *
gegl_param_spec_color_get_default (GParamSpec *self);
GeglColor *
gegl_param_spec_color_get_default (GParamSpec *self)
{
  return GEGL_PARAM_COLOR (self)->default_color;
}

static void
gegl_param_color_finalize (GParamSpec *self)
{
  GeglParamColor  *param_color  = GEGL_PARAM_COLOR (self);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_COLOR));

  if (param_color->default_color)
    {
      g_object_unref (param_color->default_color);
      param_color->default_color = NULL;
    }

  parent_class->finalize (self);
}

#if 0
static void
value_set_default (GParamSpec *param_spec,
                   GValue     *value)
{
  GeglParamColor *gegl_color = GEGL_PARAM_COLOR (param_spec);

  g_value_set_object (value, gegl_color->default_color);
}
#endif

GType
gegl_param_color_get_type (void)
{
  static GType param_color_type = 0;

  if (G_UNLIKELY (param_color_type == 0))
    {
      static GParamSpecTypeInfo param_color_type_info = {
        sizeof (GeglParamColor),
        0,
        gegl_param_color_init,
        0,
        gegl_param_color_finalize,
        NULL, /*value_set_default,*/
        NULL,
        NULL
      };
      param_color_type_info.value_type = GEGL_TYPE_COLOR;

      param_color_type = g_param_type_register_static ("GeglParamColor",
                                                       &param_color_type_info);
    }

  return param_color_type;
}

GParamSpec *
gegl_param_spec_color (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GeglColor   *default_color,
                       GParamFlags  flags)
{
  GeglParamColor *param_color;

  param_color = g_param_spec_internal (GEGL_TYPE_PARAM_COLOR,
                                       name, nick, blurb, flags);
/*
  param_color->default_color = default_color;
  if (default_color)
    g_object_ref (default_color);*/

  return G_PARAM_SPEC (param_color);
}

GParamSpec *
gegl_param_spec_color_from_string (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   const gchar *default_color_string,
                                   GParamFlags  flags)
{
  GeglParamColor *param_color;

  param_color = g_param_spec_internal (GEGL_TYPE_PARAM_COLOR,
                                       name, nick, blurb, flags);
/*
  param_color->default_color = g_object_new (GEGL_TYPE_COLOR,
                                             "string", default_color_string,
                                             NULL);*/
  return G_PARAM_SPEC (param_color);
}
