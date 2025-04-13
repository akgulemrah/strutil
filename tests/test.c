#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include <stdio.h>
#include <stdlib.h>

#include "strutil.h"

#ifdef _WIN32
    #include <io.h>
    #define DELETE_FILE(path) _unlink(path)  // Windows için
#else
    #include <unistd.h>
    #define DELETE_FILE(path) unlink(path)  // Linux/macOS için
#endif


void test_init(void) {
    struct str* s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_TRUE(str_is_empty(s));
    str_free(s);
}

void test_free(void) {
    struct str* s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
    CU_ASSERT(s == NULL);
}

void test_clear(void) {
    struct str* s = str_init();
    CU_ASSERT_EQUAL(str_set(s, "Test String"), STR_OK);
    CU_ASSERT_FALSE(str_is_empty(s));
    str_clear(s);
    CU_ASSERT_TRUE(str_is_empty(s));
    str_free(s);
}

void test_set_get(void) {
    struct str* s = str_init();
    CU_ASSERT_EQUAL(str_set(s, "Test String"), STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Test String");
    CU_ASSERT_EQUAL(str_get_size(s), 11);
    str_free(s);
}

void test_concatenation(void) {
    struct str* s = str_init();
    CU_ASSERT_EQUAL(str_set(s, "Hello"), STR_OK);
    CU_ASSERT_EQUAL(str_add(s, " World"), STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World");
    str_free(s);
}

void test_case_conversion(void) {
    struct str* s = str_init();
    CU_ASSERT_EQUAL(str_set(s, "Test String"), STR_OK);
    CU_ASSERT_EQUAL(str_to_upper(s), STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "TEST STRING");
    CU_ASSERT_EQUAL(str_to_lower(s), STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "test string");
    str_free(s);
}

void test_reverse(void) {
    struct str* s = str_init();
    CU_ASSERT_EQUAL(str_set(s, "Hello"), STR_OK);
    CU_ASSERT_EQUAL(str_reverse(s), STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "olleH");
    str_free(s);
}

void test_alloc(void) {
    struct str *s = str_alloc(10);
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_get_capacity(s) == (size_t)10);
    str_free(s);
}

void test_realloc(void) {
    struct str *s = str_alloc(10);
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_get_capacity(s) == (size_t)10);
    CU_ASSERT(str_realloc(s, 20) == STR_OK);
    CU_ASSERT(str_get_capacity(s) == (size_t)20);
    str_free(s);
}

void test_grow(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_grow(s, 10) == STR_OK);
    CU_ASSERT(str_get_capacity(s) >= (size_t)10);
    CU_ASSERT(str_grow(s, 35) == STR_OK);
    CU_ASSERT(str_get_capacity(s) >= (size_t)35);
    str_free(s);
}

void test_copy(void) {
    struct str *s1 = str_init();
    struct str *s2 = str_init();
    CU_ASSERT_PTR_NOT_NULL(s1);
    CU_ASSERT_PTR_NOT_NULL(s2);

    CU_ASSERT(str_set(s1, "Test String") == STR_OK);
    CU_ASSERT(str_copy(s2, s1, 5) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s2), "Test ");
    str_free(s1);
    str_free(s2);
}

void test_add(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_add(s, "Hello") == STR_OK);
    CU_ASSERT(str_add(s, " World") == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World");
    str_free(s);
}

void test_to_upper(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Test String") == STR_OK);
    CU_ASSERT(str_to_upper(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "TEST STRING");
    str_free(s);
}

void test_to_lower(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Test String") == STR_OK);
    CU_ASSERT(str_to_lower(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "test string");
    str_free(s);
}

void test_trim(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "  Hello World  ") == STR_OK);
    CU_ASSERT(str_trim(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World");
    str_free(s);
}

void test_pad_left(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Test") == STR_OK);
    CU_ASSERT(str_pad_left(s, 8, '*') == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "****Test");
    str_free(s);
}

void test_pad_right(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Test") == STR_OK);
    CU_ASSERT(str_pad_right(s, 8, '*') == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Test****");
    str_free(s);
}

void test_trim_left(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "  Test") == STR_OK);
    CU_ASSERT(str_trim_left(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Test");
    str_free(s);
}

void test_trim_right(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Test  ") == STR_OK);
    CU_ASSERT(str_trim_right(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Test");
    str_free(s);
}

void test_insert(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello") == STR_OK);
    CU_ASSERT(str_insert(s, 5, " World") == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World");
    str_free(s);
}

void test_find(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    CU_ASSERT(str_find(s, "World", 0) == 6);
    CU_ASSERT(str_find(s, "World", 6) != STR_NPOS);
    str_free(s);
}

void test_starts_with(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    CU_ASSERT(str_starts_with(s, "Hello"));
    CU_ASSERT_FALSE(str_starts_with(s, "World"));
    str_free(s);
}

void test_ends_with(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    CU_ASSERT(str_ends_with(s, "World"));
    CU_ASSERT_FALSE(str_ends_with(s, "Hello"));
    str_free(s);
}

void test_rem_word(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    CU_ASSERT(str_rem_word(s, "World") == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello ");
    str_free(s);
}

void test_swap_word(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    CU_ASSERT(str_swap_word(s, "World", "Universe") == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello Universe");
    str_free(s);
}

void test_get_dyn_input(void) {
    printf("Enter 'Hello': ");
    char *input = get_dyn_input(10);
    CU_ASSERT_PTR_NOT_NULL(input);
    CU_ASSERT_STRING_EQUAL(input, "Hello");
    free(input);
}

void test_input(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    FILE *stream = fopen("test_input.txt", "w");
    CU_ASSERT_PTR_NOT_NULL(stream);
    fprintf(stream, "Test String\n");
    fclose(stream);
    stream = fopen("test_input.txt", "r");
    CU_ASSERT_PTR_NOT_NULL(stream);
    CU_ASSERT(str_input(s, stream) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Test String");
    fclose(stream);
    str_free(s);
    DELETE_FILE("test_input.txt");
}

void test_add_input(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    
    FILE *stream = fopen("test_input.txt", "w");
    CU_ASSERT_PTR_NOT_NULL(stream);
    fputs("Hello World\n", stream);
    fflush(stream);
    fclose(stream);
    
    stream = fopen("test_input.txt", "r");
    CU_ASSERT_PTR_NOT_NULL(stream);
    
    // Read first word
    CU_ASSERT(str_add_input(s, stream) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello");
    
    // Read second word
    CU_ASSERT(str_add_input(s, stream) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World");
    
    fclose(stream);
    str_free(s);
    DELETE_FILE("test_input.txt");
}

void test_print(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "Hello World") == STR_OK);
    str_print(s);
    str_free(s);
}

void test_to_title_case(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_set(s, "hello world example") == STR_OK);
    CU_ASSERT(str_to_title_case(s) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s), "Hello World Example");
    str_free(s);
}

void test_str_mov(void) {
    struct str *s1 = str_init();
    struct str *s2 = str_init();
    CU_ASSERT_PTR_NOT_NULL(s1);
    CU_ASSERT_PTR_NOT_NULL(s2);
    
    CU_ASSERT(str_set(s1, "Test String") == STR_OK);
    CU_ASSERT(str_mov(s2, s1) == STR_OK);
    CU_ASSERT_STRING_EQUAL(str_get_data(s2), "Test String");
    str_free(s2);
}

void test_error_handling(void) {
    struct str *s = str_init();
    CU_ASSERT_PTR_NOT_NULL(s);
    
    // Test NULL pointer handling
    CU_ASSERT_EQUAL(str_add(NULL, "test"), STR_NULL);
    CU_ASSERT_EQUAL(str_add(s, NULL), STR_NULL);
    
    // Test empty string
    CU_ASSERT_EQUAL(str_set(s, ""), STR_INVALID);
    
    // Test overflow handling with large allocation
    CU_ASSERT_EQUAL(str_grow(s, STR_MAX_STRING_SIZE + 1), STR_OVERFLOW);
    
    // Test invalid operations
    CU_ASSERT_EQUAL(str_insert(s, 100, "test"), STR_INVALID); // Position beyond length
    
    str_free(s);
}

int main()
{
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_pSuite suite = CU_add_suite("strutil", NULL, NULL);
	if (!suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (!CU_add_test(suite, "test_init", test_init) ||
	    !CU_add_test(suite, "test_free", test_free) ||
	    !CU_add_test(suite, "test_clear", test_clear) ||
	    !CU_add_test(suite, "test_set_get", test_set_get) ||
	    !CU_add_test(suite, "test_concatenation", test_concatenation) ||
	    !CU_add_test(suite, "test_case_conversion", test_case_conversion) ||
	    !CU_add_test(suite, "test_reverse", test_reverse) ||
	    !CU_add_test(suite, "test_alloc", test_alloc) ||
	    !CU_add_test(suite, "test_realloc", test_realloc) ||
	    !CU_add_test(suite, "test_grow", test_grow) ||
	    !CU_add_test(suite, "test_copy", test_copy) ||
	    !CU_add_test(suite, "test_add", test_add) ||
	    !CU_add_test(suite, "test_to_upper", test_to_upper) ||
	    !CU_add_test(suite, "test_to_lower", test_to_lower) ||
	    !CU_add_test(suite, "test_trim", test_trim) ||
	    !CU_add_test(suite, "test_pad_left", test_pad_left) ||
	    !CU_add_test(suite, "test_pad_right", test_pad_right) ||
	    !CU_add_test(suite, "test_trim_left", test_trim_left) ||
	    !CU_add_test(suite, "test_trim_right", test_trim_right) ||
	    !CU_add_test(suite, "test_insert", test_insert) ||
	    !CU_add_test(suite, "test_find", test_find) ||
	    !CU_add_test(suite, "test_starts_with", test_starts_with) ||
	    !CU_add_test(suite, "test_ends_with", test_ends_with) ||
	    !CU_add_test(suite, "test_rem_word", test_rem_word) ||
	    !CU_add_test(suite, "test_swap_word", test_swap_word) ||
	    !CU_add_test(suite, "test_get_dyn_input", test_get_dyn_input) ||
	    !CU_add_test(suite, "test_input", test_input) ||
	    !CU_add_test(suite, "test_add_input", test_add_input) ||
	    !CU_add_test(suite, "test_to_title_case", test_to_title_case) ||
	    !CU_add_test(suite, "test_str_mov", test_str_mov) ||
	    !CU_add_test(suite, "test_error_handling", test_error_handling) ||
	    !CU_add_test(suite, "test_print", test_print)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();


	return 0;
}
