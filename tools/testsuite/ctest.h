/* ctest.h
 *
 *              Defines a test framework for C projects.
 */
#ifndef CTEST_H
#define CTEST_H

#include <stdio.h>
#define bool int
#ifndef TRUE
#define TRUE 1 
#endif
#ifndef FALSE 
#define FALSE 0 
#endif

#define ct_test(test, cond) \
        ct_do_test(test, #cond, cond, __FILE__, __LINE__)
#define ct_fail(test, str)  \
        ct_do_fail(test, str, __FILE__, __LINE__)

typedef struct Test Test;
typedef void (*TestFunc)(Test*);
typedef void (*FixtureFunc)(Test*);

Test* ct_create(const char* name);
void ct_destroy(Test* pTest);

const char* ct_getName(Test* pTest);
long ct_getNumPassed(Test* pTest);
long ct_getNumFailed(Test* pTest);
long ct_getNumTests(Test* pTest);
FILE* ct_getStream(Test* pTest);
void ct_setStream(Test* pTest, FILE* stream);

bool ct_addTestFun(Test* pTest, TestFunc tfun);
bool ct_addSetUp(Test* pTest, FixtureFunc setup);
bool ct_addTearDown(Test* pTest, FixtureFunc teardown);
void ct_succeed(Test* pTest);
long ct_run(Test* pTest);
long ct_report(Test* pTest);
void ct_reset(Test* pTest);

/* Not intended for end-users: */
void ct_do_test(Test* pTest, const char* str,
                                bool cond, const char* file, long line);
void ct_do_fail(Test* pTest, const char* str,
                                const char* file, long line);

#endif
