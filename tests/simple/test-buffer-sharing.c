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
 * Copyright (C) 2017 Jon Nordby <jononor@gmail.com>
 */

#include <gegl.h>
#include <glib/gstdio.h>

// in order of progression
typedef enum _TestState {
    TestInvalid = 0,
    TestInitialized,
    TestSetBlue,
    TestWaitingForBlue,
    TestAssertBlue,
    TestSetYellow,
    TestWaitingForYellow,
    TestAssertYellow,
    TestFailed,
    TestSucceed,
} TestState;

typedef struct _TestData {
    GMainLoop *loop;
    GeglBuffer *a;
    GeglBuffer *b;
    TestState state;
    gchar *temp_dir;
    gchar *file_path;
} TestData;

static void
print_color(GeglColor *color) {
    unsigned char rgba[4];
    const Babl *format = babl_format("R'G'B'A u8");
    gegl_color_get_pixel(color, format, (gpointer)rgba);
    g_print("[%d, %d, %d, %d]",
        rgba[0], rgba[1], rgba[2], rgba[3]);
}

static gboolean
assert_color_equal(GeglColor *expect, GeglColor *actual) {
    const Babl *format = babl_format("R'G'B'A u8");
    unsigned char e[4];
    unsigned char a[4];
    gegl_color_get_pixel(expect, format, (gpointer)e);
    gegl_color_get_pixel(actual, format, (gpointer)a);
    {
    const gboolean equal =
        a[0] == e[0] &&
        a[1] == e[1] &&
        a[2] == e[2] &&
        a[3] == e[3];

    if (!equal) {
        print_color(expect);
        g_print(" != ");
        print_color(actual);
        g_print("\n");
        return FALSE;
    }
    }
    return TRUE;
}

static GeglColor *
buffer_get_color(GeglBuffer *buffer) {
    const int pixels = 1;
    guint8 pixel[4];
    GeglRectangle r = { 0, 0, 1, pixels };
    const Babl *format = babl_format("R'G'B'A u8");
    GeglColor *color = gegl_color_new(NULL);
    gegl_buffer_get(buffer, &r, 1.0, format, (gpointer)(pixel), GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    gegl_color_set_pixel(color, format, (gpointer)pixel);
    return color;
}

// Core state-machine
static void
test_change_state(TestData *data, TestState new) {
    GeglColor *blue = gegl_color_new("blue");
    GeglColor *yellow = gegl_color_new("yellow");
    GeglRectangle rect = { 0, 0, 100, 100 };

    data->state = new;

    switch (data->state) {
    // test actions and checking
    case TestSetBlue:
        // Write blue to A, should be reflected in B
        gegl_buffer_set_extent(data->a, &rect);
        gegl_buffer_set_color(data->a, &rect, blue);
        gegl_buffer_flush(data->a);
        test_change_state(data, TestWaitingForBlue);
        break;
    case TestAssertBlue:
        {
            GeglColor *actual = buffer_get_color(data->a);
            const gboolean pass = assert_color_equal(blue, actual);
            test_change_state(data, (pass) ? TestSetYellow : TestFailed);
        }
        break;
    case TestSetYellow:
        // Write blue to A, should be reflected in B
        gegl_buffer_set_extent(data->a, &rect);
        gegl_buffer_set_color(data->a, &rect, yellow);
        gegl_buffer_flush(data->a);
        test_change_state(data, TestWaitingForYellow);
        break;
    case TestAssertYellow:
        {
            GeglColor *actual = buffer_get_color(data->a);
            const gboolean pass = assert_color_equal(yellow, actual);
            test_change_state(data, (pass) ? TestSucceed : TestFailed);
        }
        break;
    // handled elsewhere
    case TestWaitingForYellow:
    case TestInitialized:
    case TestWaitingForBlue:
        break;
    // exit
    case TestInvalid:
    case TestFailed:
    case TestSucceed:
        g_main_loop_quit(data->loop);
        break;
    }
}

static void
on_buffer_changed(GeglBuffer        *buffer,
                const GeglRectangle *rect,
                gpointer             user_data)
{
    TestData *test = (TestData *)user_data;
    //g_print("changed! %d %d %d %d \n", rect->x, rect->y, rect->width, rect->height);

    if (test->state == TestWaitingForBlue) {
        test_change_state(test, TestAssertBlue);
    } else if (test->state == TestWaitingForYellow) {
        test_change_state(test, TestAssertYellow);
    } else {
        test_change_state(test, TestFailed);
    }
}

static gboolean
on_timeout(gpointer user_data) {
    TestData *test = (TestData *)user_data;
    g_print("timeout!\n");
    test_change_state(test, TestFailed);
    return FALSE;
}

#include <unistd.h>

static void
test_init(TestData *data) {
    GeglRectangle rect = { 0, 0, 100, 100 };
    GeglColor *blank = gegl_color_new("transparent");

    data->loop = g_main_loop_new(NULL, TRUE);
    data->temp_dir = g_strdup("test-buffer-sharing-XXXXXX");
    data->temp_dir = g_mkdtemp(data->temp_dir);
    data->file_path = g_strjoin(G_DIR_SEPARATOR_S, data->temp_dir, "buffer.gegl", NULL);

    data->a = gegl_buffer_open(data->file_path);
    // FIXME: if not setting an extent and adding some data, the written on-disk file seems to be corrupt
    gegl_buffer_set_extent(data->a, &rect);
    gegl_buffer_set_color(data->a, &rect, blank);
    gegl_buffer_flush(data->a); // ensure file exists on disk

    sleep(1);

    // B observes the same on-disk buffer
    data->b = gegl_buffer_open(data->file_path);
    data->state = TestInitialized;

    gegl_buffer_signal_connect(data->b, "changed", (void*)on_buffer_changed, data);
    g_timeout_add_seconds(10, on_timeout, data);
}

static void
test_destroy(TestData *data) {

    g_remove(data->file_path);
    g_free(data->file_path);

    g_rmdir(data->temp_dir);
    g_free(data->temp_dir);

    g_object_unref(data->a);
    g_object_unref(data->b);
}

int
main (int    argc,
      char  *argv[])
{
    int exitcode;
    TestData test;
    TestData *data = &test;
    gegl_init (&argc, &argv);

    test_init(data);

    test_change_state(data, TestSetBlue);

    g_main_loop_run(data->loop);
    exitcode = (data->state == TestSucceed) ? 0 : 1;
    g_print("%s\n", (data->state == TestSucceed) ? "PASS" : "FAIL");
    test_destroy(data);
    return exitcode;
}
