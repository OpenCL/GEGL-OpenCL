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

/* Test GeglColor */

#include <glib.h>
#include <gegl.h>

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
test_rgba_setget_roundtrip(const TestRgbaSetGetData *data)
{
    GeglColor *color = gegl_color_new(NULL);
    gdouble out[4] = { -1, -1, -1, -1 };
    gdouble in[4] = { data->rgba[0], data->rgba[1], data->rgba[2], data->rgba[3] } ;

    gegl_color_set_rgba(color, in[0], in[1], in[2], in[3]);
    gegl_color_get_rgba(color, &out[0], &out[1], &out[2], &out[3]);

    g_assert_cmpfloat((float)in[0], ==, (float)out[0]);
    g_assert_cmpfloat((float)in[1], ==, (float)out[1]);
    g_assert_cmpfloat((float)in[2], ==, (float)out[2]);
    g_assert_cmpfloat((float)in[3], ==, (float)out[3]);

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
        TestRgbaSetGetData *data = &roundtrip_cases[i];
        g_test_add_data_func(data->casename, data, (GTestDataFunc)test_rgba_setget_roundtrip);
    }

    result = g_test_run();
    gegl_exit();
    return result;
}
