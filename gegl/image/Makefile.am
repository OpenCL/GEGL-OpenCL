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

IMAGE_SRC = \
	gegl-buffer.c \
	gegl-buffer-double.c \
	gegl-cache-entry.c \
	gegl-cache.c \
	gegl-normalizer.c \
	gegl-normalizer-mult.c \
	gegl-sample-model.c \
	gegl-component-sample-model.c \
	gegl-object.c \
	gegl-utils.c \
	gegl-init.c

libimage_la_SOURCES = \
	$(IMAGE_SRC)

INCLUDES = \
	-I$(top_srcdir)	\
	-I../ \
	@DEP_CFLAGS@


