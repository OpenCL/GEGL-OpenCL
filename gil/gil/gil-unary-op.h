#ifndef __GIL_UNARY_OP_H__
#define __GIL_UNARY_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-expression.h" 

#define GIL_TYPE_UNARY_OP               (gil_unary_op_get_type ())
#define GIL_UNARY_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_UNARY_OP, GilUnaryOp))
#define GIL_UNARY_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_UNARY_OP, GilUnaryOpClass))
#define GIL_IS_UNARY_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_UNARY_OP))
#define GIL_IS_UNARY_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_UNARY_OP))
#define GIL_UNARY_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_UNARY_OP, GilUnaryOpClass))

#ifndef __TYPEDEF_GIL_UNARY_OP__
#define __TYPEDEF_GIL_UNARY_OP__
typedef struct _GilUnaryOp  GilUnaryOp;
#endif

struct _GilUnaryOp 
{
    GilExpression     __parent__;

    /*< private >*/
    GilUnaryOpType op;
};

typedef struct _GilUnaryOpClass GilUnaryOpClass;
struct _GilUnaryOpClass 
{
   GilExpressionClass __parent__;
};

GType             gil_unary_op_get_type     (void);
GilUnaryOpType    gil_unary_op_get_op       (GilUnaryOp * self);
void              gil_unary_op_set_op       (GilUnaryOp * self, 
                                             GilUnaryOpType op);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
