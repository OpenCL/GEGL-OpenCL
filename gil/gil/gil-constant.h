#ifndef __GIL_CONSTANT_H__
#define __GIL_CONSTANT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-expression.h" 

#define GIL_TYPE_CONSTANT               (gil_constant_get_type ())
#define GIL_CONSTANT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_CONSTANT, GilConstant))
#define GIL_CONSTANT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_CONSTANT, GilConstantClass))
#define GIL_IS_CONSTANT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_CONSTANT))
#define GIL_IS_CONSTANT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_CONSTANT))
#define GIL_CONSTANT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_CONSTANT, GilConstantClass))

#ifndef __TYPEDEF_GIL_CONSTANT__
#define __TYPEDEF_GIL_CONSTANT__
typedef struct _GilConstant  GilConstant;
#endif

struct _GilConstant 
{
    GilExpression     __parent__;

    /*< private >*/
    GilType  type; 
    GilValue value; 
};

typedef struct _GilConstantClass GilConstantClass;
struct _GilConstantClass 
{
   GilExpressionClass __parent__;
};

GType             gil_constant_get_type                  (void);
GilType           gil_constant_get_const_type (GilConstant * self);
void              gil_constant_set_const_type (GilConstant * self, 
                                               GilType type);
gint              gil_constant_get_int (GilConstant * self);
void              gil_constant_set_int (GilConstant * self, 
                                        gint int_value);
gfloat            gil_constant_get_float (GilConstant * self);
void              gil_constant_set_float (GilConstant * self, 
                                          gfloat float_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
