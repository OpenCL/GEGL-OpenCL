#ifndef __GIL_TYPE_H__
#define __GIL_TYPE_H__

#include <glib.h>

typedef enum
{
   GIL_TYPE_INT,
   GIL_TYPE_FLOAT,
   GIL_TYPE_ARRAY,
   GIL_TYPE_FUNCTION,
   GIL_TYPE_RECORD
} GilTypeCons;

typedef struct _GilType GilType;

typedef struct _GilArrayType GilArrayType;
struct _GilArrayType
{       
  GilType *element_type;  /*element type*/
};

typedef struct _GilFunctionType GilFunctionType;
struct _GilFunctionType
{       
  GilType * return_type;    /*return GilType*/
  GList * arguments;        /*argument GilTypes*/
};

typedef struct _GilRecordType GilRecordType;
struct _GilRecordType
{       
  GList * fields;           /*list of GilProductEntrys*/
};

typedef union _GilTypeInfo GilTypeInfo;
union _GilTypeInfo
{
    GilArrayType array;
    GilFunctionType function;
    GilRecordType record;
};

struct _GilType
{
  GilTypeCons cons;  /*int,float,array,function,record */
  GilTypeInfo info;  /*info based on type above*/
};

typedef struct _GilProductEntry GilProductEntry;
struct _GilProductEntry
{
    gchar *name;        /*names of entries eg "x" "y" in Point{int x; int y;}*/
    GilType *type;      /*entry's regular type */
};

/* Constructors */			
GilType *   gil_type_simple_new(GilTypeCons cons);
GilType *   gil_type_array_new(GilType *element_type);
GilType *   gil_type_function_new(GilType * return_type,
                              GList *arguments);
GilType *   gil_type_record_new(GList * fields);
GList *     gil_type_product_entry_append(gchar *name,
                                          GilType *type,
                                          GList *list);

void	gil_type_print(GilType *type);
#endif
