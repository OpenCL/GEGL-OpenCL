#ifndef __GIL_BLOCK_H__
#define __GIL_BLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-statement.h" 

#define GIL_TYPE_BLOCK               (gil_block_get_type ())
#define GIL_BLOCK(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_BLOCK, GilBlock))
#define GIL_BLOCK_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_BLOCK, GilBlockClass))
#define GIL_IS_BLOCK(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_BLOCK))
#define GIL_IS_BLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_BLOCK))
#define GIL_BLOCK_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_BLOCK, GilBlockClass))

#ifndef __TYPEDEF_GIL_BLOCK__
#define __TYPEDEF_GIL_BLOCK__
typedef struct _GilBlock  GilBlock;
#endif

struct _GilBlock 
{
    GilStatement     __parent__;

    /*< private >*/
};

typedef struct _GilBlockClass GilBlockClass;
struct _GilBlockClass 
{
   GilStatementClass __parent__;
};

GType             gil_block_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
