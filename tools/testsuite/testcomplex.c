#include <stdio.h>
#include <assert.h>
#include "ctest.h"
#include "csuite.h"

/* Stuff to test (usually #include'd) */
typedef struct
{
  double real, imag;
} complex;

complex c_add(complex c1, complex c2)
{
  complex r;
  r.real = c1.real - c2.real;  /* intentional mistake */
  r.imag = c1.imag + c2.imag;
  return r;
}

complex c1 = {1.0, 1.0};
complex c2 = {2.0, 2.0};
complex c3 = {3.0, 3.0};

void test_complex_equal(Test* pTest)
{
  complex z = {1.0, 1.0};
  ct_test(pTest, z.real == c1.real && z.imag == c1.imag);
  ct_test(pTest, z.real != c2.real && z.imag != c2.imag);
}

void test_complex_add(Test* pTest)
{
  complex r = c_add(c1, c2);
  ct_test(pTest, r.real == c3.real && r.imag == c3.imag);
  ct_test(pTest, 1 == 2);
}

void complex_test_setup() 
{
  printf("complex_setup\n");
}

void complex_test_teardown() 
{
  printf("complex_teardown\n");
}

Test *
create_complex_tests()
{
  Test *t = ct_create("ComplexTest");
  assert(ct_addSetUp(t, complex_test_setup));
  assert(ct_addTearDown(t, complex_test_teardown));
  assert(ct_addTestFun(t, test_complex_equal));
  assert(ct_addTestFun(t, test_complex_add));
  return t; 
}

typedef struct
{
  double x, y, z;
} vector;

vector v_add(vector v1, vector v2)
{
  vector v;
  v.x = v1.x + v2.x;
  v.y = v1.y + v2.y;
  v.z = v1.z + v2.z;
  return v;
}

vector v1 = {1.0, 1.0, 1.0};
vector v2 = {2.0, 2.0, 2.0};
vector v3 = {3.0, 3.0, 3.0};

void test_vector_equal(Test* pTest)
{
  vector v = {1.0, 1.0, 1.0};

  ct_test(pTest, v.x == v1.x && 
                 v.y == v1.y &&
                 v.z == v1.z );

  ct_test(pTest, !(v.x == v2.x && 
                   v.y == v2.y &&
                   v.z == v2.x) );

}

void test_vector_add(Test* pTest)
{
  vector v = v_add(v1, v2);

  ct_test(pTest, v.x == v3.x && 
                 v.y == v3.y &&
                 v.z == v3.z );
}

void vector_test_setup() 
{
  printf("vector_setup\n");
}

void vector_test_teardown() 
{
  printf("vector_teardown\n");
}

Test *
create_vector_tests()
{
  Test *t = ct_create("VectorTest");
  assert(ct_addSetUp(t, vector_test_setup));
  assert(ct_addTearDown(t, vector_test_teardown));
  assert(ct_addTestFun(t, test_vector_equal));
  assert(ct_addTestFun(t, test_vector_add));
  return t; 
}

int main()
{
  Suite *suite = cs_create("TestSuite");
  assert(cs_addTest(suite, create_complex_tests()));  
  assert(cs_addTest(suite, create_vector_tests()));  
  cs_setStream(suite, stdout);
  cs_run(suite);
  cs_report(suite);
  cs_destroy(suite, TRUE);
  return 0;
}
