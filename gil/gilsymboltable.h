#ifndef __GIL_SYMBOL_TABLE_H__
#define __GIL_SYMBOL_TABLE_H__

#include <glib.h>
#include "gilsymbol.h"

typedef struct _GilSymbolTable GilSymbolTable;
struct _GilSymbolTable
{
	gint scope;		       
	GList *local_tables;  /*a table for each scope*/
};

GilSymbolTable * gil_symbol_table_new(void);
void             gil_symbol_table_free(GilSymbolTable *table);
void             gil_symbol_table_insert(GilSymbolTable *table,
                                         GilSymbol * symbol);
GilSymbol *      gil_symbol_table_lookup(GilSymbolTable *table, 
                                         gchar * name);
GilSymbol *      gil_symbol_table_lookup_current(GilSymbolTable *table, 
                                                 gchar * name);
void             gil_symbol_table_enter_scope(GilSymbolTable *table);
void             gil_symbol_table_exit_scope(GilSymbolTable *table);
gint             gil_symbol_table_get_scope(GilSymbolTable *table);
#endif
