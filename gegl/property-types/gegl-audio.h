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

#ifndef __GEGL_AUDIO_H__
#define __GEGL_AUDIO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_AUDIO            (gegl_audio_get_type ())
#define GEGL_AUDIO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_AUDIO, GeglAudio))
#define GEGL_AUDIO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_AUDIO, GeglAudioClass))
#define GEGL_IS_AUDIO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_AUDIO))
#define GEGL_IS_AUDIO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_AUDIO))
#define GEGL_AUDIO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_AUDIO, GeglAudioClass))

typedef struct _GeglAudioClass   GeglAudioClass;
typedef struct _GeglAudioPrivate GeglAudioPrivate;

struct _GeglAudio
{
  GObject           parent_instance;
  float left[4000];
  float right[4000];
  int samples;
  int samplerate;
  GeglAudioPrivate *priv;
};

struct _GeglAudioClass
{
  GObjectClass parent_class;
};

GType gegl_audio_get_type (void) G_GNUC_CONST;

GeglAudio *  gegl_audio_new                    (void);

/**
 * gegl_audio_duplicate:
 * @audio: the audio to duplicate.
 *
 * Creates a copy of @audio.
 *
 * Return value: (transfer full): A new copy of @audio.
 */
GeglAudio *  gegl_audio_duplicate              (GeglAudio   *audio);

#define GEGL_TYPE_PARAM_AUDIO           (gegl_param_audio_get_type ())
#define GEGL_IS_PARAM_SPEC_AUDIO(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_AUDIO))

GType        gegl_param_audio_get_type         (void) G_GNUC_CONST;

/**
 * gegl_param_spec_audio:
 * @name: canonical name of the property specified
 * @nick: nick name for the property specified
 * @blurb: description of the property specified
 * @flags: flags for the property specified
 *
 * Creates a new #GParamSpec instance specifying a #GeglAudio property.
 *
 * Returns: (transfer full): a newly created parameter specification
 */
GParamSpec * gegl_param_spec_audio             (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                GParamFlags  flags);
#if 0
/**
 * gegl_param_spec_audio_get_default:
 * @self: a #GeglAudio #GParamSpec
 *
 * Get the default audio value of the param spec
 *
 * Returns: (transfer none): the default #GeglAudio
 */
GeglAudio *
gegl_param_spec_audio_get_default (GParamSpec *self);
#endif

G_END_DECLS

#endif /* __GEGL_AUDIO_H__ */
