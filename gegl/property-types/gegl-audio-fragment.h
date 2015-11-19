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
 *           2015 OEyvind Kolaas   <pippin@gimp.org>
 */

#ifndef __GEGL_AUDIO_FRAGMENT_H__
#define __GEGL_AUDIO_FRAGMENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_AUDIO_FRAGMENT            (gegl_audio_fragment_get_type ())
#define GEGL_AUDIO_FRAGMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_AUDIO_FRAGMENT, GeglAudioFragment))
#define GEGL_AUDIO_FRAGMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_AUDIO_FRAGMENT, GeglAudioFragmentClass))
#define GEGL_IS_AUDIO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_AUDIO_FRAGMENT))
#define GEGL_IS_AUDIO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_AUDIO_FRAGMENT))
#define GEGL_AUDIO_FRAGMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_AUDIO_FRAGMENT, GeglAudioFragmentClass))

typedef struct _GeglAudioFragmentClass   GeglAudioFragmentClass;
typedef struct _GeglAudioFragmentPrivate GeglAudioFragmentPrivate;

#define GEGL_MAX_AUDIO_CHANNELS 6
#define GEGL_MAX_AUDIO_SAMPLES  2400   // this limits us to 20fps and higher for regular sample rates

struct _GeglAudioFragment
{
  GObject parent_instance;
  int   sample_rate;
  int   samples;
  int   pos;
  int   channels;
  int   channel_layout;/* unused - assumed channels = 1 is mono 2 stereo */
  float data[GEGL_MAX_AUDIO_CHANNELS][GEGL_MAX_AUDIO_SAMPLES]; 
  GeglAudioFragmentPrivate *priv;
};

struct _GeglAudioFragmentClass
{
  GObjectClass parent_class;
};

GType gegl_audio_fragment_get_type (void) G_GNUC_CONST;

GeglAudioFragment *  gegl_audio_fragment_new                    (void);

void gegl_audio_fragment_set_max_samples (GeglAudioFragment *audio, int max_samples);
void gegl_audio_fragment_set_sample_rate (GeglAudioFragment *audio, int sample_rate);
void gegl_audio_fragment_set_channels (GeglAudioFragment *audio,    int channels);
void gegl_audio_fragment_set_channel_layput (GeglAudioFragment *audio, int channel_layout);
void gegl_audio_fragment_set_samples (GeglAudioFragment *audio,     int samples);
void gegl_audio_fragment_set_pos     (GeglAudioFragment *audio,     int pos);

int gegl_audio_fragment_get_max_samples (GeglAudioFragment *audio);
int gegl_audio_fragment_get_sample_rate (GeglAudioFragment *audio);
int gegl_audio_fragment_get_channels (GeglAudioFragment *audio);
int gegl_audio_fragment_get_samples (GeglAudioFragment *audio);
int gegl_audio_fragment_get_pos     (GeglAudioFragment *audio);
int gegl_audio_fragment_get_channel_layput (GeglAudioFragment *audio);




#define GEGL_TYPE_PARAM_AUDIO_FRAGMENT  (gegl_param_audio_fragment_get_type ())
#define GEGL_IS_PARAM_SPEC_AUDIO_FRAGMENT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_AUDIO_FRAGMENT))

GType        gegl_param_audio_fragment_get_type         (void) G_GNUC_CONST;

/**
 * gegl_param_spec_audio:
 * @name: canonical name of the property specified
 * @nick: nick name for the property specified
 * @blurb: description of the property specified
 * @flags: flags for the property specified
 *
 * Creates a new #GParamSpec instance specifying a #GeglAudioFragment property.
 *
 * Returns: (transfer full): a newly created parameter specification
 */
GParamSpec * gegl_param_spec_audio_fragment    (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                GParamFlags  flags);
G_END_DECLS

#endif /* __GEGL_AUDIO_FRAGMENT_H__ */
