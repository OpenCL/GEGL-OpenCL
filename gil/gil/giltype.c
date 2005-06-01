
#include <stdio.h>
#include "giltype.h"

GilType *
gil_type_simple_new(GilTypeCons cons)
{
    GilType * type = g_new(GilType,1);
    /* set cons, dont use the info field for simple types */
    type->cons = cons;
    return type;
}

GilType *
gil_type_array_new(GilType *element_type)
{
    GilType * type = g_new(GilType,1);
    type->cons = GIL_TYPE_ARRAY;
    type->info.array.element_type = element_type;
    return type;
}

GilType *
gil_type_function_new(GilType * return_type,
                      GList *arguments)
{
    GilType * type = g_new(GilType,1);
    type->cons = GIL_TYPE_FUNCTION;
    type->info.function.return_type = return_type;
    type->info.function.arguments = arguments;
    return type;
}

GilType *
gil_type_record_new(GList * fields)
{
    GilType * type = g_new(GilType,1);
    type->cons = GIL_TYPE_RECORD;
    type->info.record.fields = fields;
    return type;
}

/* Create a product entry and add to product list */
GList *
gil_type_product_entry_append(gchar *name,
                              GilType *type,
                              GList *list)
{
    GilProductEntry * entry = g_new(GilProductEntry,1);
    entry->name = name;
    entry->type = type;
    return g_list_append(list,entry);
}

void
gil_type_print(GilType *type)
{
    printf("not implemented yet\n");
}
