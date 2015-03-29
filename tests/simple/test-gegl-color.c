/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2014 Jon Nordby <jononor@gmail.com>
 */


#include <glib.h>
#include <gegl.h>
#include <math.h>
#include <stdlib.h>

/* Common */

#define COLOR_FMT "%.3f"
static const gdouble color_diff_threshold = 0.001;

static void
assert_compare_rgba(const gdouble actual[4], const gdouble expect[4])
{
    gdouble diff[4];

    gboolean colors_equal = TRUE;
    for (int i=0; i<4; i++) {
        diff[i] = fabs(actual[0] - expect[0]);
        if (diff[i] > color_diff_threshold) {
            colors_equal = FALSE;
        }
    }

    if (!colors_equal) {
        g_error("GeglColor("COLOR_FMT","COLOR_FMT","COLOR_FMT","COLOR_FMT") != "
                "GeglColor("COLOR_FMT","COLOR_FMT","COLOR_FMT","COLOR_FMT")",
                actual[0], actual[1], actual[2], actual[3],
                expect[0], expect[1], expect[2], expect[3]);
    }
    g_assert(colors_equal);
}

/* Test GeglColor set_rgba/get_rgba roundtrip */
#define TP_ROUNDTRIP "/GeglColor/rgba_set_get/"

typedef struct _TestRgbaSetGetData {
    gchar *casename;
    gdouble rgba[4];
} TestRgbaSetGetData;

static TestRgbaSetGetData roundtrip_cases[] = {
    { TP_ROUNDTRIP"all_components_zero", { 0., 0., 0., 0. } },
    { TP_ROUNDTRIP"all_components_one", { 1., 1., 1., 1. } },
    { TP_ROUNDTRIP"mixed_components", { 0.3, 0.1, 0.5, 0.7 } }
};

static void
test_rgba_setget_roundtrip(const TestRgbaSetGetData *d)
{
    gdouble out[4] = { -1, -1, -1, -1 };

    GeglColor *color = gegl_color_new(NULL);
    gegl_color_set_rgba(color, d->rgba[0], d->rgba[1], d->rgba[2], d->rgba[3]);
    gegl_color_get_rgba(color, &out[0], &out[1], &out[2], &out[3]);
    assert_compare_rgba(out, d->rgba);
    g_object_unref(color);
}

/* Test GeglColor parsing of color strings */
#define TP_PARSE "/GeglColor/parse/"

typedef struct _TestParseString {
    gchar *casename;
    gchar *input;
    gdouble expected[4];
} TestParseString;

static TestParseString parse_cases[] = {
    /* cannot be tested because gegl_color_set_from_string causes g_warning 
    { TP_PARSE"invalid", "s22003",  { 0.f, 1.f, 1.f, 0.67f } }, */

    { TP_PARSE"named_white", "white",  { 1.f, 1.f, 1.f, 1.f } },
    { TP_PARSE"named_black", "black",  { 0.f, 0.f, 0.f, 1.f } },
    { TP_PARSE"named_olive", "olive",  { 0.21586f, 0.21586f, 0.f, 1.f } },

    { TP_PARSE"rgb_hex6_black", "#000000",  { 0.f, 0.f, 0.f, 1.f } },
    { TP_PARSE"rgb_hex6_white_lowercase", "#ffffff",  { 1.f, 1.f, 1.f, 1.f } },
    { TP_PARSE"rgb_hex6_white_uppercase", "#FFFFFF",  { 1.f, 1.f, 1.f, 1.f } },
    { TP_PARSE"rgb_hex8_olive_semitrans", "#808000aa",  { 0.21586f, 0.2195f, 0.f, 0.6666f } },
    { TP_PARSE"rgb_hex3_", "#333",  { 0.0331f, 0.0331f, 0.0331f, 1.f } },
    { TP_PARSE"rgb_hex4_", "#ccce",  { 0.6038f, 0.6038f, 0.6038f, 0.8549f } },

    // Note: not CSS compatible!
    { TP_PARSE"rgb_integer_black", "rgb(0,0,0)",  { 0.f, 0.f, 0.f, 1.f } },
    { TP_PARSE"rgb_float_white", "rgb(1.0, 1.0, 1.0)",  { 1.f, 1.f, 1.f, 1.f } },
    { TP_PARSE"rgba_middle_semitrans", "rgba(0.660, 0.330, 0.200, 0.4)",  { 0.660f, 0.330f, 0.200f, 0.4f } }
};

static void
test_parse_string(const TestParseString *data)
{
    gdouble out[4] = { -1, -1, -1, -1 };

    GeglColor *color = gegl_color_new(data->input);
    gegl_color_get_rgba(color, &out[0], &out[1], &out[2], &out[3]);
    assert_compare_rgba(out, data->expected);
    g_object_unref(color);
}


int
main (int argc, char *argv[])
{
    int result = -1;
    gegl_init(&argc, &argv);

    g_test_init(&argc, &argv, NULL);

    // set/get roundtrip tests
    for (int i = 0; i < G_N_ELEMENTS(roundtrip_cases); i++) {
        const TestRgbaSetGetData *data = &roundtrip_cases[i];
        g_test_add_data_func(data->casename, data, (GTestDataFunc)test_rgba_setget_roundtrip);
    }

    // parsing tests
    for (int i = 0; i < G_N_ELEMENTS(parse_cases); i++) {
        const TestParseString *data = &parse_cases[i];
        g_test_add_data_func(data->casename, data, (GTestDataFunc)test_parse_string);
    }

    result = g_test_run();
    gegl_exit();
    return result;
}
