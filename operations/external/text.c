/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_string (string, _("Text"), "Hello")
    description(_("String to display (utf8)"))
    ui_meta    ("multiline", "true")

property_string (font, _("Font family"), "Sans")
    description(_("Font family (utf8)"))

property_double (size, _("Size"), 10.0)
    description (_("Font size in pixels."))
    value_range (0.0, 2048.0)

property_color  (color, _("Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
    description(_("Color for the text (defaults to 'black')"))

property_int  (wrap, _("Wrap width"), -1)
    description (_("Sets the width in pixels at which long lines will wrap. "
                     "Use -1 for no wrapping."))
    value_range (-1, 1000000)
property_int  (vertical_wrap, _("Wrap height"), 0)
    description (_("Sets the height in pixels according to which the text is "
                   "vertically justified. "
                   "Use -1 for no vertical justification."))
    value_range (-1, 1000000)

property_int    (alignment, _("Justification"), 0)
    value_range (0, 2)
    description (_("Alignment for multi-line text (0=Left, 1=Center, 2=Right)"))

property_int    (vertical_alignment, _("Vertical justification"), 0)
    value_range (0, 2)
    description (_("Vertical text alignment (0=Top, 1=Middle, 2=Bottom)"))

property_int (width, _("Width"), 0)
    description (_("Rendered width in pixels. (read only)"))
property_int    (height, _("Height"), 0)
    description (_("Rendered height in pixels. (read only)"))

#else

#include <gegl-plugin.h>
#include <cairo.h>
#include <pango/pango-attributes.h>
#include <pango/pangocairo.h>

/* XXX: this struct is unneeded and could be folded directly into
 * struct _GeglOp
 */
typedef struct {
  gchar         *string;
  gchar         *font;
  gdouble        size;
  gint           wrap;
  gint           vertical_wrap;
  gint           alignment;
  gint           vertical_alignment;
  GeglRectangle  defined;
} CachedExtent;

struct _GeglOp
{
  GeglOperationSource parent_instance;
  gpointer            properties;
  CachedExtent        cex;
};

typedef struct
{
  GeglOperationSourceClass parent_class;
} GeglOpClass;

#define GEGL_OP_NAME     text
#define GEGL_OP_C_SOURCE text.c
#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION (GEGL_TYPE_OPERATION_SOURCE)





static void text_layout_text (GeglOp        *self,
                              cairo_t       *cr,
                              gdouble        rowstride,
                              GeglRectangle *bounds)
{
  GeglProperties       *o = GEGL_PROPERTIES (self);
  PangoFontDescription *desc;
  PangoLayout          *layout;
  PangoAttrList        *attrs;
  guint16               color[4];
  gchar                *string;
  gint                  alignment = 0;
  PangoRectangle        ink_rect;
  PangoRectangle        logical_rect;
  gint                  vertical_offset = 0;

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  string = g_strcompress (o->string);
  pango_layout_set_text (layout, string, -1);
  g_free (string);

  desc = pango_font_description_from_string (o->font);
  pango_font_description_set_absolute_size (desc, o->size * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);

  switch (o->alignment)
  {
  case 0:
    alignment = PANGO_ALIGN_LEFT;
    break;
  case 1:
    alignment = PANGO_ALIGN_CENTER;
    break;
  case 2:
    alignment = PANGO_ALIGN_RIGHT;
    break;
  }
  pango_layout_set_alignment (layout, alignment);
  pango_layout_set_width (layout, o->wrap * PANGO_SCALE);

  attrs = pango_attr_list_new ();

  gegl_color_get_pixel (o->color, babl_format ("R'G'B'A u16"), color);
  pango_attr_list_insert (
    attrs,
    pango_attr_foreground_new (color[0], color[1], color[2]));
  pango_attr_list_insert (
    attrs,
    pango_attr_foreground_alpha_new (color[3]));

  pango_layout_set_attributes (layout, attrs);

  /* Inform Pango to re-layout the text with the new transformation */
  pango_cairo_update_layout (cr, layout);

  pango_layout_get_pixel_extents (layout, &ink_rect, &logical_rect);
  if (o->vertical_wrap >= 0)
    {
      switch (o->vertical_alignment)
      {
      case 0: /* top */
        vertical_offset = 0;
        break;
      case 1: /* middle */
        vertical_offset = (o->vertical_wrap - logical_rect.height) / 2;
        break;
      case 2: /* bottom */
        vertical_offset = o->vertical_wrap - logical_rect.height;
        break;
      }
    }

  if (bounds)
    {
      *bounds = *GEGL_RECTANGLE (ink_rect.x,     ink_rect.y + vertical_offset,
                                 ink_rect.width, ink_rect.height);
    }
  else
    {
      /* When alpha is 0, Pango goes full alpha (by design).  Go figure... */
      if (color[3] > 0)
        {
          cairo_translate (cr, 0, vertical_offset);

          pango_cairo_show_layout (cr, layout);
        }
    }

  pango_font_description_free (desc);
  pango_attr_list_unref (attrs);
  g_object_unref (layout);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglOp *self = GEGL_OP (operation);

  guchar          *data = g_new0 (guchar, result->width * result->height * 4);
  cairo_t         *cr;
  cairo_surface_t *surface;

  surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 result->width,
                                                 result->height,
                                                 result->width * 4);
  cr = cairo_create (surface);
  cairo_translate (cr, -result->x, -result->y);
  text_layout_text (self, cr, 0, NULL);

  gegl_buffer_set (output, result, 0, babl_format ("B'aG'aR'aA u8"), data,
                   GEGL_AUTO_ROWSTRIDE);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  g_free (data);

  return  TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);
  GeglProperties           *o = GEGL_PROPERTIES (self);
  CachedExtent *extent;
  gint status = FALSE;

  extent = (CachedExtent*)&self->cex;
  /*if (!self->priv)
    {
      self->priv = g_malloc0 (sizeof (CachedExtent));
      extent = (CachedExtent*)self->priv;
      extent->string = g_strdup ("");
      extent->font = g_strdup ("");
    }*/

  if ((extent->string && strcmp (extent->string, o->string)) ||
      (extent->font && strcmp (extent->font, o->font)) ||
      extent->size != o->size ||
      extent->wrap != o->wrap ||
      extent->alignment != o->alignment)
    { /* get extents */
      cairo_t *cr;

      cairo_surface_t *surface  = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
          1, 1);
      cr = cairo_create (surface);
      text_layout_text (self, cr, 0, &extent->defined);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);

      if (extent->string)
        g_free (extent->string);
      extent->string = g_strdup (o->string);
      if (extent->font)
        g_free (extent->font);
      extent->font = g_strdup (o->font);
      extent->size = o->size;
      extent->wrap = o->wrap;
      extent->alignment = o->alignment;

      /* store the measured size for later use */
      o->width  = extent->defined.width  - extent->defined.x;
      o->height = extent->defined.height - extent->defined.y;

      /* XXX: this invalidation is *probably* unnecessary.  having
       * get_bounding_box() cause an invalidation can be surprising,
       * so disabling it for now.
       */
      /* gegl_operation_invalidate (operation, NULL, TRUE); */
    }

  if (status)
    {
      g_warning ("get defined region for text '%s' failed", o->string);
    }

  return extent->defined;
}

static void
finalize (GObject *object)
{
  GeglOp *self = GEGL_OP (object);

  if (self->cex.string)
    g_free (self->cex.string);
  if (self->cex.font)
    g_free (self->cex.font);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

static const gchar *composition =
    "<?xml version='1.0'             encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop' width='200' height='200'/>"
    "<node operation='gegl:text'>"
    "  <params>"
    "    <param name='size'>20</param>"
    "    <param name='wrap'>200</param>"
    "    <param name='color'>green</param>"
    "    <param name='string'>loves or pursues or desires to obtain pain of itself, because it is pain, but occasionally circumstances occur in which toil and pain can procure him some great pleasure. To take a trivial example, which of us ever undertakes laborious physical exercise, except to obtain some advantage from it? But who has any right to find fault with a man who chooses to enjoy a pleasure that has no annoying consequences, or one who avoids a pain that produces no</param>"
    "  </params>"
    "</node>"
    "</gegl>";

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *operation_source_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->finalize = finalize;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_source_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "reference-composition", composition,
    "title",        _("Render Text"),
    "name",         "gegl:text",
    "reference-hash", "9a18672b0e75e01507dd0ec35bb46298",
    "categories",   "render",
    "description",  _("Display a string of text using pango and cairo."),
    NULL);

}


#endif
