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
 * Copyright   2015 OEyvind Kolaas   <pippin@gimp.org>
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

#define GEGL_MAX_AUDIO_CHANNELS 8


/* the values for channel_layout are designed to match and be compatible with
 * the ones in ffmpegs libavutil
 */
#define GEGL_CH_FRONT_LEFT             0x00000001
#define GEGL_CH_FRONT_RIGHT            0x00000002
#define GEGL_CH_FRONT_CENTER           0x00000004
#define GEGL_CH_LOW_FREQUENCY          0x00000008
#define GEGL_CH_BACK_LEFT              0x00000010
#define GEGL_CH_BACK_RIGHT             0x00000020
#define GEGL_CH_FRONT_LEFT_OF_CENTER   0x00000040
#define GEGL_CH_FRONT_RIGHT_OF_CENTER  0x00000080
#define GEGL_CH_BACK_CENTER            0x00000100
#define GEGL_CH_SIDE_LEFT              0x00000200
#define GEGL_CH_SIDE_RIGHT             0x00000400
#define GEGL_CH_TOP_CENTER             0x00000800
#define GEGL_CH_TOP_FRONT_LEFT         0x00001000
#define GEGL_CH_TOP_FRONT_CENTER       0x00002000
#define GEGL_CH_TOP_FRONT_RIGHT        0x00004000
#define GEGL_CH_TOP_BACK_LEFT          0x00008000
#define GEGL_CH_TOP_BACK_CENTER        0x00010000
#define GEGL_CH_TOP_BACK_RIGHT         0x00020000
#define GEGL_CH_STEREO_LEFT            0x20000000  ///< Stereo downmix.
#define GEGL_CH_STEREO_RIGHT           0x40000000  ///< See GEGL_CH_STEREO_LEFT.
#define GEGL_CH_WIDE_LEFT              0x0000000080000000ULL
#define GEGL_CH_WIDE_RIGHT             0x0000000100000000ULL
#define GEGL_CH_SURROUND_DIRECT_LEFT   0x0000000200000000ULL
#define GEGL_CH_SURROUND_DIRECT_RIGHT  0x0000000400000000ULL
#define GEGL_CH_LOW_FREQUENCY_2        0x0000000800000000ULL

#define GEGL_CH_LAYOUT_NATIVE          0x8000000000000000ULL
#define GEGL_CH_LAYOUT_MONO              (GEGL_CH_FRONT_CENTER)
#define GEGL_CH_LAYOUT_STEREO            (GEGL_CH_FRONT_LEFT|GEGL_CH_FRONT_RIGHT)
#define GEGL_CH_LAYOUT_2POINT1           (GEGL_CH_LAYOUT_STEREO|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_2_1               (GEGL_CH_LAYOUT_STEREO|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_SURROUND          (GEGL_CH_LAYOUT_STEREO|GEGL_CH_FRONT_CENTER)
#define GEGL_CH_LAYOUT_3POINT1           (GEGL_CH_LAYOUT_SURROUND|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_4POINT0           (GEGL_CH_LAYOUT_SURROUND|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_4POINT1           (GEGL_CH_LAYOUT_4POINT0|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_2_2               (GEGL_CH_LAYOUT_STEREO|GEGL_CH_SIDE_LEFT|GEGL_CH_SIDE_RIGHT)
#define GEGL_CH_LAYOUT_QUAD              (GEGL_CH_LAYOUT_STEREO|GEGL_CH_BACK_LEFT|GEGL_CH_BACK_RIGHT)
#define GEGL_CH_LAYOUT_5POINT0           (GEGL_CH_LAYOUT_SURROUND|GEGL_CH_SIDE_LEFT|GEGL_CH_SIDE_RIGHT)
#define GEGL_CH_LAYOUT_5POINT1           (GEGL_CH_LAYOUT_5POINT0|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_5POINT0_BACK      (GEGL_CH_LAYOUT_SURROUND|GEGL_CH_BACK_LEFT|GEGL_CH_BACK_RIGHT)
#define GEGL_CH_LAYOUT_5POINT1_BACK      (GEGL_CH_LAYOUT_5POINT0_BACK|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_6POINT0           (GEGL_CH_LAYOUT_5POINT0|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_6POINT0_FRONT     (GEGL_CH_LAYOUT_2_2|GEGL_CH_FRONT_LEFT_OF_CENTER|GEGL_CH_FRONT_RIGHT_OF_CENTER)
#define GEGL_CH_LAYOUT_HEXAGONAL         (GEGL_CH_LAYOUT_5POINT0_BACK|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_6POINT1           (GEGL_CH_LAYOUT_5POINT1|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_6POINT1_BACK      (GEGL_CH_LAYOUT_5POINT1_BACK|GEGL_CH_BACK_CENTER)
#define GEGL_CH_LAYOUT_6POINT1_FRONT     (GEGL_CH_LAYOUT_6POINT0_FRONT|GEGL_CH_LOW_FREQUENCY)
#define GEGL_CH_LAYOUT_7POINT0           (GEGL_CH_LAYOUT_5POINT0|GEGL_CH_BACK_LEFT|GEGL_CH_BACK_RIGHT)
#define GEGL_CH_LAYOUT_7POINT0_FRONT     (GEGL_CH_LAYOUT_5POINT0|GEGL_CH_FRONT_LEFT_OF_CENTER|GEGL_CH_FRONT_RIGHT_OF_CENTER)
#define GEGL_CH_LAYOUT_7POINT1           (GEGL_CH_LAYOUT_5POINT1|GEGL_CH_BACK_LEFT|GEGL_CH_BACK_RIGHT)
#define GEGL_CH_LAYOUT_7POINT1_WIDE      (GEGL_CH_LAYOUT_5POINT1|GEGL_CH_FRONT_LEFT_OF_CENTER|GEGL_CH_FRONT_RIGHT_OF_CENTER)
#define GEGL_CH_LAYOUT_7POINT1_WIDE_BACK (GEGL_CH_LAYOUT_5POINT1_BACK|GEGL_CH_FRONT_LEFT_OF_CENTER|GEGL_CH_FRONT_RIGHT_OF_CENTER)
#define GEGL_CH_LAYOUT_OCTAGONAL         (GEGL_CH_LAYOUT_5POINT0|GEGL_CH_BACK_LEFT|GEGL_CH_BACK_CENTER|GEGL_CH_BACK_RIGHT)
#define GEGL_CH_LAYOUT_HEXADECAGONAL     (GEGL_CH_LAYOUT_OCTAGONAL|GEGL_CH_WIDE_LEFT|GEGL_CH_WIDE_RIGHT|GEGL_CH_TOP_BACK_LEFT|GEGL_CH_TOP_BACK_RIGHT|GEGL_CH_TOP_BACK_CENTER|GEGL_CH_TOP_FRONT_CENTER|GEGL_CH_TOP_FRONT_LEFT|GEGL_CH_TOP_FRONT_RIGHT)
#define GEGL_CH_LAYOUT_STEREO_DOWNMIX    (GEGL_CH_STEREO_LEFT|GEGL_CH_STEREO_RIGHT)

struct _GeglAudioFragment
{
  GObject parent_instance;
  float  *data[GEGL_MAX_AUDIO_CHANNELS];
  GeglAudioFragmentPrivate *priv;
};

struct _GeglAudioFragmentClass
{
  GObjectClass parent_class;
};

GType gegl_audio_fragment_get_type (void) G_GNUC_CONST;

GeglAudioFragment *  gegl_audio_fragment_new (int sample_rate, int channels, int channel_layout, int max_samples);

void gegl_audio_fragment_set_max_samples (GeglAudioFragment *audio, int max_samples);
void gegl_audio_fragment_set_sample_rate (GeglAudioFragment *audio, int sample_rate);
void gegl_audio_fragment_set_channels (GeglAudioFragment *audio,    int channels);
void gegl_audio_fragment_set_channel_layout (GeglAudioFragment *audio, int channel_layout);
void gegl_audio_fragment_set_sample_count (GeglAudioFragment *audio,     int sample_count);
void gegl_audio_fragment_set_pos     (GeglAudioFragment *audio,     int pos);

int gegl_audio_fragment_get_max_samples (GeglAudioFragment *audio);
int gegl_audio_fragment_get_sample_rate (GeglAudioFragment *audio);
int gegl_audio_fragment_get_channels (GeglAudioFragment *audio);
int gegl_audio_fragment_get_sample_count (GeglAudioFragment *audio);
int gegl_audio_fragment_get_pos     (GeglAudioFragment *audio);
int gegl_audio_fragment_get_channel_layout (GeglAudioFragment *audio);




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
