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

#include <glib-object.h>

#ifndef __GEGL_PARAM_SPECS_H__
#define __GEGL_PARAM_SPECS_H__

G_BEGIN_DECLS

/*
 * Keep in sync with libgeglconfig/geglconfig-params.h
 */
#define GEGL_PARAM_NO_VALIDATE (1 << (6 + G_PARAM_USER_SHIFT))


/*
 * GEGL_TYPE_PARAM_STRING
 */

#define GEGL_TYPE_PARAM_STRING           (gegl_param_string_get_type ())
#define GEGL_PARAM_SPEC_STRING(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_STRING, GeglParamSpecString))
#define GEGL_IS_PARAM_SPEC_STRING(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_STRING))

typedef struct _GeglParamSpecString GeglParamSpecString;

struct _GeglParamSpecString
{
  GParamSpecString parent_instance;

  guint            no_validate : 1;
  guint            null_ok     : 1;
};

GType        gegl_param_string_get_type (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_string     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         gboolean     no_validate,
                                         gboolean     null_ok,
                                         const gchar *default_value,
                                         GParamFlags  flags);


/*
 * GEGL_TYPE_PARAM_FILEPATH
 */

#define GEGL_TYPE_PARAM_FILE_PATH           (gegl_param_file_path_get_type ())
#define GEGL_PARAM_SPEC_FILE_PATH(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_FILE_PATH, GeglParamSpecFilePath))
#define GEGL_IS_PARAM_SPEC_FILE_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_FILE_PATH))

typedef struct _GeglParamSpecFilePath GeglParamSpecFilePath;

struct _GeglParamSpecFilePath
{
  GParamSpecString parent_instance;

  guint            no_validate : 1;
  guint            null_ok     : 1;
};

GType        gegl_param_file_path_get_type (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_file_path (const gchar *name,
                                        const gchar *nick,
                                        const gchar *blurb,
                                        gboolean     no_validate,
                                        gboolean     null_ok,
                                        const gchar *default_value,
                                        GParamFlags  flags);


/*
 * GEGL_TYPE_PARAM_MULTILINE
 */

#define GEGL_TYPE_PARAM_MULTILINE           (gegl_param_multiline_get_type ())
#define GEGL_PARAM_SPEC_MULTILINE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_MULTILINE, GeglParamSpecMultiline))
#define GEGL_IS_PARAM_SPEC_MULTILINE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_MULTILINE))

typedef struct _GeglParamSpecMultiline GeglParamSpecMultiline;

struct _GeglParamSpecMultiline
{
  GParamSpecString parent_instance;

  guint            no_validate : 1;
  guint            null_ok     : 1;
};

GType        gegl_param_multiline_get_type (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_multiline (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   const gchar *default_value,
                                   GParamFlags  flags);



/*
 * GEGL_TYPE_PARAM_ENUM
 */

#define GEGL_TYPE_PARAM_ENUM           (gegl_param_enum_get_type ())
#define GEGL_PARAM_SPEC_ENUM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_ENUM, GeglParamSpecEnum))

#define GEGL_IS_PARAM_SPEC_ENUM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_ENUM))

typedef struct _GeglParamSpecEnum GeglParamSpecEnum;

struct _GeglParamSpecEnum
{
  GParamSpecEnum  parent_instance;

  GSList         *excluded_values;
};

GType        gegl_param_enum_get_type     (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_enum         (const gchar       *name,
                                           const gchar       *nick,
                                           const gchar       *blurb,
                                           GType              enum_type,
                                           gint               default_value,
                                           GParamFlags        flags);

void   gegl_param_spec_enum_exclude_value (GeglParamSpecEnum *espec,
                                           gint               value);



G_END_DECLS
#endif  /*  __GEGL_PARAM_SPECS_H__  */
