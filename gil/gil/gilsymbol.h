#ifndef __GIL_SYMBOL_H__
#define __GIL_SYMBOL_H__

#include <glib-2.0/glib.h>
#include "giltype.h"

typedef enum
{
   GIL_SYMBOL_CONSTANT,
   GIL_SYMBOL_VARIABLE,
   GIL_SYMBOL_TYPE,
   GIL_SYMBOL_FUNCTION
} GilSymbolKind;

typedef struct _GilSymbol GilSymbol;
struct _GilSymbol
{
  gchar *name;
	GilSymbolKind kind;
	GilType *type;
	gint scope;
};

GilSymbol * gil_symbol_new(gchar *name, GilSymbolKind kind, GilType *type, gint scope);
gchar * gil_symbol_get_name(GilSymbol *symbol);
GilSymbolKind gil_symbol_get_kind(GilSymbol *symbol);
GilType * gil_symbol_get_type(GilSymbol *symbol);
gint gil_symbol_get_scope(GilSymbol *symbol);
#endif
