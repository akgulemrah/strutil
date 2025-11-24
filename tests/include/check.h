#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*test_func_t)(void);
typedef void (*fixture_func_t)(void);

typedef struct {
    test_func_t func;
    const char* name;
} TestInfo;

typedef struct {
    char* name;
    TestInfo tests[100];
    int num_tests;
    fixture_func_t setup;
    fixture_func_t teardown;
} TCase;

typedef struct {
    char* name;
    TCase* tcases[100];
    int num_tcases;
} Suite;

typedef struct {
    Suite* suite;
    int failed;
    int total;
} SRunner;

#define START_TEST(name) void name(void)
#define END_TEST

#define ck_assert_ptr_ne(a, b) do { if ((a) == (b)) { fprintf(stderr, "%s:%d: Assertion '%s != %s' failed\n", __FILE__, __LINE__, #a, #b); exit(1); } } while(0)
#define ck_assert_ptr_eq(a, b) do { if ((a) != (b)) { fprintf(stderr, "%s:%d: Assertion '%s == %s' failed\n", __FILE__, __LINE__, #a, #b); exit(1); } } while(0)
#define ck_assert_int_eq(a, b) do { if ((a) != (b)) { fprintf(stderr, "%s:%d: Assertion '%s == %s' failed (Got %d, Expected %d)\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); exit(1); } } while(0)
#define ck_assert_int_ge(a, b) do { if ((a) < (b)) { fprintf(stderr, "%s:%d: Assertion '%s >= %s' failed\n", __FILE__, __LINE__, #a, #b); exit(1); } } while(0)
#define ck_assert(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: Assertion '%s' failed\n", __FILE__, __LINE__, #expr); exit(1); } } while(0)
#define ck_assert_msg(expr, ...) do { if (!(expr)) { fprintf(stderr, "%s:%d: Assertion '%s' failed: ", __FILE__, __LINE__, #expr); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } } while(0)
#define ck_assert_str_eq(a, b) do { if (strcmp(a, b) != 0) { fprintf(stderr, "%s:%d: Assertion '%s == %s' failed (Got \"%s\", Expected \"%s\")\n", __FILE__, __LINE__, #a, #b, a, b); exit(1); } } while(0)

#define CK_NORMAL 0
#define CK_VERBOSE 1

static Suite* suite_create(const char* name) {
    Suite* s = (Suite*)malloc(sizeof(Suite));
    s->name = strdup(name);
    s->num_tcases = 0;
    return s;
}

static TCase* tcase_create(const char* name) {
    TCase* tc = (TCase*)malloc(sizeof(TCase));
    tc->name = strdup(name);
    tc->num_tests = 0;
    tc->setup = NULL;
    tc->teardown = NULL;
    return tc;
}

static void tcase_add_test_func(TCase* tc, test_func_t func, const char* name) {
    tc->tests[tc->num_tests].func = func;
    tc->tests[tc->num_tests].name = name;
    tc->num_tests++;
}
#define tcase_add_test(tc, tf) tcase_add_test_func(tc, tf, #tf)

static void tcase_add_checked_fixture(TCase* tc, fixture_func_t setup, fixture_func_t teardown) {
    tc->setup = setup;
    tc->teardown = teardown;
}

static void suite_add_tcase(Suite* s, TCase* tc) {
    s->tcases[s->num_tcases++] = tc;
}

static SRunner* srunner_create(Suite* s) {
    SRunner* sr = (SRunner*)malloc(sizeof(SRunner));
    sr->suite = s;
    sr->failed = 0;
    sr->total = 0;
    return sr;
}

static void srunner_run_all(SRunner* sr, int mode) {
    printf("Running suite: %s\n", sr->suite->name);
    for (int i = 0; i < sr->suite->num_tcases; i++) {
        TCase* tc = sr->suite->tcases[i];
        for (int j = 0; j < tc->num_tests; j++) {
            sr->total++;
            if (mode == CK_VERBOSE) printf("Running test: %s\n", tc->tests[j].name);
            
            if (tc->setup) tc->setup();
            tc->tests[j].func();
            if (tc->teardown) tc->teardown();
        }
    }
}

static int srunner_ntests_failed(SRunner* sr) {
    return sr->failed;
}

static void srunner_free(SRunner* sr) {
    for(int i=0; i<sr->suite->num_tcases; ++i) {
        free(sr->suite->tcases[i]->name);
        free(sr->suite->tcases[i]);
    }
    free(sr->suite->name);
    free(sr->suite);
    free(sr);
}

#endif
