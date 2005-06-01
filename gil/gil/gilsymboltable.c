#include <stdio.h>
#include "gilsymboltable.h"

GilSymbolTable *table;

static
void
gil_symbol_table_free_hash_data(gpointer key,
                                gpointer value,
                                gpointer user_data)
{
  /* freeing the symbol frees the name string as well */
  gil_symbol_delete((GilSymbol*)value);  /* value */
}

static
GList *
gil_symbol_table_free_last_local(GList *list)
{
    /* Get the last hash table on list*/
    GList *last = g_list_last(list);
    GHashTable * hash = (GHashTable *)(last->data);

    /* Free the keys and values for each entry in hash table */
    g_hash_table_foreach(hash, gil_symbol_table_free_hash_data,NULL);
    g_hash_table_destroy(hash);

    /* Remove the hash table from the list and free list node */
    list = g_list_remove_link(list,last);
    g_list_free_1(last);
    return list;
}

GilSymbolTable *
gil_symbol_table_new()
{
    GilSymbolTable *table = g_new(GilSymbolTable,1);

    /* Set up for first (= global) scope */
    table->scope = -1;
    table->local_tables = NULL;

    /* Enter the first scope */
    gil_symbol_table_enter_scope(table);

    return table;
}

void
gil_symbol_table_free(GilSymbolTable *table)
{
  GList *list = table->local_tables;

  /* Free all the local hash tables */
  while (list)
    list = gil_symbol_table_free_last_local(list);

  /* Free the symbol table */
  g_free(table);
}

void
gil_symbol_table_enter_scope(GilSymbolTable *table)
{
  /* Create a new hash table */
  GHashTable *hash = g_hash_table_new(g_str_hash,g_str_equal);

  /* Add it to the list of local tables */
  g_list_append(table->local_tables,hash);

  table->scope++;
}

void
gil_symbol_table_exit_scope(GilSymbolTable *table)
{
  /* Free the current scope hash table */
  table->local_tables =
      gil_symbol_table_free_last_local(table->local_tables);

  table->scope--;
}

gint
gil_symbol_table_get_scope(GilSymbolTable *table)
{
  return table->scope;
}

/* Create a new entry in current scope  */
void
gil_symbol_table_insert(GilSymbolTable *table,
                        GilSymbol *symbol)
{
  /* Get the current scope hash table */
  GList *last = g_list_last(table->local_tables);
  GHashTable * hash = (GHashTable *)(last->data);
  gchar * name = gil_symbol_get_name(symbol);

  /* Put symbol in the current hash table */
  g_hash_table_insert(hash,name,symbol);

}

/* Look up symbol, start in innermost scope. */
GilSymbol *
gil_symbol_table_lookup(GilSymbolTable *table,
                        gchar * name)
{
  GList *list = g_list_last(table->local_tables);
  GHashTable *hash = NULL;
  GilSymbol *symbol = NULL;

  /* Loop over scopes, starting with last */
  while (list)
  {
    /* Look for the name in this scope */
    hash = (GHashTable*)(list->data);
    symbol = (GilSymbol*)g_hash_table_lookup(hash,name);

    /* If found pass it back */
    if(symbol)
      return symbol;

    /* Go to the previous scope */
    list = g_list_previous(list);
  }

  return NULL;
}

/* Look up variable in the current scope only. */
GilSymbol *
gil_symbol_table_lookup_current(GilSymbolTable *table,
                                gchar * name)
{
  GList *last = g_list_last(table->local_tables);
  GHashTable *hash = (GHashTable *)(last->data);
  return (GilSymbol *)g_hash_table_lookup(hash,name);
}
