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
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_APPLY_H__
#define __GEGL_APPLY_H__

G_BEGIN_DECLS

/**
 * gegl_apply_op:
 * @buffer: the #GeglBuffer to apply onto
 * @operation_name: name of the operation to apply
 * @...: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to buffer, overwritting the contents of buffer.
 *
 */
void        gegl_apply_op  (GeglBuffer    *buffer,
                            const gchar   *operation_name,
                            ...) G_GNUC_NULL_TERMINATED;

/**
 * gegl_filter_op:
 * @source_buffer: the source #GeglBuffer for the filter
 * @operation_name: name of the operation to apply
 * @...: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to source_buffer, returning the result in a new buffer.
 *
 * Return value: (transfer full): the result of the filter
 */
GeglBuffer *gegl_filter_op (GeglBuffer    *source_buffer,
                            const gchar   *operation_name,
                            ...) G_GNUC_NULL_TERMINATED;


/**
 * gegl_render_op:
 * @source_buffer: the source #GeglBuffer for the filter
 * @target_buffer: the source #GeglBuffer for the filter
 * @operation_name: name of the operation to apply
 * @...: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to source_buffer, writing the results to target_buffer.
 *
 */
void        gegl_render_op (GeglBuffer    *source_buffer,
                            GeglBuffer    *target_buffer,
                            const gchar   *operation_name,
                            ...) G_GNUC_NULL_TERMINATED;

/* the following only exist to make gegl_apply and gegl_filter bindable */

/**
 * gegl_apply_op_valist:
 * @buffer: the #GeglBuffer to apply onto
 * @operation_name: name of the operation to apply
 * @var_args: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to buffer, overwritting the contents of buffer.
 *
 */
void        gegl_apply_op_valist (GeglBuffer    *buffer,
                                  const gchar   *operation_name,
                                  va_list        var_args);

/**
 * gegl_filter_op_valist:
 * @source_buffer: the source #GeglBuffer for the filter
 * @operation_name: name of the operation to apply
 * @var_args: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to source_buffer, returning the result in a new buffer.
 *
 * Return value: (transfer full): the result of the filter
 */
GeglBuffer *gegl_filter_op_valist (GeglBuffer   *source_buffer,
                                   const gchar  *operation_name,
                                   va_list       var_args);

/**
 * gegl_render_op_valist:
 * @source_buffer: the source #GeglBuffer for the filter
 * @target_buffer: the source #GeglBuffer for the filter
 * @operation_name: name of the operation to apply
 * @var_args: the settings for the operation. Zero or more key/value pairs,
 * ended terminated with NULL.
 *
 * Apply the operation to source_buffer, writing the results to target_buffer.
 *
 */
void        gegl_render_op_valist (GeglBuffer   *source_buffer,
                                   GeglBuffer   *target_buffer,
                                   const gchar  *operation_name,
                                   va_list       var_args);

G_END_DECLS

#endif /* __GEGL_APPLY_H__ */
