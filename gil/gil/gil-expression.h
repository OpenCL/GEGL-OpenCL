#ifndef __GIL_EXPRESSION_H__
#define __GIL_EXPRESSION_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-node.h"

#define GIL_TYPE_EXPRESSION               (gil_expression_get_type ())
#define GIL_EXPRESSION(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_EXPRESSION, GilExpression))
#define GIL_EXPRESSION_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_EXPRESSION, GilExpressionClass))
#define GIL_IS_EXPRESSION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_EXPRESSION))
#define GIL_IS_EXPRESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_EXPRESSION))
#define GIL_EXPRESSION_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_EXPRESSION, GilExpressionClass))

#ifndef __TYPEDEF_GIL_EXPRESSION__
#define __TYPEDEF_GIL_EXPRESSION__
typedef struct _GilExpression  GilExpression;
#endif

struct _GilExpression
{
    GilNode     __parent__;

    /*< private >*/
};

typedef struct _GilExpressionClass GilExpressionClass;
struct _GilExpressionClass
{
   GilNodeClass __parent__;
};

GType             gil_expression_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
