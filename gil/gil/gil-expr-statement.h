#ifndef __GIL_EXPR_STATEMENT_H__
#define __GIL_EXPR_STATEMENT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-statement.h"

#define GIL_TYPE_EXPR_STATEMENT               (gil_expr_statement_get_type ())
#define GIL_EXPR_STATEMENT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_EXPR_STATEMENT, GilExprStatement))
#define GIL_EXPR_STATEMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_EXPR_STATEMENT, GilExprStatementClass))
#define GIL_IS_EXPR_STATEMENT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_EXPR_STATEMENT))
#define GIL_IS_EXPR_STATEMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_EXPR_STATEMENT))
#define GIL_EXPR_STATEMENT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_EXPR_STATEMENT, GilExprStatementClass))

#ifndef __TYPEDEF_GIL_EXPR_STATEMENT__
#define __TYPEDEF_GIL_EXPR_STATEMENT__
typedef struct _GilExprStatement  GilExprStatement;
#endif

struct _GilExprStatement
{
    GilStatement     __parent__;

    /*< private >*/
};

typedef struct _GilExprStatementClass GilExprStatementClass;
struct _GilExprStatementClass
{
   GilStatementClass __parent__;
};

GType             gil_expr_statement_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
