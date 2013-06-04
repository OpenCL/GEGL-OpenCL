#ifndef __GEGL_GRAPH_TRAVERSAL_H__
#define __GEGL_GRAPH_TRAVERSAL_H__

typedef struct _GeglGraphTraversal GeglGraphTraversal;

void gegl_graph_dfs_dump (GeglNode *);

GeglGraphTraversal *gegl_graph_build (GeglNode *node);
void gegl_graph_rebuild (GeglGraphTraversal *path, GeglNode *node);
void gegl_graph_free (GeglGraphTraversal *path);

void gegl_graph_prepare (GeglGraphTraversal *path);
void gegl_graph_prepare_request (GeglGraphTraversal *path, const GeglRectangle *roi);
GeglBuffer *gegl_graph_process (GeglGraphTraversal *path);
GeglRectangle gegl_graph_get_bounding_box (GeglGraphTraversal *path);

#endif /* __GEGL_GRAPH_TRAVERSAL_H__ */
