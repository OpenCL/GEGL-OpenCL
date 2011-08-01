/* 
 * File:   mesh-render.h
 * Author: Barak
 *
 * Created on 1 אוגוסט 2011, 15:37
 */

#ifndef MESH_RENDER_H
#define	MESH_RENDER_H

#ifdef	__cplusplus
extern "C"
{
#endif

typedef struct {
  /* Minimal X and Y coordinates to start sampling at */
  gdouble min_x, min_y;
  /* Size of a step in each axis */
  gdouble step_x, step_y;
  /* The amount of samples desired in each axis */
  guint x_samples, y_samples;
  /* The amount of channels per pixel, both in destination buffer and in the
   * colors returned from the matching point-to-color function */
  guint cpp;
} P2tRImageConfig;

typedef void (*P2tRPointToColorFunc) (P2tRPoint* point, gfloat *dest, gpointer user_data);

void p2tr_test_point_to_color (P2tRPoint* point, gfloat *dest, gpointer user_data);

void
p2tr_mesh_render_scanline (P2tRTriangulation    *T,
                           gfloat               *dest,
                           P2tRImageConfig      *config,
                           P2tRPointToColorFunc  pt2col,
                           gpointer              pt2col_user_data);




#ifdef	__cplusplus
}
#endif

#endif	/* MESH_RENDER_H */

