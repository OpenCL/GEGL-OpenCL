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
  GEGL_DEBUG_MISC            = 1 << 6,
  GEGL_DEBUG_INVALIDATION    = 1 << 7
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
  { "invalidation",  GEGL_DEBUG_INVALIDATION},
  { "all",           GEGL_DEBUG_PROCESS|
                     GEGL_DEBUG_BUFFER_LOAD|
                     GEGL_DEBUG_BUFFER_SAVE|
                     GEGL_DEBUG_TILE_BACKEND|
                     GEGL_DEBUG_PROCESSOR|
                     GEGL_DEBUG_CACHE},
};
#endif /* GEGL_ENABLE_DEBUG */

#if defined(__cplusplus) && defined(GEGL_ISO_CXX_VARIADIC_MACROS)
#  define GEGL_ISO_VARIADIC_MACROS 1
#endif

#ifdef GEGL_ENABLE_DEBUG

#if defined(GEGL_ISO_VARIADIC_MACROS)

/* Use the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
#define GEGL_NOTE(type,...)               G_STMT_START {        \
        if (gegl_debug_flags & type)                            \
	{                                                       \
	  gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
          g_message ("[" #type "] " G_STRLOC ": %s",_fmt);      \
          g_free (_fmt);                                        \
	}                                                       \
                                                } G_STMT_END

#define GEGL_TIMESTAMP(type,...)             G_STMT_START {     \
        if (gegl_debug_flags & type)                            \
	{                                                       \
	  gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
          g_message ("[" #type "]" " %li:"  G_STRLOC ": %s",    \
                       gegl_get_timestamp(), _fmt);             \
          g_free (_fmt);                                        \
	}                                                       \
                                                   } G_STMT_END

#elif defined(GEGL_GNUC_VARIADIC_MACROS)

#define GEGL_NOTE(type,format,a...)          G_STMT_START {     \
        if (gegl_debug_flags & type)                            \
          { g_message ("[" #type "] " G_STRLOC ": "             \
                       format, ##a); }                          \
                                                } G_STMT_END

#define GEGL_TIMESTAMP(type,format,a...)        G_STMT_START {  \
        if (gegl_debug_flags & type)                            \
          { g_message ("[" #type "]" " %li:"  G_STRLOC ": "     \
                       format, gegl_get_timestamp(), ##a); }    \
                                                   } G_STMT_END

#else

#include <stdarg.h>

static const gchar *
gegl_lookup_debug_string  (guint type)
{
  const gchar *key = "!INVALID!";
  guint i = 0;

  if (type == GEGL_DEBUG_MISC)
    {
      key = "misc";
    }
  else
    {
      while (g_strcmp0 (gegl_debug_keys[i].key, "all") != 0)
        {
          if (gegl_debug_keys[i].value == type)
            {
              key = gegl_debug_keys[i].key;
              break;
            }
        }
    }

  return key;
}

static inline void
GEGL_NOTE (guint type, const char *format, ...)
{
  va_alist args;

  if (gegl_debug_flags & type)
  {
    gchar *formatted;
    va_start (args, format);
    formatted = g_strdup_printf (format, args);
    g_message ("[ %s ] " G_STRLOC ": %s",
               gegl_lookup_debug_string (type), formatted);
    g_free (formatted);
    va_end (args);
  }
}

static inline void
GEGL_TIMESTAMP (guint type, const char *format, ...)
{
  va_alist args;
  
  if (gegl_debug_flags & type)
  {
    gchar *formatted;
    va_start (args, format);
    formatted = g_strdup_printf (format, args);
    g_message ("[ %s ] %li: " G_STRLOC ": %s",
               gegl_lookup_debug_string (type), gegl_get_timestamp(),
               formatted);
    g_free (formatted);
    va_end (args);
  }
}

#endif

#define GEGL_MARK()      GEGL_NOTE(GEGL_DEBUG_MISC, "== mark ==")
#define GEGL_DBG(x) { a }

#else /* !GEGL_ENABLE_DEBUG */

#if defined(GEGL_ISO_VARIADIC_MACROS)

#define GEGL_NOTE(type,...)
#define GEGL_TIMESTAMP(type,...)

#elif defined(GEGL_GNUC_VARIADIC_MACROS)

#define GEGL_NOTE(type,format,a...)
#define GEGL_TIMESTAMP(type,format,a...)

#else

static inline void
GEGL_NOTE (guint type, const char *format, ...)
{
  return;
}

static inline void
GEGL_TIMESTAMP (guint type, const char *format, ...)
{
  return;
}

#endif

#define GEGL_MARK()
#define GEGL_DBG(x)

#endif /* GEGL_ENABLE_DEBUG */

extern guint gegl_debug_flags;

G_END_DECLS

#endif /* __GEGL_DEBUG_H__  */
