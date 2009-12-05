#ifndef __GEGL_SIMD_H__
#define __GEGL_SIMD_H__


#if defined(__GNUC__) && (__GNUC__ >= 4)
#define HAS_G4FLOAT 1
#include <math.h>

typedef float g4float __attribute__ ((vector_size (4*sizeof(float))));

#define g4float_a(a)      ((float *)(&a))
#define g4floatR(a)       g4float_a(a)[0]
#define g4floatG(a)       g4float_a(a)[1]
#define g4floatB(a)       g4float_a(a)[2]
#define g4floatA(a)       g4float_a(a)[3]
#define g4float(a,b,c,d)  ((g4float){a,b,c,d})
#define g4float_all(val)  g4float(val,val,val,val)
#define g4float_zero      g4float_all(0.0)
#define g4float_one       g4float_all(1.0)
#define g4float_half      g4float_all(0.5)
#define g4float_mul(a,val)  g4float_all(val)*(a)

#ifdef USE_SSE

#define g4float_sqrt(v)     __builtin_ia32_sqrtps((v))
#define g4float_max(a,b)    __builtin_ia32_maxps((a,b))
#define g4float_min(a,b)    __builtin_ia32_minps((a,b))
#define g4float_rcp(a,b)    __builtin_ia32_rcpps((v))

#else

#define g4float_sqrt(v)     g4float(sqrt(g4floatR(v)),\
                                    sqrt(g4floatG(v)),\
                                    sqrt(g4floatB(v)),\
                                    sqrt(g4floatA(v)))
#define g4float_rcp(v)      g4float(1.0/(g4floatR(v)),\
                                    1.0/(g4floatG(v)),\
                                    1.0/(g4floatB(v)),\
                                    1.0/(g4floatA(v)))
#define g4float_max(a,b)   g4float(\
                               gfloat4R(a)>gfloat4R(b)?gfloat4R(a):gfloat4R(b),\
                               gfloat4G(a)>gfloat4G(b)?gfloat4G(a):gfloat4G(b),\
                               gfloat4B(a)>gfloat4B(b)?gfloat4B(a):gfloat4B(b),\
                               gfloat4A(a)>gfloat4A(b)?gfloat4A(a):gfloat4A(b))
#define g4float_min(a,b)   g4float(\
                               gfloat4R(a)<gfloat4R(b)?gfloat4R(a):gfloat4R(b),\
                               gfloat4G(a)<gfloat4G(b)?gfloat4G(a):gfloat4G(b),\
                               gfloat4B(a)<gfloat4B(b)?gfloat4B(a):gfloat4B(b),\
                               gfloat4A(a)<gfloat4A(b)?gfloat4A(a):gfloat4A(b))
#endif

#endif

#endif
