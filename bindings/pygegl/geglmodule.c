/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   geglmodule.c: module wrapping the GEGL library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

void pygegl_register_classes (PyObject *d);
void pygegl_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pygegl_functions[];

DL_EXPORT(void)
initgegl(void)
{
    PyObject *m, *d;

    /* set the default Python encoding to utf-8 */
    PyUnicode_SetDefaultEncoding("utf-8");

    init_pygobject ();

#if 0
    pygimp_init_pygobject();

    /* Create the module and add the functions */
    m = Py_InitModule4("gimp", gimp_methods,
                       gimp_module_documentation,
                       NULL, PYTHON_API_VERSION);
#endif

    m = Py_InitModule ("gegl", pygegl_functions);
    d = PyModule_GetDict (m);

    pygegl_register_classes (d);
//    pygegl_add_constants(m, "GEGL_");
}

//quitgegl(void)
//{
//    quit_pygobject ();
//}

