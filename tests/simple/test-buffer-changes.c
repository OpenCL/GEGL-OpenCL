/* This file is a test-case for GEGL
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
 */

#include <gegl.h>

typedef struct {
    GeglBuffer *buffer;
    GeglRectangle buffer_extent;
    const Babl *buffer_format;
    
    guint buffer_changed_called;
    GeglRectangle buffer_changed_rect;
} TestCase;

GeglRectangle null_rect = {0, 0, 0, 0};

gboolean    test_gegl_rectangle_equal(const GeglRectangle *expected, const GeglRectangle *actual);
TestCase *  test_case_new(void);
void        handle_buffer_changed(GeglBuffer *buffer, const GeglRectangle *rect, gpointer user_data);
void        test_buffer_change_signal_on_set(void);
void        test_buffer_change_signal_with_iter(guint access_method, guint expected_signal_calls);
void        test_buffer_change_signal_with_iter_write(void);
void        test_buffer_change_signal_with_iter_readwrite(void);
void        test_buffer_no_change_signal_with_iter_read(void);

gboolean
test_gegl_rectangle_equal(const GeglRectangle *expected, const GeglRectangle *actual)
{
    gboolean equal = gegl_rectangle_equal(expected, actual);
    if (!equal) {
        g_warning("GeglRectangle(%d, %d %dx%d) != GeglRectangle(%d, %d %dx%d)",
                  expected->x, expected->y, expected->width, expected->height,
                  actual->x, actual->y, actual->width, actual->height);
    }
    return equal;
}

TestCase *
test_case_new(void)
{
    TestCase *test_case = g_new(TestCase, 1);

    test_case->buffer_extent.x = 0;
    test_case->buffer_extent.y = 0;
    test_case->buffer_extent.width = 500;
    test_case->buffer_extent.height = 500;
    test_case->buffer_format = babl_format("RGBA u8");
    test_case->buffer = gegl_buffer_new(&test_case->buffer_extent, test_case->buffer_format);
    
    test_case->buffer_changed_called = 0;
    test_case->buffer_changed_rect = null_rect;
    return test_case;
}


void
handle_buffer_changed(GeglBuffer *buffer, const GeglRectangle *rect, gpointer user_data)
{
    TestCase *t = (TestCase *)user_data;
    t->buffer_changed_called++;
    t->buffer_changed_rect = *rect;
}

/* Test that 'changed' signal is emitted on gegl_buffer_set */
void
test_buffer_change_signal_on_set(void)
{
    TestCase *test_case = test_case_new();
    GeglRectangle rect = {0, 0, 100, 100};
    char *tmp = g_malloc(rect.height*rect.width*1*4);
    
    gegl_buffer_signal_connect(test_case->buffer, "changed", (GCallback)handle_buffer_changed, test_case);
    
    gegl_buffer_set(test_case->buffer, &rect, 0, test_case->buffer_format, tmp, GEGL_AUTO_ROWSTRIDE);
    
    g_assert_cmpint(test_case->buffer_changed_called, ==, 1);
    g_assert(test_gegl_rectangle_equal(&(test_case->buffer_changed_rect), &rect));
    
    g_free(tmp);
    g_free(test_case);
}

/* Utility function to test emission of 'changed' signal on GeglBuffer
 * when accessing with GeglBufferIterator. 
 * @access_method: GEGL_ACCESS_READ, GEGL_ACCESS_WRITE, GEGL_ACCESS_READWRITE
 * @expected_signal_calls: Whether the 'changed' signal is expected to be emitted or not
 */
void
test_buffer_change_signal_with_iter(guint access_method, guint expected_signal_calls)
{
    TestCase *test_case = test_case_new();
    GeglRectangle rect = {0, 0, 100, 100};
    char *tmp = g_malloc(rect.height*rect.width*1*4);
    GeglBufferIterator *gi = gegl_buffer_iterator_new(test_case->buffer, &rect, 0,
                                test_case->buffer_format, access_method, GEGL_ABYSS_NONE);
    
    gegl_buffer_signal_connect(test_case->buffer, "changed", (GCallback)handle_buffer_changed, test_case);

    while (gegl_buffer_iterator_next(gi)) {
    }

    if (expected_signal_calls == 0)
        rect = null_rect;

    g_assert(test_case->buffer_changed_called == expected_signal_calls);
    g_assert(test_gegl_rectangle_equal(&(test_case->buffer_changed_rect), &rect));
    
    g_free(tmp);
    g_free(test_case);
}

/* Test that 'changed' signal is emitted once for gegl_buffer_iterator in WRITE mode */
void
test_buffer_change_signal_with_iter_write(void)
{
    test_buffer_change_signal_with_iter(GEGL_ACCESS_WRITE, 1);
}

/* Test that 'changed' signal is emitted once for gegl_buffer_iterator in READWRITE mode */
void
test_buffer_change_signal_with_iter_readwrite(void)
{
    test_buffer_change_signal_with_iter(GEGL_ACCESS_READWRITE, 1);
}

/* Test that 'changed' signal is _not_ emitted on gegl_buffer_iterator in READ mode */
void
test_buffer_no_change_signal_with_iter_read(void)
{
    test_buffer_change_signal_with_iter(GEGL_ACCESS_READ, 0);
}

gint
main(gint argc, gchar **argv)
{
    babl_init();
    gegl_init(&argc, &argv);
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/buffer/change/signal-on-set", test_buffer_change_signal_on_set);
    g_test_add_func ("/buffer/change/no-signal-with-iter-read", test_buffer_no_change_signal_with_iter_read);
    g_test_add_func ("/buffer/change/signal-with-iter-readwrite", test_buffer_change_signal_with_iter_readwrite);
    g_test_add_func ("/buffer/change/signal-with-iter-write", test_buffer_change_signal_with_iter_write);
    return g_test_run();
}
