
 /**
 * gegl_cl_init:
 * @error: Any error that occured
 *
 * Initialize and enable OpenCL, calling this function again
 * will re-enable OpenCL if it has been disabled.
 *
 * Return value: True if OpenCL was initialized
 */
gboolean          gegl_cl_init (GError **error);

 /**
 * gegl_cl_is_accelerated:
 *
 * Check if OpenCL is enabled.
 *
 * Return value: True if OpenCL is initialized and enabled
 */
gboolean          gegl_cl_is_accelerated (void);

 /**
 * gegl_cl_disable:
 *
 * Disable OpenCL
 */
void              gegl_cl_disable (void);