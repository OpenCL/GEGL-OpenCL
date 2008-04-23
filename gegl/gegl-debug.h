#ifndef __GEGL_DEBUG_H__
#define __GEGL_DEBUG_H__

#include <glib.h>
#include "gegl-init.h"

G_BEGIN_DECLS

typedef enum {
  GEGL_DEBUG_PROCESS         = 1 << 0,
  GEGL_DEBUG_BUFFER_LOAD     = 1 << 1,
  GEGL_DEBUG_BUFFER_SAVE     = 1 << 2,
  GEGL_DEBUG_TILE_BACKEND    = 1 << 3,
  GEGL_DEBUG_PROCESSOR       = 1 << 4,
  GEGL_DEBUG_CACHE           = 1 << 5,
} GeglDebugFlag;

/* only compiled in from gegl-init.c but kept here to
 * make it easier to update and keep in sync with the
 * flags
 */
#ifdef __GEGL_INIT_C
static const GDebugKey gegl_debug_keys[] = {
  { "process",       GEGL_DEBUG_PROCESS},
  { "cache",         GEGL_DEBUG_CACHE},
  { "buffer-load",   GEGL_DEBUG_BUFFER_LOAD},
  { "buffer-save",   GEGL_DEBUG_BUFFER_SAVE},
  { "tile-backend",  GEGL_DEBUG_TILE_BACKEND},
  { "processor",     GEGL_DEBUG_PROCESSOR},
  { "all",           GEGL_DEBUG_PROCESS|
                     GEGL_DEBUG_BUFFER_LOAD|
                     GEGL_DEBUG_BUFFER_SAVE|
                     GEGL_DEBUG_TILE_BACKEND|
                     GEGL_DEBUG_PROCESSOR|
                     GEGL_DEBUG_CACHE},
};
#endif /* GEGL_ENABLE_DEBUG */

#ifdef GEGL_ENABLE_DEBUG

#ifdef __GNUC_
#define GEGL_NOTE(type,x,a...)               G_STMT_START {     \
        if (gegl_debug_flags & GEGL_DEBUG_##type)               \
          { g_message ("[" #type "] " G_STRLOC ": " x, ##a); }  \
                                                } G_STMT_END

#define GEGL_TIMESTAMP(type,x,a...)             G_STMT_START {  \
        if (gegl_debug_flags & GEGL_DEBUG_##type)               \
          { g_message ("[" #type "]" " %li:"  G_STRLOC ": "     \
                       x, gegl_get_timestamp(), ##a); }         \
                                                   } G_STMT_END
#else
/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
#define GEGL_NOTE(type,...)               G_STMT_START {        \
        if (gegl_debug_flags & GEGL_DEBUG_##type)               \
	{                                                       \
	  gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
          g_message ("[" #type "] " G_STRLOC ": %s",_fmt);      \
          g_free (_fmt);                                        \
	}                                                       \
                                                } G_STMT_END

#define GEGL_TIMESTAMP(type,...)             G_STMT_START {     \
        if (gegl_debug_flags & GEGL_DEBUG_##type)               \
	{                                                       \
	  gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
          g_message ("[" #type "]" " %li:"  G_STRLOC ": %s",    \
                       gegl_get_timestamp(), _fmt);             \
          g_free (_fmt);                                        \
	}                                                       \
                                                   } G_STMT_END
#endif

#define GEGL_MARK()      GEGL_NOTE(MISC, "== mark ==")
#define GEGL_DBG(x) { a }

#else /* !GEGL_ENABLE_DEBUG */

#define GEGL_NOTE(type,...)
#define GEGL_MARK()
#define GEGL_DBG(x)
#define GEGL_TIMESTAMP(type,...)

#endif /* GEGL_ENABLE_DEBUG */

extern guint gegl_debug_flags;

G_END_DECLS

#endif /* __GEGL_DEBUG_H__  */
