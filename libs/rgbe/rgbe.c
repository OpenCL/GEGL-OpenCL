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
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */

#include "config.h"

#include "rgbe.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


/* Scanlines are limited to 2^(16 - 1), as RLE encoded lines only have the
 * lower 15 bits of the R&G components to store its length.
 */
#define RGBE_MAX_SCANLINE_WIDTH    (1 << 15)

#define RGBE_MAX_SOFTWARE_LEN      63
#define RGBE_NUM_RGB               3
#define RGBE_NUM_RGBE              4
#define RGBE_MAX_VARIABLE_LINE_LEN 24


/* Describes the colour space of the pixel components. These values must
 * index correctly into the array RGBE_FORMAT_STRINGS for correct operation.
 */
typedef enum
{
  FORMAT_RGBE,
  FORMAT_XYZE,
  FORMAT_UNKNOWN,

  NUM_RGBE_FORMATS = FORMAT_XYZE
} rgbe_format;

/* Describes the behaviour of indices across, and between scanlines. */
typedef enum
{
  ORIENT_DECREASING,
  ORIENT_INCREASING,
  ORIENT_UNKNOWN
} rgbe_orientation;

enum
{
  OFFSET_R = 0, OFFSET_X = OFFSET_R,
  OFFSET_G = 1, OFFSET_Y = OFFSET_G,
  OFFSET_B = 2, OFFSET_Z = OFFSET_B,
  OFFSET_E = 3,
  OFFSET_A = 3
};

typedef struct
{
    rgbe_orientation orient;
    guint16          size;
} rgbe_axis;

typedef struct
{
  rgbe_format   format;

  gchar         software[RGBE_MAX_SOFTWARE_LEN + 1];

  gfloat        exposure;
  gfloat        colorcorr[RGBE_NUM_RGB];
  /* TODO: xyz primaries   */

  /* TODO: view parameters */

  /* resolution parameters */
  rgbe_axis     x_axis,
                y_axis;
  gfloat        pixel_aspect;
} rgbe_header;

struct _rgbe_file
{
  rgbe_header  header;

  GMappedFile *file;
  /* Stores the address of the scanlines, or NULL */
  const void  *scanlines;
};


static const gchar RADIANCE_MAGIC[] = "#?RADIANCE";

static const gchar *RGBE_FORMAT_STRINGS[] =
{
  "32-bit_rle_rgbe",
  "32-bit_rle_xyze",
  NULL
};


/**
 * rgbe_mapped_file_remaining:
 * @f:    the file to read the image data from
 * @data: the current file read cursor
 *
 * Calculates the number of bytes remaining to be read in a mapped file.
 **/
static guint
rgbe_mapped_file_remaining (GMappedFile *f,
                            const void  *data)
{
  g_return_val_if_fail (f, 0);
  g_return_val_if_fail (GPOINTER_TO_UINT (data) >
                        GPOINTER_TO_UINT (g_mapped_file_get_contents (f)), 0);

  return GPOINTER_TO_UINT (data) -
         GPOINTER_TO_UINT (g_mapped_file_get_contents (f)) -
         g_mapped_file_get_length (f);
}

static void
rgbe_header_init (rgbe_header *header)
{
  g_return_if_fail (header);

  header->format              = FORMAT_UNKNOWN;
  memset (header->software, '\0', G_N_ELEMENTS (header->software));

  header->exposure            = 1.0;
  header->colorcorr[OFFSET_R] = 1.0;
  header->colorcorr[OFFSET_G] = 1.0;
  header->colorcorr[OFFSET_B] = 1.0;

  header->pixel_aspect        = 1.0;

  header->x_axis.orient = header->y_axis.orient = ORIENT_UNKNOWN;
  header->x_axis.size   = header->y_axis.size   = 0;
}

static gboolean
rgbe_file_init (rgbe_file   *file,
                const gchar *path)
{
  g_return_val_if_fail (file != NULL, FALSE);

  rgbe_header_init (&file->header);
  file->file      = g_mapped_file_new (path, FALSE, NULL);
  file->scanlines = NULL;

  return file->file != NULL;
}

static rgbe_file*
rgbe_file_new (const gchar *path)
{
  rgbe_file *file;

  g_return_val_if_fail (path, NULL);

  file = g_new (rgbe_file, 1);
  if (!rgbe_file_init (file, path))
    {
      rgbe_file_free (file);
      file = NULL;
    }

  return file;
}

void
rgbe_file_free (rgbe_file *file)
{
  if (!file)
      return;

  g_mapped_file_unref (file->file);
  file->scanlines = NULL;

  g_free (file);
}

/* Parse the variable initialisations for an rgbe header. Returns the offset
 * after the header delimiting newline in cursor. The incoming cursor should
 * (most likely) be zero.
 *
 * Updates cursor on success.
 */
static gboolean
rgbe_header_read_variables (rgbe_file *file,
                            goffset   *cursor)
{
  const gchar *data;
  gboolean     success = FALSE;

  g_return_val_if_fail (file,                  FALSE);
  g_return_val_if_fail (file->file,            FALSE);
  g_return_val_if_fail (cursor && *cursor > 0, FALSE);

  data = g_mapped_file_get_contents (file->file) + *cursor;

  /* Keep iterating if it looks like there's enough data to satisfy another
   * line (the estimate doesn't need to be exact, as we can run a little over
   * due to required resolution specification which will be coming up next).
   */
  while (rgbe_mapped_file_remaining (file->file, data) > RGBE_MAX_VARIABLE_LINE_LEN)
    {
      /* Check the colourspace/type of pixels in the file */
      if (g_str_has_prefix (data, "FORMAT="))
        {
          guint i;
          data += strlen ("FORMAT=");

          file->header.format = FORMAT_UNKNOWN;
          for (i = 0; i < NUM_RGBE_FORMATS; ++i)
            {
              if (g_str_has_prefix (data, RGBE_FORMAT_STRINGS[i]))
                {
                  file->header.format = (rgbe_format)i;
                  break;
                }
            }

          if (file->header.format != FORMAT_RGBE)
            {
              g_warning ("Unsupported color format for rgbe format");
              goto cleanup;
            }
          continue;
        }

      /* Check the exposure multiplier */
      else if (g_str_has_prefix (data, "EXPOSURE="))
        {
          gdouble exposure;

          data += strlen ("EXPOSURE=");

          errno    = 0;
          exposure = g_ascii_strtod (data, NULL);
          if (errno)
            {
              g_warning ("Invalid value for exposure in radiance image file");
              goto cleanup;
            }
          else
            {
              file->header.exposure *= exposure;
            }
        }

      /* Parse the component multipliers */
      else if (g_str_has_prefix (data, "COLORCORR="))
        {
          guint i;
          data += strlen ("COLORCORR=");

          for (i = 0; i < RGBE_NUM_RGB; ++i)
            {
              gdouble multiplier;

              errno = 0;
              multiplier = g_ascii_strtod (data, (gchar**)&data);
              if (errno)
                {
                  g_warning ("Invalid value for COLORCORR");
                  goto cleanup;
                }

              file->header.colorcorr[i] *= multiplier;
            }
        }

      /* Generating software identifier */
      else if (g_str_has_prefix (data, "SOFTWARE="))
        {
          gchar * lineend;

          data    += strlen ("SOFTWARE=");
          lineend  = g_strstr_len (data,
                                   MIN (rgbe_mapped_file_remaining (file->file,
                                                                    data),
                                        G_N_ELEMENTS (file->header.software)),
                                   "\n");

          if (!lineend)
            {
              g_warning ("Cannot find a usable value for SOFTWARE, ignoring");
            }
          else
            {
              guint linesize = lineend - data;
              strncpy (file->header.software, data,
                       MIN (linesize, G_N_ELEMENTS (file->header.software) - 1));
            }
        }

      /* Ratio of pixel height to width */
      else if (g_str_has_prefix (data, "PIXASPECT="))
        {
          gdouble aspect;

          data  += strlen ("PIXASPECT=");

          errno = 0;
          aspect = g_ascii_strtod (data, (gchar **)&data);
          if (errno)
            {
              g_warning ("Invalid pixel aspect ratio");
              goto cleanup;
            }
          else
            {
              file->header.pixel_aspect *= aspect;
            }
        }

      /* We reached a blank line, so it's the end of the header */
      else if (!strncmp (data, "\n", strlen ("\n")))
        {
          data += strlen ("\n");
          *cursor = GPOINTER_TO_UINT (data) -
                    GPOINTER_TO_UINT (g_mapped_file_get_contents (file->file));
          success = TRUE;
          goto cleanup;
        }

      /* Skip past the end of the line for the next variable */
      data = g_strstr_len (data,
                           rgbe_mapped_file_remaining (file->file, data),
                           "\n");
      if (!data)
          goto cleanup;
      data += strlen ("\n");
    }

cleanup:
  return success;
}


/* Convert from '-' or '+' to useful scanline index constants
 */
static rgbe_orientation
rgbe_char_to_orientation (gchar c)
{
  switch (c)
    {
      case '-':
        return ORIENT_DECREASING;

      case '+':
        return ORIENT_INCREASING;

      default:
        return ORIENT_UNKNOWN;
    }
}


/* Return the axis which the scanline index character refers to.
 */
static rgbe_axis*
rgbe_char_to_axis (rgbe_file *file,
                   gchar      c)
{
  switch (c)
    {
      case 'y':
      case 'Y':
        return &file->header.y_axis;

      case 'x':
      case 'X':
        return &file->header.x_axis;

      default:
        return NULL;
    }
}


/* Parse the orientation/resolution line. The following format is repeated
 * twice: "[+-][XY] \d+" It specifies column or row major ordering, and the
 * direction of pixel indices (eg, mirrored).
 *
 * Updates cursor on success.
 */
static gboolean
rgbe_header_read_orientation (rgbe_file *file,
                              goffset   *cursor)
{
  const gchar      *data;
  rgbe_orientation  orient;
  rgbe_axis        *axis;
  gchar             firstaxis = '?';
  gboolean          success = FALSE;

  g_return_val_if_fail (file,                  FALSE);
  g_return_val_if_fail (file->file,            FALSE);
  g_return_val_if_fail (cursor && *cursor > 0, FALSE);

  data = g_mapped_file_get_contents (file->file) + *cursor;

  /* Read each direction, axis, and size until a newline is reached */
  do
    {
      orient = rgbe_char_to_orientation (*data++);
      if (orient == ORIENT_UNKNOWN)
          goto cleanup;

      /* Axis can be ordered with X major, which we don't currently handle */
      if (firstaxis == '?' && *data != 'Y' && *data != 'y')
          goto cleanup;
      else
          firstaxis = *data;

      axis = rgbe_char_to_axis (file, *data++);
      if (!axis)
          goto cleanup;
      axis->orient = orient;

      if (*data++ != ' ')
          goto cleanup;

      errno = 0;
      axis->size = g_ascii_strtoull (data, (gchar **)&data, 0);
      if (errno)
          goto cleanup;

  /* The termination check is simplified to a space check, as each set of
   * axis parameters are space seperated. We double check for a newline next
   * though.
   */
  } while (*data++ == ' ');

  if (data[-1] != '\n')
      goto cleanup;

  *cursor = data - g_mapped_file_get_contents (file->file);
  success = TRUE;

cleanup:
  return success;
}


/* Read each component of an rgbe file header. A pointer to the scanlines,
 * immediately after the header, is cached on success.
 */
static gboolean
rgbe_header_read (rgbe_file *file)
{
  gchar    *data;
  gboolean  success = FALSE;
  goffset   cursor  = 0;

  g_return_val_if_fail (file,                   FALSE);
  g_return_val_if_fail (file->file,             FALSE);

  rgbe_header_init (&file->header);

  data = g_mapped_file_get_contents (file->file);
  if (strncmp (&data[cursor], RADIANCE_MAGIC, strlen (RADIANCE_MAGIC)))
      goto cleanup;
  cursor += strlen (RADIANCE_MAGIC);

  if (data[cursor] != '\n')
      goto cleanup;
  ++cursor;

  if (!rgbe_header_read_variables (file, &cursor))
      goto cleanup;

  if (!rgbe_header_read_orientation (file, &cursor))
      goto cleanup;

  file->scanlines = &data[cursor];
  success = TRUE;
cleanup:
  return success;
}


/* Convert an array of gfloat mantissas to their full values. Applies the
 * exponent, exposure compensation, and color channel compensation.
 */
static void
rgbe_apply_exponent (const rgbe_file *file,
                     gfloat          *rgb,
                     gfloat           e)
{
  gfloat mult;

  g_return_if_fail (file);
  g_return_if_fail (rgb);

  if (e == 0)
    {
      rgb[OFFSET_R] = rgb[OFFSET_G] = rgb[OFFSET_B] = 0;
      goto cleanup;
    }

  mult = ldexp (1.0, e - (128 + 8));
  rgb[OFFSET_R] *= mult                  *
                   file->header.exposure *
                   file->header.colorcorr[OFFSET_R];
  rgb[OFFSET_G] *= mult                  *
                   file->header.exposure *
                   file->header.colorcorr[OFFSET_G];
  rgb[OFFSET_B] *= mult                  *
                   file->header.exposure *
                   file->header.colorcorr[OFFSET_B];
  rgb[OFFSET_A]  = 1.0f;

cleanup:
  return;
}


/* Convert an array of RGBE uints to their floating point format (applying
 * exponents and compensations as required).
 */
static void
rgbe_rgbe_to_float (const rgbe_file *file,
                    const guint8    *rgbe,
                    gfloat          *output)
{
  g_return_if_fail (file);
  g_return_if_fail (rgbe);
  g_return_if_fail (output);

  output[OFFSET_R] = rgbe[OFFSET_R];
  output[OFFSET_G] = rgbe[OFFSET_G];
  output[OFFSET_B] = rgbe[OFFSET_B];
  output[OFFSET_A] = 1.0f;

  rgbe_apply_exponent (file, output, rgbe[OFFSET_E]);
}


/* Read one uncompressed scanline row. Updates cursor on success. */
static gboolean
rgbe_read_uncompressed (const rgbe_file *file,
                        goffset         *cursor,
                        gfloat          *pixels)
{
  const guint8 *data;
  guint         i;

  g_return_val_if_fail (file,                  FALSE);
  g_return_val_if_fail (file->file,            FALSE);
  g_return_val_if_fail (cursor && *cursor > 0, FALSE);
  g_return_val_if_fail (pixels,                FALSE);

  data = (guint8 *)g_mapped_file_get_contents (file->file) + *cursor;

  for (i = 0; i < file->header.x_axis.size; ++i)
    {
      rgbe_rgbe_to_float (file, data, pixels);
      data   += RGBE_NUM_RGBE;
      pixels += RGBE_NUM_RGBE;
    }

  *cursor   = GPOINTER_TO_UINT (data) -
              GPOINTER_TO_UINT (g_mapped_file_get_contents (file->file));
  return TRUE;
}


/* Read an old style rle scanline row. Unimplemented */
static gboolean
rgbe_read_old_rle (const rgbe_file *file,
                   goffset         *cursor,
                   gfloat          *pixels)
{
  /* const gchar * data = g_mapped_file_get_contents (f) + *cursor; */

  g_return_val_if_fail (file,                  FALSE);
  g_return_val_if_fail (file->file,            FALSE);
  g_return_val_if_fail (cursor && *cursor > 0, FALSE);
  g_return_val_if_fail (pixels,                FALSE);

  g_return_val_if_reached (FALSE);
}


/* Read one new style rle scanline row. Updates cursor on success. */
static gboolean
rgbe_read_new_rle (const rgbe_file *file,
                   goffset         *cursor,
                   gfloat          *pixels)
{
  const guint8 *data;
  guint16       linesize;
  guint         i;
  guint         component;
  gfloat       *pixoffset[RGBE_NUM_RGBE] =
    {
      pixels + OFFSET_R,
      pixels + OFFSET_G,
      pixels + OFFSET_B,
      pixels + OFFSET_E
    };

  g_return_val_if_fail (file,                  FALSE);
  g_return_val_if_fail (file->file,            FALSE);
  g_return_val_if_fail (cursor && *cursor > 0, FALSE);
  g_return_val_if_fail (pixels,                FALSE);

  /* Read the scanline header: two magic bytes, and two byte pixel count. We
   * can assert on the magic as it should have been checked before
   * dispatching to this decoding routine.
   */
  data     = (guint8 *)g_mapped_file_get_contents (file->file) + *cursor;
  g_return_val_if_fail (data[OFFSET_R] == 2 && data[OFFSET_G] == 2, FALSE);
  linesize = (data[OFFSET_B] << 8) | data[OFFSET_E];

  data += RGBE_NUM_RGBE;

  /* Decode the rle/dump sequences for each color channel, continuing until
   * we've reached the expected offsets for each channel. Stores the exponent
   * values in the alpha channel temporarily.
   */
  for (component = 0; component < RGBE_NUM_RGBE; ++component)
    {
      while (pixoffset[component] < pixels + RGBE_NUM_RGBE * linesize)
        {
          const guint HIGH_BIT = (1 << 7);
          gboolean         rle = *data &  HIGH_BIT;
          guint         length = *data & ~HIGH_BIT;

          /* A dump/run of 0 is a special marker for dump 128 */
          if (length == 0)
            {
              rle    = FALSE;
              length = 128;
            }

          data++;

          /* A compressed run */
          if (rle)
            {
              for (i = 0; i < length; ++i)
                {
                  *pixoffset[component]  = *data;
                   pixoffset[component] += RGBE_NUM_RGBE;
                }

              data++;
            }
          /* A dump of values */
          else
            {
              for (i = 0; i < length; ++i)
                {
                   *pixoffset[component]  = *data;
                    pixoffset[component] += RGBE_NUM_RGBE;
                   data++;
                }
            }
        }
    }

  /* Double check we encountered as many pixels as expected. Pixoffsets should
   * have been incremented to just past the final pixel for each component.
   */
  for (component = 0; component < RGBE_NUM_RGBE; ++component)
    {
      g_warn_if_fail (pixoffset[component] == pixels + RGBE_NUM_RGBE * linesize + component);
    }

  /* Multiply the colours by the exponent. Remove 'transparency' by setting
   * alpha high as a precaution, it should be discarded in any case.
   */
  for (i = 0; i < linesize; ++i)
    {
      gfloat *pixel = pixels + i * RGBE_NUM_RGBE;
      rgbe_apply_exponent (file, pixel, pixel[OFFSET_E]);
    }

  *cursor = GPOINTER_TO_UINT (data) -
            GPOINTER_TO_UINT (g_mapped_file_get_contents (file->file));

  return TRUE;
}


/* Write a null terminated string (with user provided trailing newline) to
 * the output file, freeing the line and returning an error if needed.
 */
static gboolean
rgbe_write_line (FILE *f, gchar *line)
{
  size_t written;
  guint  len = strlen (line);

  g_return_val_if_fail (g_str_has_suffix (line, "\n"), FALSE);
  written = fwrite (line, sizeof (line[0]), len, f);
  g_free (line);

  return written == len ? TRUE : FALSE;
}


/* Write all rgbe header variables (which aren't defaults) out to a file. */
static gboolean
rgbe_header_write (const rgbe_header *header,
                   FILE              *f)
{
  gchar    *line    = NULL;
  gboolean  success = FALSE;
  gint      len;

  g_return_val_if_fail (header, FALSE);
  g_return_val_if_fail (f,      FALSE);

  /* Magic header bytes */
  line = g_strconcat (RADIANCE_MAGIC, "\n", NULL);
  if (!rgbe_write_line (f, line))
      goto cleanup;

  /* Insert the package name as the software name if not present (zero len)
   * or we don't have a null terminated name length.
   */
  len = strlen (header->software);
  if (len  == 0 || len > RGBE_MAX_SOFTWARE_LEN - 1)
    {
      line = g_strconcat ("SOFTWARE=", PACKAGE_STRING, "\n", NULL);
    }
  else
    {
      line = g_strconcat ("SOFTWARE=", header->software, "\n", NULL);
    }
  if (!rgbe_write_line (f, line))
      goto cleanup;

  /* Type of pixel components */
  g_return_val_if_fail (header->format < FORMAT_UNKNOWN,                     FALSE);
  g_return_val_if_fail (header->format < G_N_ELEMENTS (RGBE_FORMAT_STRINGS), FALSE);
  line = g_strconcat ("FORMAT=",
                      RGBE_FORMAT_STRINGS[header->format],
                      "\n", NULL);
  if (!rgbe_write_line (f, line))
      goto cleanup;

  /* Exposure compensation */
  if (header->exposure != 1.0)
    {
      gchar exp_line[G_ASCII_DTOSTR_BUF_SIZE + 1];
      line = g_strconcat ("EXPOSURE=",
                          g_ascii_dtostr (exp_line,
                                          G_N_ELEMENTS (exp_line),
                                          header->exposure),
                          "\n",
                          NULL);
      if (!rgbe_write_line (f, line))
          goto cleanup;
    }

  /* Color channel correction */
  if (header->colorcorr [OFFSET_R] != 1.0 &&
      header->colorcorr [OFFSET_G] != 1.0 &&
      header->colorcorr [OFFSET_B] != 1.0)
  {
    gchar corr_line[G_ASCII_DTOSTR_BUF_SIZE + 1][RGBE_NUM_RGB];
    line = g_strconcat ("COLORCORR=",
                        g_ascii_dtostr (corr_line[OFFSET_R],
                                        G_N_ELEMENTS (corr_line[OFFSET_R]),
                                        header->colorcorr[OFFSET_R]), " ",
                        g_ascii_dtostr (corr_line[OFFSET_G],
                                        G_N_ELEMENTS (corr_line[OFFSET_G]),
                                        header->colorcorr[OFFSET_G]), " ",
                        g_ascii_dtostr (corr_line[OFFSET_B],
                                        G_N_ELEMENTS (corr_line[OFFSET_B]),
                                        header->colorcorr[OFFSET_R]),
                        "\n",
                        NULL);
    if (!rgbe_write_line (f, line))
        goto cleanup;
  }


  /* Resolution specifier */
  {
    const guint res_line_sz = strlen ("\n")
                            + strlen ("-Y ") * 2
                            + strlen (G_STRINGIFY (RGBE_MAX_SCANLINE_WIDTH)) * 2
                            + strlen ("\n")
                            + 1;
    gint err;

    line = g_malloc (res_line_sz * sizeof (line[0]));
    err  = snprintf (line, res_line_sz,
                     "\n-Y %hu +X %hu\n",
                     header->y_axis.size,
                     header->x_axis.size);
    if (err < 0 || !rgbe_write_line (f, line))
        goto cleanup;
  }

  success = TRUE;
cleanup:
  return success;
}


/* Convert an array of floats to rgbe components for file output */
static void
rgbe_float_to_rgbe (const gfloat *f,
                    guint8       *rgbe)
{
  gint   e;
  gfloat frac, max;

  g_return_if_fail (f);
  g_return_if_fail (rgbe);

  max =           f[OFFSET_R];
  max = MAX (max, f[OFFSET_G]);
  max = MAX (max, f[OFFSET_B]);

  if (max < 1e-38)
    {
      rgbe[OFFSET_R] = rgbe[OFFSET_G] = rgbe[OFFSET_B] = 0;
      goto cleanup;
    }

  frac  = frexp (max, &e) * 256.0 / max;

  rgbe[OFFSET_R] = f[OFFSET_R] * frac;
  rgbe[OFFSET_G] = f[OFFSET_G] * frac;
  rgbe[OFFSET_B] = f[OFFSET_B] * frac;

  rgbe[OFFSET_E] = e + 128;

cleanup:
  return;
}


/* Write the first scanline from pixels into the file. Does not use RLE. */
static gboolean
rgbe_write_uncompressed (const rgbe_header *header,
                         const gfloat      *pixels,
                         FILE              *f)
{
  guint    x, y;
  guint8   rgbe[RGBE_NUM_RGBE];
  gboolean success = TRUE;

  g_return_val_if_fail (header, FALSE);
  g_return_val_if_fail (pixels, FALSE);
  g_return_val_if_fail (f,      FALSE);

  for (y = 0; y < header->y_axis.size; ++y)
      for (x = 0; x < header->x_axis.size; ++x)
        {
          rgbe_float_to_rgbe (pixels, rgbe);

          /* Ensure we haven't inadvertantly triggered an rle scanline */
          g_warn_if_fail (rgbe[0] != 2 || rgbe[1] != 2);
          g_warn_if_fail (rgbe[0] != 1 || rgbe[1] != 1 || rgbe[2] != 1);

          if (G_N_ELEMENTS (rgbe) != fwrite (rgbe, sizeof (rgbe[0]), G_N_ELEMENTS (rgbe), f))
            success = FALSE;
          pixels += RGBE_NUM_RGB;
        }

  return success;
}


gboolean
rgbe_save_path (const gchar *path,
                guint        width,
                guint        height,
                gfloat      *pixels)
{
  rgbe_header  header;
  FILE        *f       = NULL;
  gboolean     success = FALSE;

  f = (!strcmp (path, "-") ? stdout : fopen(path, "wb"));
  if (!f)
      goto cleanup;

  rgbe_header_init (&header);
  header.x_axis.orient = ORIENT_INCREASING;
  header.x_axis.size   = width;
  header.y_axis.orient = ORIENT_DECREASING;
  header.y_axis.size   = height;
  header.format        = FORMAT_RGBE;

  success = rgbe_header_write  (&header, f);
  if (!success)
      goto cleanup;

  success = rgbe_write_uncompressed (&header, pixels, f);

cleanup:
  if (f)
      fclose (f);

  return success;
}


rgbe_file *
rgbe_load_path (const gchar *path)
{
  gboolean success = FALSE;
  rgbe_file *file;

  file = rgbe_file_new (path);
  if (!file)
      goto cleanup;

  if (!rgbe_header_read (file))
      goto cleanup;
  success = TRUE;

cleanup:
  if (!success)
    {
      rgbe_file_free (file);
      file = NULL;
    }
  return file;
}


gboolean
rgbe_get_size (rgbe_file *file,
               guint     *x,
               guint     *y)
{
  g_return_val_if_fail (file, FALSE);
  *x = file->header.x_axis.size;
  *y = file->header.y_axis.size;

  return TRUE;
}


/* Peek on each scanline row to dispatch to decoders.
 *
 * - Assumes row major ordering.
 * - Assumes cursor is at the start of a scanline
 * - Updates cursor, which is undefined on error.
 */
gfloat *
rgbe_read_scanlines (rgbe_file *file)
{
  guint     i;
  gboolean  success = FALSE;
  gfloat   *pixels  = NULL,
           *pixel_cursor;
  goffset   offset;

  g_return_val_if_fail (file,            NULL);
  g_return_val_if_fail (file->scanlines, NULL);

  pixels = pixel_cursor = g_new (gfloat, file->header.x_axis.size *
                                         file->header.y_axis.size *
                                         RGBE_NUM_RGBE);
  offset = GPOINTER_TO_UINT (file->scanlines) -
           GPOINTER_TO_UINT (g_mapped_file_get_contents (file->file));

  for (i = 0; i < file->header.y_axis.size; ++i)
    {
      const gchar *data = g_mapped_file_get_contents (file->file);

      if (data[offset + OFFSET_R] == 1 &&
          data[offset + OFFSET_G] == 1 &&
          data[offset + OFFSET_B] == 1)
        success = rgbe_read_old_rle      (file, &offset, pixel_cursor);
      else if (data[offset + OFFSET_R] == 2 &&
               data[offset + OFFSET_G] == 2)
        success = rgbe_read_new_rle      (file, &offset, pixel_cursor);
      else
        success = rgbe_read_uncompressed (file, &offset, pixel_cursor);

      if (!success)
        {
          g_warning ("Unable to parse rgbe scanlines, fail at row %u\n", i);
          goto cleanup;
        }
      pixel_cursor += file->header.x_axis.size * RGBE_NUM_RGBE;
    }

  success = TRUE;

cleanup:
  if (!success)
    {
      g_free (pixels);
      pixels = NULL;
    }

  return pixels;
}



