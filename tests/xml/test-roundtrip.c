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
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
 */

/* Test XML load->save roundtrips  */

#include <glib.h>
#include <glib/gprintf.h>

#include <gegl.h>

#include "common.c"

typedef struct {
    gchar *expected_result;
    gchar *xml_output;
    GeglNode *graph;
} TestRoundtripFixture;


static void
test_xml_roundtrip_setup(TestRoundtripFixture *fixture, const void *data)
{
    const gchar *file_path = (const gchar *)data;
    gchar *file_contents = NULL;
    gchar *xml_output = NULL;
    gchar *cwd = g_get_current_dir();
    gboolean success = g_file_get_contents(file_path, &file_contents, NULL, NULL);
    GeglNode *graph = NULL;

    g_assert(success);
    g_assert(file_contents);

    graph = gegl_node_new_from_xml(file_contents, "");
    g_assert(graph);

    xml_output = gegl_node_to_xml(graph, "");
    g_assert(xml_output);

    fixture->expected_result = file_contents;
    fixture->xml_output = xml_output;
    g_free(cwd);
}

/*
 *
 * Create a graph from XML, save this graph to XML and compare the results */
static void
test_xml_roundtrip(TestRoundtripFixture *fixture, const void *data)
{
    assert_equivalent_xml(fixture->xml_output, fixture->expected_result);
}


static void
test_xml_roundtrip_teardown(TestRoundtripFixture *fixture, const void *data)
{
    g_free(fixture->xml_output);
    g_free(fixture->expected_result);
    g_free((gpointer)data);
}

static gboolean
add_tests_for_xml_files_in_directory(const gchar *path)
{
    GError *error = NULL;
    GDir *dir = g_dir_open(path, 0, &error);
    const gchar *filename = NULL;

    if (!dir) {
        g_fprintf(stderr, "Unable to open directory: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    while ( (filename = g_dir_read_name(dir)) ) {
        gchar *test_path;
        const gchar *file_path = g_build_filename(path, filename, NULL);
        test_path = g_strdup_printf("/xml/roundtrip/%s/%s", path, filename);;

        if (!g_str_has_suffix(filename, ".xml")) {
            continue;
        }

        /* Need to pass in the path name */
        g_test_add (test_path, TestRoundtripFixture, file_path,
                    test_xml_roundtrip_setup,
                    test_xml_roundtrip,
                    test_xml_roundtrip_teardown);
        g_free(test_path);
        /* file_path freed by teardown function*/

    }
    g_dir_close(dir);

    return TRUE;
}

int
main (int argc, char *argv[])
{
    int result = -1;
    gchar *datadir;

    gegl_init(&argc, &argv);
    g_test_init(&argc, &argv, NULL);

    datadir = g_build_filename (g_getenv ("ABS_TOP_SRCDIR"), "tests/xml/data", NULL);
    if (!add_tests_for_xml_files_in_directory(datadir)) {
        result = -1;
    }
    else {
        result = g_test_run();
    }

    gegl_exit();
    return result;
}
