#ifndef __GIL_VARIABLE_H__
#define __GIL_VARIABLE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-expression.h" 

#define GIL_TYPE_VARIABLE               (gil_variable_get_type ())
#define GIL_VARIABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_VARIABLE, GilVariable))
#define GIL_VARIABLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_VARIABLE, GilVariableClass))
#define GIL_IS_VARIABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_VARIABLE))
#define GIL_IS_VARIABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_VARIABLE))
#define GIL_VARIABLE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_VARIABLE, GilVariableClass))

#ifndef __TYPEDEF_GIL_VARIABLE__
#define __TYPEDEF_GIL_VARIABLE__
typedef struct _GilVariable  GilVariable;
#endif

struct _GilVariable 
{
    GilExpression     __parent__;

    /*< private >*/
    gchar * name;
    GilType type;
};

typedef struct _GilVariableClass GilVariableClass;
struct _GilVariableClass 
{
   GilExpressionClass __parent__;
};

GType             gil_variable_get_type                  (void);

GilType gil_variable_get_variable_type (GilVariable * self);
void gil_variable_set_variable_type (GilVariable * self, GilType type);

void gil_variable_set_name (GilVariable * self, const gchar * name);
G_CONST_RETURN gchar* gil_variable_get_name (GilVariable * self);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
