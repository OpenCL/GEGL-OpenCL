/* PyGEGL - Python bindings for the GEGL image processing library
 * Copyright (C) 2007 Manish Singh
 *
 *   geglmodule.c: module wrapping the GEGL library.
 *
 * PyGEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * PyGEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PyGEGL; if not, see <http://www.gnu.org/licenses/>.
 */

/* Portions of this are derived from parts of the PyGTK project */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

void pygegl_register_classes (PyObject *d);
void pygegl_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pygegl_functions[];

static char pygegl_doc[] = "This module provides bindings for GEGL";

static void
pygegl_init(void)
{
    /* Taken from pygtk's gtk_init_check implementation */
    PyObject *av;
    int argc, i;
    char **argv;

    av = PySys_GetObject("argv");
    if (av != NULL) {
        if (!PyList_Check(av)) {
            PyErr_Warn(PyExc_Warning,
                       "ignoring sys.argv: it must be a list of strings");
            av = NULL;
        } else {
            argc = PyList_Size(av);
            for (i = 0; i < argc; i++)
                if (!PyString_Check(PyList_GetItem(av, i))) {
                    PyErr_Warn(PyExc_Warning,
                               "ignoring sys.argv: it must be a list of strings");
                    av = NULL;
                    break;
                }
        }
    }
    if (av != NULL) {
        argv = g_new(char *, argc);
        for (i = 0; i < argc; i++)
            argv[i] = g_strdup(PyString_AsString(PyList_GetItem(av, i)));
    } else {
        argc = 0;
        argv = NULL;
    }

    gegl_init(&argc, &argv);

    if (argv != NULL) {
        PySys_SetArgv(argc, argv);
        for (i = 0; i < argc; i++)
            g_free(argv[i]);
        g_free(argv);
    }
}

PyMODINIT_FUNC
init_gegl(void)
{
    PyObject *m, *d;
    PyObject *av;
    int argc, i;
    char **argv;
    GLogLevelFlags fatal_mask;

    PyUnicode_SetDefaultEncoding("utf-8");

    init_pygobject ();

    m = Py_InitModule3("_gegl", pygegl_functions, pygegl_doc);
    d = PyModule_GetDict(m);

    pygegl_register_classes(d);
    /* pygegl_add_constants(m, "GEGL_"); */

    pygegl_init();

    if (PyErr_Occurred())
        Py_FatalError("can't initialize module _gegl");
}
