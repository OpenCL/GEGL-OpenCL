#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"

#include "gegl-mock-pad-visitor.h"


static void gegl_mock_pad_visitor_class_init (GeglMockPadVisitorClass *klass);
static void gegl_mock_pad_visitor_init       (GeglMockPadVisitor      *self);

static void visit_pad (GeglVisitor *visitor, GeglPad *pad);


G_DEFINE_TYPE (GeglMockPadVisitor, gegl_mock_pad_visitor,
               GEGL_TYPE_VISITOR)


static void
gegl_mock_pad_visitor_class_init (GeglMockPadVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_pad = visit_pad;
}

static void
gegl_mock_pad_visitor_init (GeglMockPadVisitor *self)
{
}

static void
visit_pad (GeglVisitor *visitor,
           GeglPad     *pad)
{
  GEGL_VISITOR_CLASS (gegl_mock_pad_visitor_parent_class)->visit_pad (visitor, pad);

#if 0
  {
    GeglOperation *operation = gegl_pad_get_operation (pad);
    g_print ("Visiting pad %s from op %s\n",
             gegl_pad_get_name (pad),
             gegl_object_get_name (GEGL_OBJECT (operation)));
  }
#endif
}
