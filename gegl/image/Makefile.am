#
#   This file is part of GEGL.
#
#   GEGL is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    Foobar is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Foobar; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  Copyright 2003 Daniel S. Rogers
#
#

noinst_LTLIBRARIES = libimage.la

IMAGE_sources = \
	gegl-buffer-double.c		\
	gegl-buffer.c			\
	gegl-cache-entry.c		\
	gegl-cache-store.c		\
	gegl-cache.c			\
	gegl-component-sample-model.c	\
	gegl-entry-record.c		\
	gegl-heap-cache-store.c		\
	gegl-heap-cache.c		\
	gegl-normalizer-mult.c		\
	gegl-normalizer.c		\
	gegl-null-cache-store.c		\
	gegl-sample-model.c		\
	gegl-swap-cache-store.c		\
	gegl-swap-cache.c

IMAGE_headers = \
	gegl-image-types.h		\
	\
	gegl-buffer-double.h		\
	gegl-buffer.h			\
	gegl-cache-entry.h		\
	gegl-cache-store.h		\
	gegl-cache.h			\
	gegl-component-sample-model.h	\
	gegl-entry-record.h		\
	gegl-heap-cache-store.h		\
	gegl-heap-cache.h		\
	gegl-normalizer-mult.h		\
	gegl-normalizer.h		\
	gegl-null-cache-store.h		\
	gegl-sample-model.h		\
	gegl-swap-cache-store.h		\
	gegl-swap-cache.h

IMAGE_unused_sources = \
	gegl-color-model.c		\
	gegl-color-model.h		\
	gegl-color-space-impl.h		\
	gegl-tile.h			\
	gegl-tile-iterator.h

libimage_la_SOURCES = $(IMAGE_sources) $(IMAGE_headers)

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/gegl	\
	@DEP_CFLAGS@
