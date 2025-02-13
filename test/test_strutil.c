/*
 * test_strutil.c - Detailed unit tests for the thread-safe dynamic string library.
 *
 * This file tests the behavior of the library functions under various parameter conditions,
 * including normal cases and edge cases such as:
 *  - NULL pointers,
 *  - Empty strings,
 *  - Out-of-bound positions,
 *  - Memory management limits.
 *
 * Compile with: gcc -o test_strutil_detailed_params test_strutil_detailed_params.c -lpthread
 *
 * "Talk is cheap. Show me the code." - Linus Torvalds
 */

#include "strutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* ------------------------------------------------------------------------- */
/*                   Memory Management & Initialization Tests                */
/* ------------------------------------------------------------------------- */

/* Test that str_init returns a valid object with proper initial values. */
static void test_str_init_valid(void) {
    str *s = str_init();
    assert(s != NULL && "str_init should not return NULL");
    assert(s->length == 0 && "Newly initialized string should have length 0");
    /* Note: s->data is expected to be NULL until set via str_set */
    str_free(s);
}

/* Test str_alloc and str_realloc with valid and invalid sizes. */
static void test_memory_management(void) {
    /* Valid allocation */
    size_t size = 50;
    str *s = str_alloc(size);
    assert(s != NULL && "str_alloc should return a valid pointer for non-zero size");
    assert(s->capacity == size);
    
    /* Set a string and then reallocate to a smaller size.
       Since the new capacity is 10, maximum length becomes 9 (10 - 1). */
    const char *input = "Memory management test";
    assert(str_set(s, input) == STR_OK);
    size_t current_length = s->length;
    int ret = str_realloc(s, 10);
    assert(ret == STR_OK);
    assert(s->capacity == 10);
    assert(s->length == 9);  /* Length adjusted to new capacity - 1 */
    str_free(s);

    /* Allocating with size 0 should return NULL. */
    s = str_alloc(0);
    assert(s == NULL);

    /* Reallocating to a size greater than STR_MAX_STRING_SIZE should return STR_OVERFLOW. */
    s = str_alloc(20);
    ret = str_realloc(s, STR_MAX_STRING_SIZE + 1);
    assert(ret == STR_OVERFLOW);
    str_free(s);
}

/* ------------------------------------------------------------------------- */
/*                      Basic Operation Tests                                */
/* ------------------------------------------------------------------------- */

/* Test str_set with valid strings, empty strings, and invalid (NULL) parameters. */
static void test_str_set_valid_and_edge(void) {
    str *s = str_init();

    /* Valid string */
    int ret = str_set(s, "Test string");
    assert(ret == STR_OK && "str_set should return STR_OK for valid string");
    assert(strcmp(str_get(s), "Test string") == 0);

    /* Empty string */
    ret = str_set(s, "");
    assert(ret == STR_OK && "str_set should handle empty string");
    assert(strcmp(str_get(s), "") == 0);
    assert(str_get_size(s) == 0);

    /* Passing NULL as the string should return STR_NULL */
    ret = str_set(s, NULL);
    assert(ret == STR_NULL && "str_set should return STR_NULL when string parameter is NULL");

    /* Passing NULL as the object should also return STR_NULL */
    ret = str_set(NULL, "Test");
    assert(ret == STR_NULL && "str_set should return STR_NULL when self is NULL");

    str_free(s);
}

/* Test str_add for appending valid strings, empty strings, and handling NULL parameters. */
static void test_str_add_valid_and_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "Base");
    assert(ret == STR_OK);

    /* Append a valid non-empty string */
    ret = str_add(s, " Added");
    assert(ret == STR_OK && "str_add should succeed for valid input");
    assert(strcmp(str_get(s), "Base Added") == 0);

    /* Append an empty string should not change the content */
    ret = str_add(s, "");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Base Added") == 0);

    /* NULL tests */
    ret = str_add(NULL, "Test");
    assert(ret == STR_NULL && "str_add should return STR_NULL if self is NULL");
    ret = str_add(s, NULL);
    assert(ret == STR_NULL && "str_add should return STR_NULL if data is NULL");

    str_free(s);
}

/* Test str_copy behavior with valid inputs and edge cases. */
static void test_str_copy_behavior(void) {
    char dest[20];
    const char *source = "Copy test";
    char *res = str_copy(dest, source, sizeof(dest));
    assert(res != NULL);
    assert(strcmp(dest, "Copy test") == 0);

    /* Zero max_len should yield NULL. */
    res = str_copy(dest, source, 0);
    assert(res == NULL);

    /* NULL pointer tests */
    res = str_copy(NULL, source, 20);
    assert(res == NULL);
    res = str_copy(dest, NULL, 20);
    assert(res == NULL);
}

/* ------------------------------------------------------------------------- */
/*                   String Manipulation Function Tests                    */
/* ------------------------------------------------------------------------- */

/* Test case conversion functions (to_upper, to_lower, to_title_case, reverse)
   with various parameter conditions. */
static void test_case_conversion_edge(void) {
    str *s = str_init();

    /* When s->data is NULL, conversion functions should return STR_NULL. */
    int ret = str_to_upper(s);
    assert(ret == STR_NULL);
    ret = str_to_lower(s);
    assert(ret == STR_NULL);
    ret = str_to_title_case(s);
    assert(ret == STR_NULL);
    ret = str_reverse(s);
    assert(ret == STR_NULL);

    /* Now set a valid string and test conversions. */
    ret = str_set(s, "MiXeD CaSe");
    assert(ret == STR_OK);
    ret = str_to_upper(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "MIXED CASE") == 0);

    ret = str_set(s, "MiXeD CaSe");
    assert(ret == STR_OK);
    ret = str_to_lower(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "mixed case") == 0);

    ret = str_set(s, "miXeD CaSe");
    assert(ret == STR_OK);
    ret = str_to_title_case(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Mixed Case") == 0);

    ret = str_set(s, "abcdef");
    assert(ret == STR_OK);
    ret = str_reverse(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "fedcba") == 0);

    str_free(s);
}

/* Test str_insert at beginning, middle, end, and invalid positions. */
static void test_str_insert_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "HelloWorld");
    assert(ret == STR_OK);

    /* Insert at the beginning */
    ret = str_insert(s, 0, "Start-");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Start-HelloWorld") == 0);

    /* Insert in the middle */
    ret = str_insert(s, 6, "Middle-");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Start-Middle-HelloWorld") == 0);

    /* Insert at the end */
    size_t current_length = str_get_size(s);
    ret = str_insert(s, current_length, "-End");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Start-Middle-HelloWorld-End") == 0);

    /* Inserting at a position greater than current length should return STR_INVALID */
    ret = str_insert(s, str_get_size(s) + 1, "Fail");
    assert(ret == STR_INVALID);

    /* NULL pointer tests */
    ret = str_insert(NULL, 0, "Test");
    assert(ret == STR_NULL);
    ret = str_insert(s, 0, NULL);
    assert(ret == STR_NULL);

    str_free(s);
}

/* Test str_substr for valid extraction and edge cases. */
static void test_str_substr_edge(void) {
    str *s = str_init();
    str *sub = str_init();
    int ret = str_set(s, "Extract this substring.");
    assert(ret == STR_OK);

    /* Valid extraction: starting at index 8, length 4 should yield "this" */
    ret = str_substr(s, sub, 8, 4);
    assert(ret == STR_OK);
    assert(strcmp(str_get(sub), "this") == 0);

    /* Extraction with length exceeding remaining characters */
    ret = str_substr(s, sub, 8, 100);
    assert(ret == STR_OK);
    assert(strcmp(str_get(sub), "this substring.") == 0);

    /* Extraction with pos equal to length should return STR_INVALID */
    ret = str_substr(s, sub, str_get_size(s), 5);
    assert(ret == STR_INVALID);

    /* NULL parameter tests */
    ret = str_substr(NULL, sub, 0, 5);
    assert(ret == STR_NULL);
    ret = str_substr(s, NULL, 0, 5);
    assert(ret == STR_NULL);

    str_free(s);
    str_free(sub);
}

/* Test str_rem_word when the target word occurs multiple times and when it is absent. */
static void test_str_rem_word_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "word word word");
    assert(ret == STR_OK);

    /* Remove first occurrence */
    ret = str_rem_word(s, "word");
    assert(ret == STR_OK);
    /* Sonuçta ilk "word" kaldırılmış olmalı */
    assert(strstr(str_get(s), "word") != NULL);

    /* Remove remaining occurrences one by one */
    ret = str_rem_word(s, "word");
    assert(ret == STR_OK);
    ret = str_rem_word(s, "word");
    assert(ret == STR_OK);
    /* Artık "word" bulunmamalı */
    assert(strstr(str_get(s), "word") == NULL);

    /* Çalışmayan durum: bulunmayan kelime */
    ret = str_rem_word(s, "absent");
    assert(ret == STR_FAIL);

    /* NULL tests */
    ret = str_rem_word(NULL, "word");
    assert(ret == STR_NULL);
    ret = str_rem_word(s, NULL);
    assert(ret == STR_NULL);

    str_free(s);
}

/* Test str_swap_word with replacement strings of different lengths. */
static void test_str_swap_word_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "Swap test");
    assert(ret == STR_OK);

    /* Swap with a longer replacement */
    ret = str_swap_word(s, "test", "detailed test");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Swap detailed test") == 0);

    /* Swap with a shorter replacement */
    ret = str_set(s, "Replace long with short");
    assert(ret == STR_OK);
    ret = str_swap_word(s, "long", "s");
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "Replace s with short") == 0);

    /* Non-existent substring */
    ret = str_swap_word(s, "nonexistent", "fail");
    assert(ret == STR_FAIL);

    /* NULL tests */
    ret = str_swap_word(NULL, "test", "test");
    assert(ret == STR_NULL);
    ret = str_swap_word(s, NULL, "test");
    assert(ret == STR_NULL);
    ret = str_swap_word(s, "test", NULL);
    assert(ret == STR_NULL);

    str_free(s);
}

/* Test str_find with different starting positions and empty substring. */
static void test_str_find_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "Find the needle in this haystack.");
    assert(ret == STR_OK);

    /* Valid search */
    size_t pos = str_find(s, "needle", 0);
    assert(pos != (size_t)-1);

    /* Search starting beyond string length */
    pos = str_find(s, "needle", str_get_size(s));
    assert(pos == (size_t)-1);

    /* Search for an empty substring should return the start position */
    pos = str_find(s, "", 5);
    assert(pos == 5);

    /* Search for non-existent substring */
    pos = str_find(s, "absent", 0);
    assert(pos == (size_t)-1);

    /* NULL tests */
    pos = str_find(NULL, "needle", 0);
    assert(pos == (size_t)-1);
    pos = str_find(s, NULL, 0);
    assert(pos == (size_t)-1);

    str_free(s);
}

/* Test str_starts_with and str_ends_with with valid, mismatching, and NULL parameters. */
static void test_starts_ends_with_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "Prefix and Suffix");
    assert(ret == STR_OK);

    /* Positive checks */
    assert(str_starts_with(s, "Prefix") == true);
    assert(str_ends_with(s, "Suffix") == true);
    /* Negative checks */
    assert(str_starts_with(s, "suffix") == false);
    assert(str_ends_with(s, "prefix") == false);

    /* NULL tests */
    assert(str_starts_with(NULL, "Prefix") == false);
    assert(str_starts_with(s, NULL) == false);
    assert(str_ends_with(NULL, "Suffix") == false);
    assert(str_ends_with(s, NULL) == false);

    str_free(s);
}

/* Test padding functions (str_pad_left and str_pad_right) under various conditions. */
static void test_padding_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "pad");
    assert(ret == STR_OK);

    /* When total_length <= current length, no change should occur */
    ret = str_pad_left(s, 2, '0');
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "pad") == 0);
    ret = str_pad_right(s, 3, '0');
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "pad") == 0);

    /* Padding to a greater length */
    ret = str_pad_left(s, 6, 'X');
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "XXXpad") == 0);
    ret = str_set(s, "pad");
    assert(ret == STR_OK);
    ret = str_pad_right(s, 6, 'Y');
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "padYYY") == 0);

    /* NULL tests */
    ret = str_pad_left(NULL, 6, '0');
    assert(ret == STR_NULL);
    ret = str_pad_right(NULL, 6, '0');
    assert(ret == STR_NULL);

    str_free(s);
}

/* Test trimming functions (str_trim, str_trim_left, str_trim_right) for various input cases. */
static void test_trimming_edge(void) {
    str *s = str_init();
    int ret = str_set(s, "   trim   ");
    assert(ret == STR_OK);

    /* Full trim */
    ret = str_trim(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "trim") == 0);

    /* Left trim */
    ret = str_set(s, "   left trim");
    assert(ret == STR_OK);
    ret = str_trim_left(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "left trim") == 0);

    /* Right trim */
    ret = str_set(s, "right trim   ");
    assert(ret == STR_OK);
    ret = str_trim_right(s);
    assert(ret == STR_OK);
    assert(strcmp(str_get(s), "right trim") == 0);

    /* NULL tests */
    ret = str_trim(NULL);
    assert(ret == STR_NULL);
    ret = str_trim_left(NULL);
    assert(ret == STR_NULL);
    ret = str_trim_right(NULL);
    assert(ret == STR_NULL);

    str_free(s);
}

/* Test get_dyn_input using simulated stdin to cover its behavior under normal input. */
static void test_get_dyn_input_edge(void) {
    const char *input_str = "Simulated dynamic input test\n";
    FILE *f = fmemopen((void*)input_str, strlen(input_str), "r");
    assert(f != NULL);
    FILE *old_stdin = stdin;
    stdin = f;
    char *result = get_dyn_input(100);
    assert(result != NULL);
    /* The trailing newline should be stripped */
    assert(strcmp(result, "Simulated dynamic input test") == 0);
    free(result);
    fclose(f);
    stdin = old_stdin;
}

/* ------------------------------------------------------------------------- */
/*                                Main                                     */
/* ------------------------------------------------------------------------- */

int main(void) {
    printf("Running detailed unit tests for strutil library (various parameter conditions)...\n");

    test_str_init_valid();
    test_str_set_valid_and_edge();
    test_str_add_valid_and_edge();
    test_str_copy_behavior();
    test_case_conversion_edge();
    test_str_insert_edge();
    test_str_substr_edge();
    test_str_rem_word_edge();
    test_str_swap_word_edge();
    test_str_find_edge();
    test_starts_ends_with_edge();
    test_padding_edge();
    test_trimming_edge();
    test_memory_management();
    test_get_dyn_input_edge();

    printf("All detailed parameter tests passed successfully.\n");
    return 0;
}
