#ifndef __GIL_BINARY_OP_H__
#define __GIL_BINARY_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-expression.h"

#define GIL_TYPE_BINARY_OP               (gil_binary_op_get_type ())
#define GIL_BINARY_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_BINARY_OP, GilBinaryOp))
#define GIL_BINARY_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_BINARY_OP, GilBinaryOpClass))
#define GIL_IS_BINARY_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_BINARY_OP))
#define GIL_IS_BINARY_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_BINARY_OP))
#define GIL_BINARY_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_BINARY_OP, GilBinaryOpClass))

#ifndef __TYPEDEF_GIL_BINARY_OP__
#define __TYPEDEF_GIL_BINARY_OP__
typedef struct _GilBinaryOp  GilBinaryOp;
#endif

struct _GilBinaryOp
{
    GilExpression     __parent__;

    /*< private >*/
    GilBinaryOpType op;
};

typedef struct _GilBinaryOpClass GilBinaryOpClass;
struct _GilBinaryOpClass
{
   GilExpressionClass __parent__;
};

GType             gil_binary_op_get_type     (void);

GilBinaryOpType   gil_binary_op_get_op       (GilBinaryOp * self);
void              gil_binary_op_set_op       (GilBinaryOp * self,
                                              GilBinaryOpType op);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
