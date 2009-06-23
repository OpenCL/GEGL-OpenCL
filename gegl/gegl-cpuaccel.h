/* LIBGEGL - The GEGL Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GEGL_CPU_ACCEL_H__
#define __GEGL_CPU_ACCEL_H__

G_BEGIN_DECLS


typedef enum
{
  GEGL_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  GEGL_CPU_ACCEL_X86_MMX     = 0x01000000,
  GEGL_CPU_ACCEL_X86_3DNOW   = 0x40000000,
  GEGL_CPU_ACCEL_X86_MMXEXT  = 0x20000000,
  GEGL_CPU_ACCEL_X86_SSE     = 0x10000000,
  GEGL_CPU_ACCEL_X86_SSE2    = 0x08000000,
  GEGL_CPU_ACCEL_X86_SSE3    = 0x02000000,

  /* powerpc accelerations */
  GEGL_CPU_ACCEL_PPC_ALTIVEC = 0x04000000
} GeglCpuAccelFlags;


GeglCpuAccelFlags  gegl_cpu_accel_get_support (void);


/* for internal use only */
void               gegl_cpu_accel_set_use     (gboolean use);


G_END_DECLS

#endif  /* __GEGL_CPU_ACCEL_H__ */
