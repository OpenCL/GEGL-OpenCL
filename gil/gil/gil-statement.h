#ifndef __GIL_STATEMENT_H__
#define __GIL_STATEMENT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-node.h"

#define GIL_TYPE_STATEMENT               (gil_statement_get_type ())
#define GIL_STATEMENT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_STATEMENT, GilStatement))
#define GIL_STATEMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_STATEMENT, GilStatementClass))
#define GIL_IS_STATEMENT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_STATEMENT))
#define GIL_IS_STATEMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_STATEMENT))
#define GIL_STATEMENT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_STATEMENT, GilStatementClass))

#ifndef __TYPEDEF_GIL_STATEMENT__
#define __TYPEDEF_GIL_STATEMENT__
typedef struct _GilStatement  GilStatement;
#endif

struct _GilStatement
{
    GilNode     __parent__;

    /*< private >*/
    gint position;
};

typedef struct _GilStatementClass GilStatementClass;
struct _GilStatementClass
{
   GilNodeClass __parent__;
};

GType             gil_statement_get_type                  (void);

gint gil_statement_get_position (GilStatement * self);
void gil_statement_set_position (GilStatement * self,
                                 gint position);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
