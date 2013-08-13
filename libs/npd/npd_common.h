/*
 * This file is part of n-point image deformation library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#ifndef __NPD_COMMON_H__
#define	__NPD_COMMON_H__

#include <glib.h>

/* opaque types for independency on used display library */
typedef struct _NPDImage    NPDImage;
typedef struct _NPDColor    NPDColor;
typedef struct _NPDDisplay  NPDDisplay;
typedef struct _NPDMatrix   NPDMatrix;

typedef struct _NPDPoint    NPDPoint;
typedef struct _NPDBone     NPDBone;
typedef struct _NPDOverlappingPoints NPDOverlappingPoints;

struct _NPDPoint
{
  gfloat                x;
  gfloat                y;
  gboolean              fixed;
  gfloat               *weight;            /* pointer to weight in array of weights */
  gint                  index;
  NPDBone              *current_bone;
  NPDBone              *reference_bone;
  NPDPoint             *counterpart;
  NPDOverlappingPoints *overlapping_points;
};

struct _NPDBone
{
  gint                  num_of_points;
  NPDPoint             *points;            /* array of points */
  gfloat               *weights;           /* array of weights */
};

struct _NPDOverlappingPoints
{
  gint                  num_of_points;
  NPDPoint             *representative;    /* pointer to representative of cluster */
  NPDPoint            **points;            /* array of pointers to points */
};

typedef struct
{
  gint                  num_of_bones;
  gint                  num_of_overlapping_points;
  gboolean              ASAP;              /* don't change directly!
                                            * use npd_set_deformation_type function */
  gboolean              MLS_weights;       /* don't change directly!
                                            * use npd_set_deformation_type function */
  gfloat                MLS_weights_alpha;
  NPDBone              *current_bones;     /* array of current bones */
  NPDBone              *reference_bones;   /* array of reference bones */
  NPDOverlappingPoints *list_of_overlapping_points; /* array of overlapping points */
} NPDHiddenModel;

typedef struct
{
  NPDPoint              point;
  NPDOverlappingPoints *overlapping_points; /* pointer to overlapping points */
} NPDControlPoint;

typedef struct
{
  gint                  control_point_radius;
  gboolean              control_points_visible;
  gboolean              mesh_visible;
  gboolean              texture_visible;
  gint                  mesh_square_size;
  GArray               *control_points;     /* GArray of control points */
  NPDHiddenModel       *hidden_model;
  NPDImage             *reference_image;
  NPDDisplay           *display;
} NPDModel;

#define npd_init(set_pixel, get_pixel)\
npd_set_pixel_color = set_pixel;\
npd_get_pixel_color = get_pixel

void             npd_init_model                 (NPDModel        *model);
void             npd_destroy_hidden_model       (NPDHiddenModel  *model);
void             npd_destroy_model              (NPDModel        *model);

NPDControlPoint *npd_add_control_point          (NPDModel        *model,
                                                 NPDPoint        *coord);
void             npd_remove_control_point       (NPDModel        *model,
                                                 NPDControlPoint *control_point);
void             npd_remove_all_control_points  (NPDModel        *model);
void             npd_set_control_point_weight   (NPDControlPoint *cp,
                                                 gfloat           weight);
gboolean         npd_equal_coordinates          (NPDPoint        *p1,
                                                 NPDPoint        *p2);
gboolean         npd_equal_coordinates_epsilon  (NPDPoint        *p1,
                                                 NPDPoint        *p2,
                                                 gfloat           epsilon);
NPDControlPoint *npd_get_control_point_at       (NPDModel        *model,
                                                 NPDPoint        *coord);
void             npd_create_list_of_overlapping_points
                                                (NPDHiddenModel  *model);
void             add_point_to_suitable_cluster  (GHashTable      *coords_to_cluster,
                                                 NPDPoint        *point,
                                                 GPtrArray       *list_of_overlapping_points);
void             npd_set_overlapping_points_weight
                                                (NPDOverlappingPoints *op,
                                                 gfloat           weight);
void             npd_set_point_coordinates      (NPDPoint        *target,
                                                 NPDPoint        *source);
void             npd_set_deformation_type       (NPDModel        *model,
                                                 gboolean         ASAP,
                                                 gboolean         MLS_weights);
void             npd_compute_MLS_weights        (NPDModel        *model);
void             npd_reset_weights              (NPDHiddenModel  *hidden_model);

void             npd_print_hidden_model         (NPDHiddenModel  *hm,
                                                 gboolean         print_bones,
                                                 gboolean         print_overlapping_points);
void             npd_print_bone                 (NPDBone         *bone);
void             npd_print_point                (NPDPoint        *point,
                                                 gboolean         print_details);
void             npd_print_overlapping_points   (NPDOverlappingPoints *op);
#endif	/* __NPD_COMMON_H__ */