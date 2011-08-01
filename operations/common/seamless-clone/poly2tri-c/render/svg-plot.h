/*
 * This file is a part of Poly2Tri-C - The C port of the Poly2Tri library
 * Porting to C done by (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SVG_PLOT_H
#define	SVG_PLOT_H

#ifdef	__cplusplus
extern "C"
{
#endif

#include "../refine/refine.h"

#define PLOT_LINE_WIDTH 0.40
#define ARROW_SIDE_ANGLE (M_PI / 180 * 30)
#define ARROW_HEAD_SIZE 2.0

#define X_SCALE 3.
#define Y_SCALE 3.
#define X_TRANSLATE 500.
#define Y_TRANSLATE 500.

void
p2tr_plot_svg_plot_group_start (const gchar *Name, FILE* outfile);

void
p2tr_plot_svg_plot_group_end (FILE* outfile);

void
p2tr_plot_svg_plot_line (gdouble x1, gdouble y1, gdouble x2, gdouble y2, const gchar *color, FILE* outfile);

void
p2tr_plot_svg_plot_arrow (gdouble x1, gdouble y1, gdouble x2, gdouble y2, const gchar* color, FILE* outfile);

void
p2tr_plot_svg_fill_triangle (gdouble x1, gdouble y1, gdouble x2, gdouble y2, gdouble x3, gdouble y3, const gchar *color, FILE* outfile);

void
p2tr_plot_svg_fill_point (gdouble x1, gdouble y1, const gchar* color, FILE* outfile);

void
p2tr_plot_svg_plot_circle (gdouble xc, gdouble yc, gdouble R, const gchar* color, FILE* outfile);

void
p2tr_plot_svg_plot_end (FILE* outfile);

void
p2tr_plot_svg_plot_init (FILE* outfile);

void
p2tr_plot_svg_plot_edge (P2tREdge *self, const gchar* color, FILE* outfile);

void
p2tr_plot_svg_plot_triangle (P2tRTriangle *self, const gchar* color, FILE* outfile);

#define r() (10+(rand () % 91))

void
p2tr_plot_svg (P2tRTriangulation *T, FILE* outfile);

#ifdef	__cplusplus
}
#endif

#endif	/* SVG_PLOT_H */

