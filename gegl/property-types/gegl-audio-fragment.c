/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-audio-fragment.h"

enum
{
  PROP_0,
  PROP_STRING
};

struct _GeglAudioFragmentPrivate
{
  int     max_samples;
  int     sample_count;
  int     channels;
  int     channel_layout;
  int     sample_rate;
  int     pos;
};

static void      set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec);
static void      get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec);

G_DEFINE_TYPE (GeglAudioFragment, gegl_audio_fragment, G_TYPE_OBJECT)


static void deallocate_data (GeglAudioFragment *audio)
{
  int i;
  for (i = 0; i < GEGL_MAX_AUDIO_CHANNELS; i++)
  {
    if (audio->data[i])
    {
      g_free (audio->data[i]);
      audio->data[i] = NULL;
    }
  }
}

static void allocate_data (GeglAudioFragment *audio)
{
  int i;
  deallocate_data (audio);
  if (audio->priv->channels <= 0 ||
      audio->priv->max_samples <= 0)
    return;
  for (i = 0; i < audio->priv->channels; i++)
  {
    audio->data[i] = g_malloc (sizeof (float) * audio->priv->max_samples);
  }
}

#define GEGL_MAX_AUDIO_SAMPLES  2400 

static void
gegl_audio_fragment_init (GeglAudioFragment *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), GEGL_TYPE_AUDIO_FRAGMENT, GeglAudioFragmentPrivate);
  self->priv->max_samples = GEGL_MAX_AUDIO_SAMPLES;
  allocate_data (self);
}

static void
gegl_audio_fragment_finalize (GObject *self)
{
  GeglAudioFragment *audio = (void*)self;
  deallocate_data (audio);
}

static void
gegl_audio_fragment_class_init (GeglAudioFragmentClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = gegl_audio_fragment_finalize;

  g_object_class_install_property (gobject_class, PROP_STRING,
                                   g_param_spec_string ("string",
                                                        "String",
                                                        "A String representation of the GeglAudioFragment",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GeglAudioFragmentPrivate));
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  //GeglAudioFragment *audio = GEGL_AUDIO_FRAGMENT (gobject);

  switch (property_id)
    {
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
  //GeglAudioFragment *audio = GEGL_AUDIO_FRAGMENT (gobject);

  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}


GeglAudioFragment *
gegl_audio_fragment_new (int sample_rate, int channels, int channel_layout, int max_samples)
{
  GeglAudioFragment *ret = g_object_new (GEGL_TYPE_AUDIO_FRAGMENT, NULL);
  gegl_audio_fragment_set_sample_rate (ret, sample_rate);
  gegl_audio_fragment_set_channel_layout (ret, channel_layout);
  gegl_audio_fragment_set_max_samples (ret, max_samples);
  gegl_audio_fragment_set_channels (ret, channels);
  return ret;
}

void
gegl_audio_fragment_set_max_samples (GeglAudioFragment *audio,
                                     int                max_samples)
{
  if (audio->priv->max_samples == max_samples)
    return;
  audio->priv->max_samples = max_samples;
  allocate_data (audio);
}

void
gegl_audio_fragment_set_sample_rate (GeglAudioFragment *audio,
                                     int                sample_rate)
{
  audio->priv->sample_rate = sample_rate;
}

void
gegl_audio_fragment_set_channels (GeglAudioFragment *audio,
                                  int                channels)
{
  if (audio->priv->channels == channels)
    return;
  audio->priv->channels = channels;
  allocate_data (audio);
}

void
gegl_audio_fragment_set_channel_layout (GeglAudioFragment *audio,
                                        int                channel_layout)
{
  audio->priv->channel_layout = channel_layout;
}

void
gegl_audio_fragment_set_sample_count (GeglAudioFragment *audio,
                                      int                samples)
{
  audio->priv->sample_count = samples;
}

void
gegl_audio_fragment_set_pos (GeglAudioFragment *audio,
                             int                pos)
{
  audio->priv->pos = pos;
}

int
gegl_audio_fragment_get_max_samples (GeglAudioFragment *audio)
{
  return audio->priv->max_samples;
}


int
gegl_audio_fragment_get_sample_rate (GeglAudioFragment *audio)
{
  return audio->priv->sample_rate;
}

int
gegl_audio_fragment_get_channels (GeglAudioFragment *audio)
{
  return audio->priv->channels;
}

int
gegl_audio_fragment_get_sample_count (GeglAudioFragment *audio)
{
  return audio->priv->sample_count;
}

int
gegl_audio_fragment_get_pos     (GeglAudioFragment *audio)
{
  return audio->priv->pos;
}

int
gegl_audio_fragment_get_channel_layout (GeglAudioFragment *audio)
{
  return audio->priv->channel_layout;
}

/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglAudioFragment as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_AUDIO_FRAGMENT (obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_AUDIO_FRAGMENT, GeglParamAudioFragment))
#define GEGL_IS_PARAM_AUDIO_FRAGMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_AUDIO_FRAGMENT))

typedef struct _GeglParamAudioFragment GeglParamAudioFragment;

struct _GeglParamAudioFragment
{
  GParamSpec parent_instance;
};

static void
gegl_param_audio_fragment_init (GParamSpec *self)
{
}

static void
gegl_param_audio_fragment_finalize (GParamSpec *self)
{
  //GeglParamAudioFragment  *param_audio  = GEGL_PARAM_AUDIO (self);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_AUDIO_FRAGMENT));

  parent_class->finalize (self);
}

static void
gegl_param_audio_fragment_set_default (GParamSpec *param_spec,
                              GValue     *value)
{
  //GeglParamAudioFragment *gegl_audio_fragment = GEGL_PARAM_AUDIO (param_spec);
}

GType
gegl_param_audio_fragment_get_type (void)
{
  static GType param_audio_fragment_type = 0;

  if (G_UNLIKELY (param_audio_fragment_type == 0))
    {
      static GParamSpecTypeInfo param_audio_fragment_type_info = {
        sizeof (GeglParamAudioFragment),
        0,
        gegl_param_audio_fragment_init,
        0,
        gegl_param_audio_fragment_finalize,
        gegl_param_audio_fragment_set_default,
        NULL,
        NULL
      };
      param_audio_fragment_type_info.value_type = GEGL_TYPE_AUDIO_FRAGMENT;

      param_audio_fragment_type = g_param_type_register_static ("GeglParamAudioFragment",
                                                       &param_audio_fragment_type_info);
    }

  return param_audio_fragment_type;
}

GParamSpec *
gegl_param_spec_audio_fragment (const gchar *name,
                                const gchar *nick,
                                const gchar *blurb,
                                GParamFlags  flags)
{
  GeglParamAudioFragment *param_audio;

  param_audio = g_param_spec_internal (GEGL_TYPE_PARAM_AUDIO_FRAGMENT,
                                       name, nick, blurb, flags);

  return G_PARAM_SPEC (param_audio);
}


