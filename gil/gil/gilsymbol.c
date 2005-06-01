#include "config.h"

#include <stdio.h>
#include "gilsymbol.h"

GilSymbol * gil_symbol_new(gchar * name,
                           GilSymbolKind kind,
                           GilType *type,
                           gint scope)
{
   GilSymbol *symbol = g_new(GilSymbol,1);
   symbol->name = g_strdup(name);
   symbol->kind = kind;
   symbol->type = type;
   symbol->scope = scope;
   return symbol;
}

void gil_symbol_delete(GilSymbol * symbol)
{
   g_free(symbol->name);
   g_free(symbol);
}

gchar * gil_symbol_get_name(GilSymbol *symbol)
{
    return symbol->name;
}

GilSymbolKind gil_symbol_get_kind(GilSymbol *symbol)
{
    return symbol->kind;
}

GilType * gil_symbol_get_type(GilSymbol *symbol)
{
    return symbol->type;
}

gint gil_symbol_get_scope(GilSymbol *symbol)
{
    return symbol->scope;
}

