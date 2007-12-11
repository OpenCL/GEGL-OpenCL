/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
gegl_chant_pointer (buffer, "The location where to store the output GeglBuffer")
#else

#define GEGL_CHANT_SINK
#define GEGL_CHANT_NAME            save_buffer
#define GEGL_CHANT_DESCRIPTION     "A GEGL buffer destination surface."
#define GEGL_CHANT_SELF            "save-buffer.c"
#define GEGL_CHANT_CATEGORIES      "programming:output"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
	GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
	GeglBuffer          *input;
	GeglRectangle	    *extent;

	if (self->buffer)
		{
			input = GEGL_BUFFER(gegl_operation_get_data(operation,
								    context_id,
								    "input"));
      
			g_assert (input);
			GeglBuffer **output = self->buffer;
			extent = gegl_operation_result_rect(operation,
							    context_id);
			*output = gegl_buffer_create_sub_buffer (input, extent);
		}
	return TRUE;
}

#endif
