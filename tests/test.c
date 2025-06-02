#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> // For ULONG_MAX (though STR_NPOS is defined directly as -1)
#include "strutil.h" // Include the header for the string utility library

// Define some global string objects used across test cases.
// These are managed by setup/teardown functions.
struct str *test_str = NULL;
struct str *test_str2 = NULL;
struct str *test_str_alloc_fixed = NULL; // For testing fixed-size scenarios specifically
struct str *test_str_io = NULL;         // For I/O related tests

// Assuming MIN_CAPACITY is defined internally and globally visible if needed,
// otherwise, use its typical value (e.g., 16) or define it here for tests if strictly necessary.
// #define MIN_CAPACITY 16 // This is defined in strutil.c, typically internal.

// Get CHUNK_SIZE from strutil.c, used for I/O tests.
// A common way to get it from .c without exposing it in .h for tests is through extern or re-define
// For simplicity in this test file, we assume its value, or you could make it an extern constant in strutil.h for tests.
// Let's re-define for self-contained testing:
#ifndef CHUNK_SIZE
#define CHUNK_SIZE 4096 // Should match the one in strutil.c
#endif


struct str {
    unsigned int flags;
    char *data;
    size_t length;
    size_t capacity;
    pthread_mutex_t lock;
};


// --- Setup and Teardown Functions for Test Fixtures ---

// Basic setup: initializes a single string object.
void setup_basic(void) {
    test_str = str_init();
    ck_assert_ptr_ne(test_str, NULL);
}

// Basic teardown: frees the single string object.
void teardown_basic(void) {
    str_free(test_str);
    ck_assert_ptr_eq(test_str, NULL); // Assert that the pointer was nulled after free
}

// Setup for tests requiring two string objects.
void setup_two_strings(void) {
    setup_basic(); // Initializes test_str
    test_str2 = str_init();
    ck_assert_ptr_ne(test_str2, NULL);
}

// Teardown for tests with two string objects.
void teardown_two_strings(void) {
    teardown_basic(); // Frees test_str
    str_free(test_str2);
    ck_assert_ptr_eq(test_str2, NULL);
}

// Setup for tests needing a pre-allocated fixed-size string or I/O string
void setup_special_strings(void) {
    setup_two_strings(); // Gets test_str and test_str2

    // For fixed-size tests
    test_str_alloc_fixed = str_alloc(10); // Allocate with capacity 10
    ck_assert_ptr_ne(test_str_alloc_fixed, NULL);
    SET_FLAG(test_str_alloc_fixed->flags, STR_FLAG_FIXED_SIZE); // Make it fixed size

    // For I/O tests
    test_str_io = str_init();
    ck_assert_ptr_ne(test_str_io, NULL);
}

// Teardown for tests with special strings
void teardown_special_strings(void) {
    teardown_two_strings(); // Frees test_str and test_str2
    str_free(test_str_alloc_fixed);
    ck_assert_ptr_eq(test_str_alloc_fixed, NULL);
    str_free(test_str_io);
    ck_assert_ptr_eq(test_str_io, NULL);
}

// --- Test Cases for Core Functions ---

START_TEST(test_str_init_and_free)
{
    // A standalone test, as it's not strictly using fixtures but testing them.
    struct str *s = str_init();
    ck_assert_ptr_ne(s, NULL);
    ck_assert_int_eq(str_get_size(s), 0);
    // As MIN_CAPACITY is an internal static const, we assume it's 16 here.
    // If not, test would need more robust way to discover MIN_CAPACITY.
    ck_assert_int_ge(str_get_capacity(s), 16); 
    ck_assert(str_is_empty(s));
    ck_assert_ptr_ne(str_get_data(s), NULL); // Data pointer must be valid
    
    // Check flags are set correctly
    ck_assert_msg(CHECK_FLAG(s->flags, STR_FLAG_DYNAMIC), "STR_FLAG_DYNAMIC should be set");
    ck_assert_msg(CHECK_FLAG(s->flags, STR_FLAG_MUTEX_INIT), "STR_FLAG_MUTEX_INIT should be set");
    ck_assert_msg(!CHECK_FLAG(s->flags, STR_FLAG_READONLY), "STR_FLAG_READONLY should NOT be set initially");
    ck_assert_msg(!CHECK_FLAG(s->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should NOT be set initially");

    str_free(s);
    ck_assert_ptr_eq(s, NULL); // Assert that the pointer was nulled after freeing
}
END_TEST

START_TEST(test_str_clear)
{
    Str_err_t err;
    err = str_set(test_str, "Hello World");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(test_str), 11);
    ck_assert_str_eq(str_get_data(test_str), "Hello World");
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_set");

    str_clear(test_str);
    ck_assert_int_eq(str_get_size(test_str), 0);
    ck_assert(str_is_empty(test_str));
    ck_assert_str_eq(str_get_data(test_str), ""); // Data should be null-terminated empty string
    ck_assert_msg(!CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be cleared after str_clear");

    // Clear an already empty string
    str_clear(test_str);
    ck_assert_int_eq(str_get_size(test_str), 0);
    ck_assert(str_is_empty(test_str));
}
END_TEST

// --- Test Cases for Memory Management ---

START_TEST(test_str_alloc)
{
    struct str *s = str_alloc(100);
    ck_assert_ptr_ne(s, NULL);
    ck_assert_int_eq(str_get_size(s), 0);
    ck_assert_int_ge(str_get_capacity(s), 100); // Should be at least 100 or actual alloc (e.g. MIN_CAPACITY then grow or just 100 directly)
    ck_assert_str_eq(str_get_data(s), ""); // Should be an empty, null-terminated string
    str_free(s);
    ck_assert_ptr_eq(s, NULL);

    // Test with size 0, should return NULL
    s = str_alloc(0);
    ck_assert_ptr_eq(s, NULL);

    // Test with size larger than STR_MAX_STRING_SIZE, should return NULL
    s = str_alloc(STR_MAX_STRING_SIZE + 1);
    ck_assert_ptr_eq(s, NULL);
}
END_TEST

START_TEST(test_str_realloc_macro_basic_functionality)
{
    Str_err_t err = STR_OK;
    struct str *s = str_init();
    ck_assert_ptr_ne(s, NULL);
    ck_assert_int_ge(str_get_capacity(s), 16); // Assuming MIN_CAPACITY

    // Grow to 100
    err = str_realloc(s, 100);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_ge(str_get_capacity(s), 100); // Capacity should be at least 100
    ck_assert_int_eq(str_get_size(s), 0);
    ck_assert_str_eq(str_get_data(s), "");

    // Set some content
    err = str_set(s, "Test String for Realloc");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(s), strlen("Test String for Realloc"));
    ck_assert_str_eq(str_get_data(s), "Test String for Realloc");

    // Shrink but maintain content length
    size_t required_cap = strlen("Test String for Realloc") + 1; // Content len + null terminator
    err = str_realloc(s, required_cap);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_capacity(s), required_cap); // Capacity should be exact match
    ck_assert_int_eq(str_get_size(s), strlen("Test String for Realloc"));
    ck_assert_str_eq(str_get_data(s), "Test String for Realloc");

    // Shrink below content length (truncation)
    err = str_realloc(s, 5); // Target capacity 5, string is "Test String for Realloc" (len 23)
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_capacity(s), 5);
    ck_assert_int_eq(str_get_size(s), 4); // New length 4 (0-based: index 0 to 3) for cap 5 (+1 for null)
    ck_assert_str_eq(str_get_data(s), "Test"); // Content should be truncated and null-terminated
    str_free(s);
}
END_TEST

START_TEST(test_str_realloc_fixed_size_fail)
{
    struct str *s = test_str_alloc_fixed; // Capacity 10, FIXED_SIZE flag set in setup
    ck_assert_ptr_ne(s, NULL);
    ck_assert_int_eq(str_get_capacity(s), 10);
    ck_assert_msg(CHECK_FLAG(s->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set");

    // Try to grow fixed-size string (target 20 > current 10)
    Str_err_t err = str_realloc(s, 20);
    ck_assert_int_eq(err, STR_MAXSIZE); // Should fail due to fixed size
    ck_assert_int_eq(str_get_capacity(s), 10); // Capacity should be unchanged

    // Try to realloc to a smaller size, this should be allowed.
    err = str_realloc(s, 5);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_capacity(s), 5); // Should have shrunk
    
    // Try to realloc with too large new_size (regardless of fixed_size)
    err = str_realloc(s, STR_MAX_STRING_SIZE + 1);
    ck_assert_int_eq(err, STR_OVERFLOW);
    ck_assert_int_eq(str_get_capacity(s), 5); // Capacity should remain 5

    // Realloc to 0, should free
    str_realloc(s, 0); // s should become NULL after this, handled by check and next test if called
    ck_assert_ptr_eq(s, NULL);
}
END_TEST


START_TEST(test_str_grow)
{
    ck_assert_ptr_ne(test_str, NULL);
    size_t initial_cap = str_get_capacity(test_str);

    // Test growth: request capacity double its initial (approx)
    Str_err_t err = str_grow(test_str, initial_cap * 2);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_ge(str_get_capacity(test_str), initial_cap * 2); // New capacity is at least doubled
    size_t after_grow1_cap = str_get_capacity(test_str);

    // Test grow with target less than or equal to current capacity (should do nothing/return OK)
    err = str_grow(test_str, after_grow1_cap - 1);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_capacity(test_str), after_grow1_cap); // Capacity should be unchanged

    // Test growth to a specific much larger minimum capacity
    err = str_grow(test_str, 500);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_ge(str_get_capacity(test_str), 500);

    // Test exceeding max string size
    err = str_grow(test_str, STR_MAX_STRING_SIZE + 1);
    ck_assert_int_eq(err, STR_OVERFLOW);
    // String should not be modified
    ck_assert_int_ge(str_get_capacity(test_str), 500); // Check that it retained its previous valid size
}
END_TEST

START_TEST(test_str_copy)
{
    Str_err_t err;
    err = str_set(test_str, "Source String for Copy");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Source String for Copy");

    // Basic copy: max_len is source length, so full copy
    err = str_copy(test_str2, test_str, str_get_size(test_str));
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str2), "Source String for Copy");
    ck_assert_int_eq(str_get_size(test_str2), 22);
    ck_assert_int_ge(str_get_capacity(test_str2), 23); // length + 1 for null

    // Copy with truncation (max_len < source length)
    err = str_copy(test_str2, test_str, 6);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str2), "Source");
    ck_assert_int_eq(str_get_size(test_str2), 6);
    ck_assert_int_ge(str_get_capacity(test_str2), 7); // Capacity adjusts to copied length + 1

    // Copy to an empty string (max_len very large, will be truncated to source_len)
    str_clear(test_str2);
    err = str_copy(test_str2, test_str, 100);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str2), "Source String for Copy");
    ck_assert_int_eq(str_get_size(test_str2), 22);
    ck_assert_int_ge(str_get_capacity(test_str2), 23);

    // Copy empty source string
    str_clear(test_str);
    err = str_copy(test_str2, test_str, 10);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str2), "");
    ck_assert_int_eq(str_get_size(test_str2), 0);
    ck_assert_int_ge(str_get_capacity(test_str2), 1); // Capacity is adjusted to at least 1 for null-terminator

    // Test with NULL source/destination
    err = str_copy(NULL, test_str, 1);
    ck_assert_int_eq(err, STR_NULL);
    err = str_copy(test_str2, NULL, 1);
    ck_assert_int_eq(err, STR_NULL);
}
END_TEST

START_TEST(test_str_copy_fixed_size_fail)
{
    Str_err_t err;
    err = str_set(test_str, "Source Data Longer Than Fixed Size");
    ck_assert_int_eq(err, STR_OK);

    struct str *fixed_str = test_str_alloc_fixed; // Pre-made in setup, capacity 10, FIXED_SIZE flag set
    str_clear(fixed_str); // Ensure it's empty
    ck_assert_int_eq(str_get_capacity(fixed_str), 10);
    ck_assert_msg(CHECK_FLAG(fixed_str->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set for fixed_str");

    // Try to copy string that doesn't fit into fixed_str (source len 34 > capacity 10)
    err = str_copy(fixed_str, test_str, str_get_size(test_str));
    ck_assert_int_eq(err, STR_MAXSIZE);
    ck_assert_str_eq(str_get_data(fixed_str), ""); // Should be unchanged
    ck_assert_int_eq(str_get_size(fixed_str), 0); // Length should remain 0

    // Try to copy with specific max_len that fits (copy 9 chars, fits capacity 10 for 9 chars + '\0')
    err = str_copy(fixed_str, test_str, 9);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(fixed_str), "Source Da");
    ck_assert_int_eq(str_get_size(fixed_str), 9);
}
END_TEST

START_TEST(test_str_mov)
{
    Str_err_t err;
    err = str_set(test_str, "Original Content of Test String");
    ck_assert_int_eq(err, STR_OK);
    size_t original_len = str_get_size(test_str);
    size_t original_cap = str_get_capacity(test_str);
    char *original_data_ptr = (char*)str_get_data(test_str); // Get internal pointer before move

    // Move content from test_str to test_str2
    // IMPORTANT: test_str becomes invalid and its pointer is nulled after this!
    err = str_mov(test_str2, test_str);
    ck_assert_int_eq(err, STR_OK);

    // Verify test_str (source) state after move
    // Check framework uses `test_str = NULL;` in teardown_basic, so after str_mov test_str
    // needs to be correctly identified as NULL. For this `tcase` it should pass `NULL` implicitly.
    // The `str_mov` function actually frees `src` if it was dynamically allocated.
    // In our test context `test_str` points to an allocated `struct str` in setup.
    // So after `str_mov`, `test_str` pointer should become NULL.
    ck_assert_ptr_eq(test_str, NULL); 

    // Verify test_str2 (destination) state
    ck_assert_str_eq(str_get_data(test_str2), "Original Content of Test String");
    ck_assert_int_eq(str_get_size(test_str2), original_len);
    ck_assert_int_ge(str_get_capacity(test_str2), original_cap); // Should inherit original capacity
    // Critical: check if the data pointer was actually moved, not copied.
    ck_assert_ptr_eq(str_get_data(test_str2), original_data_ptr);

    // Test with NULL source (destination is valid, source is NULL)
    str_clear(test_str2);
    ck_assert_ptr_ne(test_str, NULL); // Re-initialize test_str for this sub-test, or just use `NULL` directly
    err = str_mov(test_str2, NULL);
    ck_assert_int_eq(err, STR_NULL); // Should indicate NULL source

    // Test with NULL destination (source is valid, destination is NULL)
    test_str = str_init(); // re-init test_str
    err = str_set(test_str, "Source for NULL dest test");
    err = str_mov(NULL, test_str);
    ck_assert_int_eq(err, STR_NULL); // Should indicate NULL dest
    // test_str is not modified if dest is NULL or move failed early
    ck_assert_str_eq(str_get_data(test_str), "Source for NULL dest test");
    // Ensure cleanup of test_str after this explicit non-fixture init.
    str_free(test_str);
    ck_assert_ptr_eq(test_str, NULL);
}
END_TEST


// --- Test Cases for String Operations ---

START_TEST(test_str_add)
{
    Str_err_t err;
    err = str_set(test_str, "Hello");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello");
    ck_assert_int_eq(str_get_size(test_str), 5);

    // Append a word
    err = str_add(test_str, " World");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello World");
    ck_assert_int_eq(str_get_size(test_str), 11);

    // Append empty string (should do nothing but return OK)
    err = str_add(test_str, "");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello World");
    ck_assert_int_eq(str_get_size(test_str), 11);

    // Append to empty string
    str_clear(test_str);
    err = str_add(test_str, "First word");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "First word");
    ck_assert_int_eq(str_get_size(test_str), 10);

    // Append causing growth (from minimal initial capacity)
    str_clear(test_str); // Clear it again for specific capacity test
    size_t current_cap = str_get_capacity(test_str);
    // Test that adding a string slightly larger than initial capacity causes growth.
    // e.g., if MIN_CAPACITY is 16, a string of length 17 will cause growth.
    char large_content[current_cap + 2]; // For len current_cap + 1, including null
    memset(large_content, 'X', current_cap + 1);
    large_content[current_cap + 1] = '\0';
    err = str_add(test_str, large_content);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(test_str), current_cap + 1);
    ck_assert_int_ge(str_get_capacity(test_str), current_cap + 2); // Capacity must be > new_length
}
END_TEST

START_TEST(test_str_add_overflow)
{
    // Need a separate string object for large data testing that is not constrained by fixed capacity fixtures.
    struct str *s_long = str_init();
    ck_assert_ptr_ne(s_long, NULL);

    // Fill it nearly to STR_MAX_STRING_SIZE.
    size_t len_to_set = STR_MAX_STRING_SIZE - 2; // Reserve 1 byte for future add, and 1 for null terminator
    char *very_long_data = (char *)malloc(len_to_set + 1);
    ck_assert_ptr_ne(very_long_data, NULL);
    memset(very_long_data, 'X', len_to_set);
    very_long_data[len_to_set] = '\0';
    Str_err_t err = str_set(s_long, very_long_data);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(s_long), len_to_set);

    // Now attempt to append a single character, which should push it over STR_MAX_STRING_SIZE
    err = str_add(s_long, "Y");
    ck_assert_int_eq(err, STR_OVERFLOW);
    ck_assert_int_eq(str_get_size(s_long), len_to_set); // Should not have changed
    ck_assert_str_eq(str_get_data(s_long), very_long_data); // Content unchanged

    free(very_long_data);
    str_free(s_long);
}
END_TEST

START_TEST(test_str_add_fixed_size_fail)
{
    Str_err_t err;
    struct str *s_fixed_add = test_str_alloc_fixed; // Pre-made, capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_add, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_add), 10);
    ck_assert_msg(CHECK_FLAG(s_fixed_add->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set for s_fixed_add");

    err = str_set(s_fixed_add, "Short"); // Length 5
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(s_fixed_add), 5);

    // Try to append "Append" (Length 6). New total length would be 5 + 6 = 11.
    // This exceeds fixed capacity of 10 (+1 for null terminator means required 11).
    err = str_add(s_fixed_add, "Append"); 
    ck_assert_int_eq(err, STR_MAXSIZE); // Should fail
    ck_assert_str_eq(str_get_data(s_fixed_add), "Short"); // Should be unchanged
    ck_assert_int_eq(str_get_size(s_fixed_add), 5);

    // Add string that perfectly fits (Length 4). New total 5 + 4 = 9. Max cap 10.
    err = str_add(s_fixed_add, "More");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(s_fixed_add), "ShortMore");
    ck_assert_int_eq(str_get_size(s_fixed_add), 9);

    // Add 1 more char, should exactly exceed capacity (9+1=10. Req 11 for null, fail)
    err = str_add(s_fixed_add, "!");
    ck_assert_int_eq(err, STR_MAXSIZE);
    ck_assert_str_eq(str_get_data(s_fixed_add), "ShortMore");
}
END_TEST

START_TEST(test_str_set)
{
    Str_err_t err;
    ck_assert(str_is_empty(test_str));

    // Basic set operation
    err = str_set(test_str, "New Content");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "New Content");
    ck_assert_int_eq(str_get_size(test_str), 11);
    ck_assert_int_ge(str_get_capacity(test_str), 12); // Capacity must accommodate string + null
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_set");

    // Set to an empty string (should clear current content)
    err = str_set(test_str, "");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_int_eq(str_get_size(test_str), 0);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should still be set after clearing via str_set");


    // Set again to ensure re-setting works for non-empty content
    err = str_set(test_str, "Another String");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Another String");
    ck_assert_int_eq(str_get_size(test_str), 14);

    // Set NULL source string
    err = str_set(test_str, NULL);
    ck_assert_int_eq(err, STR_NULL); // Should return NULL
    // String should be unchanged after NULL input error
    ck_assert_str_eq(str_get_data(test_str), "Another String"); 
}
END_TEST

START_TEST(test_str_set_readonly_fail)
{
    Str_err_t err;
    SET_FLAG(test_str->flags, STR_FLAG_READONLY); // Set readonly flag on our fixture string
    
    // Attempt to set a string, should fail because of readonly flag
    err = str_set(test_str, "Should Not Change");
    ck_assert_int_eq(err, STR_INVALID); // STR_INVALID is returned for readonly operations
    ck_assert_str_eq(str_get_data(test_str), ""); // String content should remain empty from initial state
    ck_assert_int_eq(str_get_size(test_str), 0); // Length should be unchanged

    CLEAR_FLAG(test_str->flags, STR_FLAG_READONLY); // Clear flag for proper teardown
}
END_TEST

START_TEST(test_str_set_fixed_size_fail)
{
    Str_err_t err;
    struct str *s_fixed_set = test_str_alloc_fixed; // Pre-made, capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_set, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_set), 10);
    ck_assert_msg(CHECK_FLAG(s_fixed_set->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set for s_fixed_set");

    // Set content that is exactly at capacity limit (length 9 + 1 null = 10)
    err = str_set(s_fixed_set, "123456789"); // Length 9
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(s_fixed_set), "123456789");
    ck_assert_int_eq(str_get_size(s_fixed_set), 9);
    ck_assert_int_eq(str_get_capacity(s_fixed_set), 10);

    // Try to set content that is too long for fixed capacity (length 10 + 1 null = 11 > 10)
    err = str_set(s_fixed_set, "0123456789"); // Length 10
    ck_assert_int_eq(err, STR_MAXSIZE);
    ck_assert_str_eq(str_get_data(s_fixed_set), "123456789"); // Should be unchanged

    // Set content that fits easily
    err = str_set(s_fixed_set, "Fits");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(s_fixed_set), "Fits");
    ck_assert_int_eq(str_get_size(s_fixed_set), 4);
}
END_TEST


START_TEST(test_str_assign_n)
{
    Str_err_t err;
    ck_assert(str_is_empty(test_str));

    // Basic assign N, fewer chars copied than source string
    err = str_assign_n(test_str, "Long Example String", 8);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Long Exa");
    ck_assert_int_eq(str_get_size(test_str), 8);
    ck_assert_int_ge(str_get_capacity(test_str), 9); // Must accommodate '8' characters + null

    // Assign N, more chars than source string length
    err = str_assign_n(test_str, "Short", 100);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Short");
    ck_assert_int_eq(str_get_size(test_str), 5);
    ck_assert_int_ge(str_get_capacity(test_str), 6); // Capacity is adjusted to fit 5 + null

    // Assign N, zero chars (should clear the string content)
    err = str_assign_n(test_str, "Clear Me", 0);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_int_eq(str_get_size(test_str), 0);
    ck_assert_msg(!CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be cleared after str_assign_n with count 0");
    
    // Assign N, max_len close to STR_MAX_STRING_SIZE.
    // Using a large buffer to test limits, but actual string copy limited by 'count'
    size_t target_copy_len = STR_MAX_STRING_SIZE - 10;
    char *temp_long_data = (char *)malloc(target_copy_len + 100); // larger temp buf
    ck_assert_ptr_ne(temp_long_data, NULL);
    memset(temp_long_data, 'Z', target_copy_len + 99);
    temp_long_data[target_copy_len + 99] = '\0';

    err = str_assign_n(test_str, temp_long_data, target_copy_len);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(test_str), target_copy_len);
    // You would then verify content starts with 'Z's up to target_copy_len
    
    free(temp_long_data);

    // Assign N, 'count' causes overflow check
    err = str_assign_n(test_str, "Test", STR_MAX_STRING_SIZE + 1);
    ck_assert_int_eq(err, STR_INVALID); // `count` directly checked against STR_MAX_STRING_SIZE
}
END_TEST

START_TEST(test_str_getters)
{
    // Check initial state from setup
    ck_assert_ptr_ne(str_get_data(test_str), NULL);
    ck_assert_int_eq(str_get_size(test_str), 0);
    ck_assert_int_ge(str_get_capacity(test_str), 16); // Assuming MIN_CAPACITY
    ck_assert(str_is_empty(test_str));

    // After setting content
    Str_err_t err = str_set(test_str, "Testing Getters");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Testing Getters");
    ck_assert_int_eq(str_get_size(test_str), 15);
    ck_assert_int_ge(str_get_capacity(test_str), 16);
    ck_assert(!str_is_empty(test_str));

    // Test with NULL string object
    ck_assert_ptr_eq(str_get_data(NULL), NULL);
    ck_assert_int_eq(str_get_size(NULL), 0);
    ck_assert_int_eq(str_get_capacity(NULL), 0);
    ck_assert(str_is_empty(NULL)); // NULL is considered empty as per `str_is_empty` implementation
}
END_TEST

// --- Test Cases for String Manipulation ---

START_TEST(test_str_to_upper)
{
    str_set(test_str, "Hello World 123!@#$%");
    ck_assert_int_eq(str_to_upper(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "HELLO WORLD 123!@#$%");
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_to_upper");

    // Empty string
    str_set(test_str, "");
    ck_assert_int_eq(str_to_upper(test_str), STR_OK); // OK on empty string
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_msg(!CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be cleared on `str_clear` implicitly by str_set"); // No change after str_to_upper for empty

    // String with no alphabetic characters
    str_set(test_str, "123 !@#$");
    ck_assert_int_eq(str_to_upper(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "123 !@#$");
}
END_TEST

START_TEST(test_str_to_lower)
{
    str_set(test_str, "Hello World 123!@#$%");
    ck_assert_int_eq(str_to_lower(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "hello world 123!@#$%");
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_to_lower");


    // Empty string
    str_set(test_str, "");
    ck_assert_int_eq(str_to_lower(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");

    // String with no alphabetic characters
    str_set(test_str, "123 !@#$");
    ck_assert_int_eq(str_to_lower(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "123 !@#$");
}
END_TEST

START_TEST(test_str_to_title_case)
{
    str_set(test_str, "hello world, this is a TEST string. 1st element!");
    ck_assert_int_eq(str_to_title_case(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello World, This Is A Test String. 1st Element!");
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_to_title_case");

    str_set(test_str, "another-test_string. WITH-123 numb3rs!");
    ck_assert_int_eq(str_to_title_case(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Another-Test_String. With-123 Numb3rs!"); // hyphen and underscore are not word separators here

    str_set(test_str, "   first word   "); // leading/trailing spaces
    ck_assert_int_eq(str_to_title_case(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "   First Word   "); // Spaces preserved

    str_set(test_str, ""); // Empty string
    ck_assert_int_eq(str_to_title_case(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
}
END_TEST

START_TEST(test_str_reverse)
{
    str_set(test_str, "ReverseMe");
    ck_assert_int_eq(str_reverse(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "eMesenReV");
    ck_assert_int_eq(str_get_size(test_str), 9); // Size should be same
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_reverse");


    str_set(test_str, "odd");
    ck_assert_int_eq(str_reverse(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "ddo");

    str_set(test_str, "even");
    ck_assert_int_eq(str_reverse(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "neve");

    str_set(test_str, ""); // Empty string
    ck_assert_int_eq(str_reverse(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
}
END_TEST

START_TEST(test_str_remove_word)
{
    str_set(test_str, "One Two Three Two Four Five");
    ck_assert_int_eq(str_remove_word(test_str, "Two"), STR_OK); // Removes first 'Two'
    ck_assert_str_eq(str_get_data(test_str), "One Three Two Four Five");
    ck_assert_int_eq(str_get_size(test_str), 23);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_remove_word");


    ck_assert_int_eq(str_remove_word(test_str, "NotHere"), STR_FAIL); // Substring not found
    ck_assert_str_eq(str_get_data(test_str), "One Three Two Four Five"); // String unchanged
    ck_assert_msg(!CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be cleared by teardown/setup between tests, or checked specifically for this failed operation.");

    // Remove empty word (should fail as it doesn't represent a 'word' to remove)
    ck_assert_int_eq(str_remove_word(test_str, ""), STR_FAIL); 
    ck_assert_str_eq(str_get_data(test_str), "One Three Two Four Five"); // String unchanged

    // Remove a word at the beginning, verify leading space remains
    str_set(test_str, "Start OfString");
    ck_assert_int_eq(str_remove_word(test_str, "Start"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), " OfString"); // Note the leading space remaining from "Start "
    ck_assert_int_eq(str_get_size(test_str), 9);

    // Remove a word at the end
    str_set(test_str, "Before EndWord");
    ck_assert_int_eq(str_remove_word(test_str, "EndWord"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Before ");
    ck_assert_int_eq(str_get_size(test_str), 7);
}
END_TEST

START_TEST(test_str_replace_word)
{
    str_set(test_str, "Alpha Beta Gamma Delta Epsilon");

    // Replace with shorter word
    ck_assert_int_eq(str_replace_word(test_str, "Beta", "Nu"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Alpha Nu Gamma Delta Epsilon");
    ck_assert_int_eq(str_get_size(test_str), 28);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_replace_word");


    // Replace with longer word
    ck_assert_int_eq(str_replace_word(test_str, "Gamma", "Omicron"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Alpha Nu Omicron Delta Epsilon");
    ck_assert_int_eq(str_get_size(test_str), 31);

    // Replace with equal length word
    ck_assert_int_eq(str_replace_word(test_str, "Delta", "Sigma"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Alpha Nu Omicron Sigma Epsilon");
    ck_assert_int_eq(str_get_size(test_str), 31);

    // Substring not found
    ck_assert_int_eq(str_replace_word(test_str, "NotHere", "New"), STR_FAIL);
    ck_assert_str_eq(str_get_data(test_str), "Alpha Nu Omicron Sigma Epsilon"); // String unchanged

    // Replace with empty string (effectively removing)
    ck_assert_int_eq(str_replace_word(test_str, "Sigma ", ""), STR_OK); // Note: space included in old_word
    ck_assert_str_eq(str_get_data(test_str), "Alpha Nu Omicron Epsilon");
    ck_assert_int_eq(str_get_size(test_str), 24);

    // Replace first occurrence
    str_set(test_str, "This is a test string. This is another test.");
    ck_assert_int_eq(str_replace_word(test_str, "This", "That"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "That is a test string. This is another test.");
}
END_TEST

START_TEST(test_str_replace_word_fixed_size_fail)
{
    Str_err_t err;
    struct str *s_fixed_replace = test_str_alloc_fixed; // Pre-made, capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_replace, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_replace), 10);
    ck_assert_msg(CHECK_FLAG(s_fixed_replace->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set");
    
    err = str_set(s_fixed_replace, "my-text."); // Length 8
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(s_fixed_replace), 8);

    // Try to replace 'text' (4) with 'replacement' (11).
    // Original length 8. After removal of 'text' -> length 4. Replace with 11 -> new length 4 + 11 = 15.
    // 15 exceeds fixed capacity of 10.
    err = str_replace_word(s_fixed_replace, "text", "replacement"); 
    ck_assert_int_eq(err, STR_MAXSIZE);
    ck_assert_str_eq(str_get_data(s_fixed_replace), "my-text."); // Should be unchanged
    ck_assert_int_eq(str_get_size(s_fixed_replace), 8);

    // Replace with a string that fits: 'text' (4) with 'words' (5)
    // New length 8 - 4 + 5 = 9. Fits capacity 10.
    err = str_replace_word(s_fixed_replace, "text", "words"); 
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(s_fixed_replace), "my-words.");
    ck_assert_int_eq(str_get_size(s_fixed_replace), 9);
}
END_TEST

// --- Test Cases for Advanced String Operations ---

START_TEST(test_str_insert)
{
    Str_err_t err;
    str_set(test_str, "world");
    ck_assert_int_eq(str_insert(test_str, 0, "Hello "), STR_OK); // Insert at beginning
    ck_assert_str_eq(str_get_data(test_str), "Hello world");
    ck_assert_int_eq(str_get_size(test_str), 11);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_insert");

    ck_assert_int_eq(str_insert(test_str, 5, ", beautiful"), STR_OK); // Insert in middle
    ck_assert_str_eq(str_get_data(test_str), "Hello, beautiful world");
    ck_assert_int_eq(str_get_size(test_str), 22);

    ck_assert_int_eq(str_insert(test_str, str_get_size(test_str), "!"), STR_OK); // Insert at end (append)
    ck_assert_str_eq(str_get_data(test_str), "Hello, beautiful world!");
    ck_assert_int_eq(str_get_size(test_str), 23);

    // Insert to an empty string (like a set operation)
    str_clear(test_str);
    ck_assert_int_eq(str_insert(test_str, 0, "Inserted text"), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Inserted text");
    ck_assert_int_eq(str_get_size(test_str), 13);


    // Invalid position (greater than current length)
    ck_assert_int_eq(str_insert(test_str, 100, "Too Far"), STR_INVALID);
    ck_assert_str_eq(str_get_data(test_str), "Inserted text"); // String should be unchanged

    // Insert NULL string (invalid input)
    ck_assert_int_eq(str_insert(test_str, 0, NULL), STR_NULL);
    ck_assert_str_eq(str_get_data(test_str), "Inserted text"); // String should be unchanged
}
END_TEST

START_TEST(test_str_insert_fixed_size_fail)
{
    Str_err_t err;
    struct str *s_fixed_insert = test_str_alloc_fixed; // Pre-made, capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_insert, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_insert), 10);
    ck_assert_msg(CHECK_FLAG(s_fixed_insert->flags, STR_FLAG_FIXED_SIZE), "STR_FLAG_FIXED_SIZE should be set");
    
    err = str_set(s_fixed_insert, "Hello."); // Length 6
    ck_assert_int_eq(err, STR_OK);
    ck_assert_int_eq(str_get_size(s_fixed_insert), 6);

    // Insert 10 characters at pos 0: current 6 + 10 = 16. Exceeds capacity 10.
    err = str_insert(s_fixed_insert, 0, "XXXXXXXXXX"); 
    ck_assert_int_eq(err, STR_MAXSIZE);
    ck_assert_str_eq(str_get_data(s_fixed_insert), "Hello."); // Should be unchanged

    // Insert 3 characters at pos 0: current 6 + 3 = 9. Fits capacity 10.
    err = str_insert(s_fixed_insert, 0, "ABC");
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(s_fixed_insert), "ABCHello.");
    ck_assert_int_eq(str_get_size(s_fixed_insert), 9);
}
END_TEST


START_TEST(test_str_find)
{
    str_set(test_str, "This is a test string for find operation.");
    size_t pos_found;

    pos_found = str_find(test_str, "test", 0);
    ck_assert_int_eq(pos_found, 10);

    pos_found = str_find(test_str, "string", 0);
    ck_assert_int_eq(pos_found, 15);

    pos_found = str_find(test_str, "is", 0); // First "is"
    ck_assert_int_eq(pos_found, 2);

    pos_found = str_find(test_str, "is", 3); // Start search from index 3 (after first "is")
    ck_assert_int_eq(pos_found, 5); // Should find the second "is"

    pos_found = str_find(test_str, "is", 6); // Start search after second "is"
    ck_assert_int_eq(pos_found, STR_NPOS); // No more "is"

    pos_found = str_find(test_str, "notfound", 0);
    ck_assert_int_eq(pos_found, STR_NPOS); // Substring not present

    pos_found = str_find(test_str, "This", 10); // Search 'This' but starting later in string
    ck_assert_int_eq(pos_found, STR_NPOS);

    pos_found = str_find(test_str, "", 0); // Empty substring at beginning
    ck_assert_int_eq(pos_found, 0);

    pos_found = str_find(test_str, "", 5); // Empty substring at middle position
    ck_assert_int_eq(pos_found, 5);

    pos_found = str_find(test_str, "", str_get_size(test_str)); // Empty substring at end
    ck_assert_int_eq(pos_found, str_get_size(test_str)); // Should return string's length

    // Test with NULL inputs
    pos_found = str_find(NULL, "test", 0);
    ck_assert_int_eq(pos_found, STR_NPOS);
    pos_found = str_find(test_str, NULL, 0);
    ck_assert_int_eq(pos_found, STR_NPOS);
}
END_TEST

START_TEST(test_str_starts_with)
{
    str_set(test_str, "HelloWorldExample");
    ck_assert(str_starts_with(test_str, "Hello"));
    ck_assert(str_starts_with(test_str, "HelloWorldExample")); // Full string match
    ck_assert(str_starts_with(test_str, "")); // Empty prefix should always match
    ck_assert(!str_starts_with(test_str, "world")); // Case sensitive
    ck_assert(!str_starts_with(test_str, "World")); // Not at beginning
    ck_assert(!str_starts_with(test_str, "HelloWorldExampleTooLong")); // Longer prefix

    // Test with NULL inputs
    ck_assert(!str_starts_with(NULL, "Test")); // NULL self
    ck_assert(!str_starts_with(test_str, NULL)); // NULL prefix
}
END_TEST

START_TEST(test_str_ends_with)
{
    str_set(test_str, "HelloWorldExample");
    ck_assert(str_ends_with(test_str, "Example"));
    ck_assert(str_ends_with(test_str, "HelloWorldExample")); // Full string match
    ck_assert(str_ends_with(test_str, "")); // Empty suffix should always match
    ck_assert(!str_ends_with(test_str, "Hello")); // Not at end
    ck_assert(!str_ends_with(test_str, "example")); // Case sensitive
    ck_assert(!str_ends_with(test_str, "AHelloWorldExample")); // Longer suffix

    // Test with NULL inputs
    ck_assert(!str_ends_with(NULL, "Test")); // NULL self
    ck_assert(!str_ends_with(test_str, NULL)); // NULL suffix
}
END_TEST

// --- Test Cases for String Formatting ---

START_TEST(test_str_pad_left)
{
    str_set(test_str, "ABC"); // Length 3
    ck_assert_int_eq(str_pad_left(test_str, 5, '-'), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "--ABC");
    ck_assert_int_eq(str_get_size(test_str), 5);
    ck_assert_int_ge(str_get_capacity(test_str), 6);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_pad_left");


    str_set(test_str, "short"); // Length 5
    ck_assert_int_eq(str_pad_left(test_str, 5, '#'), STR_OK); // total_length equals current length: no padding, OK
    ck_assert_str_eq(str_get_data(test_str), "short");
    ck_assert_int_eq(str_get_size(test_str), 5);

    str_set(test_str, "long string"); // Length 11
    ck_assert_int_eq(str_pad_left(test_str, 10, '#'), STR_OK); // total_length less than current length: no change, OK
    ck_assert_str_eq(str_get_data(test_str), "long string");
    ck_assert_int_eq(str_get_size(test_str), 11);


    // Padding an empty string
    str_clear(test_str);
    ck_assert_int_eq(str_pad_left(test_str, 3, 'X'), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "XXX");
    ck_assert_int_eq(str_get_size(test_str), 3);

    // Total length requested would exceed max string size (STR_MAX_STRING_SIZE)
    str_set(test_str, ".");
    size_t target_len_too_large = STR_MAX_STRING_SIZE + 1;
    ck_assert_int_eq(str_pad_left(test_str, target_len_too_large, '-'), STR_OVERFLOW);
    ck_assert_int_eq(str_get_size(test_str), 1); // Should be unchanged

    // Test with fixed-size string (should fail as it implies growth)
    struct str *s_fixed_pad_l = test_str_alloc_fixed; // Capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_pad_l, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_pad_l), 10);
    str_set(s_fixed_pad_l, "hi"); // len 2
    ck_assert_int_eq(str_pad_left(s_fixed_pad_l, 12, 'a'), STR_MAXSIZE); // total 12 > cap 10
    ck_assert_str_eq(str_get_data(s_fixed_pad_l), "hi"); // unchanged
}
END_TEST

START_TEST(test_str_pad_right)
{
    str_set(test_str, "ABC"); // Length 3
    ck_assert_int_eq(str_pad_right(test_str, 5, '*'), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "ABC**");
    ck_assert_int_eq(str_get_size(test_str), 5);
    ck_assert_int_ge(str_get_capacity(test_str), 6);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_pad_right");

    str_set(test_str, "long"); // Length 4
    ck_assert_int_eq(str_pad_right(test_str, 4, '~'), STR_OK); // total_length equals current length: no padding, OK
    ck_assert_str_eq(str_get_data(test_str), "long");
    ck_assert_int_eq(str_get_size(test_str), 4);

    str_set(test_str, "longer string"); // Length 13
    ck_assert_int_eq(str_pad_right(test_str, 10, '~'), STR_OK); // total_length less than current length: no change, OK
    ck_assert_str_eq(str_get_data(test_str), "longer string");
    ck_assert_int_eq(str_get_size(test_str), 13);


    // Padding an empty string
    str_clear(test_str);
    ck_assert_int_eq(str_pad_right(test_str, 3, 'O'), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "OOO");
    ck_assert_int_eq(str_get_size(test_str), 3);

    // Total length requested would exceed max string size
    str_set(test_str, ".");
    size_t target_len_too_large = STR_MAX_STRING_SIZE + 1;
    ck_assert_int_eq(str_pad_right(test_str, target_len_too_large, '-'), STR_OVERFLOW);
    ck_assert_int_eq(str_get_size(test_str), 1); // Should be unchanged

    // Test with fixed-size string (should fail as it implies growth)
    struct str *s_fixed_pad_r = test_str_alloc_fixed; // Capacity 10, FIXED_SIZE
    ck_assert_ptr_ne(s_fixed_pad_r, NULL);
    ck_assert_int_eq(str_get_capacity(s_fixed_pad_r), 10);
    str_set(s_fixed_pad_r, "hi"); // len 2
    ck_assert_int_eq(str_pad_right(s_fixed_pad_r, 12, 'a'), STR_MAXSIZE); // total 12 > cap 10
    ck_assert_str_eq(str_get_data(s_fixed_pad_r), "hi"); // unchanged
}
END_TEST

START_TEST(test_str_trim_left)
{
    str_set(test_str, "   Hello World   ");
    ck_assert_int_eq(str_trim_left(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello World   ");
    ck_assert_int_eq(str_get_size(test_str), 14);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_trim_left");

    str_set(test_str, "NoLeadingSpace");
    ck_assert_int_eq(str_trim_left(test_str), STR_OK); // No leading space to trim
    ck_assert_str_eq(str_get_data(test_str), "NoLeadingSpace");
    ck_assert_int_eq(str_get_size(test_str), 14);

    str_set(test_str, "    "); // String contains only spaces
    ck_assert_int_eq(str_trim_left(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_int_eq(str_get_size(test_str), 0);

    str_set(test_str, ""); // Empty string
    ck_assert_int_eq(str_trim_left(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
}
END_TEST

START_TEST(test_str_trim_right)
{
    str_set(test_str, "   Hello World   ");
    ck_assert_int_eq(str_trim_right(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "   Hello World");
    ck_assert_int_eq(str_get_size(test_str), 14);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_trim_right");


    str_set(test_str, "NoTrailingSpace");
    ck_assert_int_eq(str_trim_right(test_str), STR_OK); // No trailing space to trim
    ck_assert_str_eq(str_get_data(test_str), "NoTrailingSpace");
    ck_assert_int_eq(str_get_size(test_str), 15);

    str_set(test_str, "    "); // String contains only spaces
    ck_assert_int_eq(str_trim_right(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_int_eq(str_get_size(test_str), 0);

    str_set(test_str, ""); // Empty string
    ck_assert_int_eq(str_trim_right(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
}
END_TEST

START_TEST(test_str_trim)
{
    str_set(test_str, "   Hello World   ");
    ck_assert_int_eq(str_trim(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "Hello World");
    ck_assert_int_eq(str_get_size(test_str), 11);
    ck_assert_msg(CHECK_FLAG(test_str->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_trim");


    str_set(test_str, "No spaces");
    ck_assert_int_eq(str_trim(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "No spaces");

    str_set(test_str, "    "); // String contains only spaces
    ck_assert_int_eq(str_trim(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
    ck_assert_int_eq(str_get_size(test_str), 0);

    str_set(test_str, ""); // Empty string
    ck_assert_int_eq(str_trim(test_str), STR_OK);
    ck_assert_str_eq(str_get_data(test_str), "");
}
END_TEST

// --- Test Cases for Input/Output ---

START_TEST(test_str_read_line)
{
    // Use test_str_io string fixture for this test
    FILE *temp_file = tmpfile(); // Create a temporary file
    ck_assert_ptr_ne(temp_file, NULL);

    fputs("First Line of Data\n", temp_file);
    fputs("Second Line of Data Without Newline", temp_file);
    rewind(temp_file); // Reset file pointer to beginning

    // Read first line
    Str_err_t err = str_read_line(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "First Line of Data");
    ck_assert_int_eq(str_get_size(test_str_io), 18);
    ck_assert_msg(CHECK_FLAG(test_str_io->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_read_line");

    // Read second line (without newline at end of file)
    err = str_read_line(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "Second Line of Data Without Newline");
    ck_assert_int_eq(str_get_size(test_str_io), 35);

    // Attempt to read past EOF (or stream error if it occurs after `fgets` fails)
    err = str_read_line(test_str_io, temp_file);
    // As per `str_read_line` implementation, `feof()` implies `STR_EMPTY`.
    // It also `str_clear`s the string, so it must be empty now.
    ck_assert_int_eq(err, STR_EMPTY); 
    ck_assert_str_eq(str_get_data(test_str_io), ""); 
    ck_assert_int_eq(str_get_size(test_str_io), 0); // Must be 0 length

    fclose(temp_file); // Close and delete temporary file
}
END_TEST

START_TEST(test_str_read_line_long_input_truncation)
{
    // Use test_str_io string fixture
    FILE *temp_file = tmpfile();
    ck_assert_ptr_ne(temp_file, NULL);

    char long_line_exceeds_chunk[CHUNK_SIZE + 50]; // Example: 4096 + 50 bytes
    memset(long_line_exceeds_chunk, 'A', sizeof(long_line_exceeds_chunk) - 1);
    long_line_exceeds_chunk[sizeof(long_line_exceeds_chunk) - 1] = '\0';
    fputs(long_line_exceeds_chunk, temp_file);
    rewind(temp_file);

    Str_err_t err = str_read_line(test_str_io, temp_file);
    // str_read_line reads into a `CHUNK_SIZE` buffer.
    // If the line is longer than `CHUNK_SIZE - 1` (for null terminator), it truncates.
    ck_assert_int_eq(err, STR_OK);
    // The `fgets` within `str_read_line` will read `CHUNK_SIZE-1` chars + null.
    // So the actual length copied to string will be `CHUNK_SIZE - 1`.
    ck_assert_int_eq(str_get_size(test_str_io), CHUNK_SIZE - 1); 
    
    // Verify content (should be all 'A's up to the limit)
    for (size_t i = 0; i < CHUNK_SIZE - 1; ++i) {
        ck_assert_msg(str_get_data(test_str_io)[i] == 'A', "Character at index %zu should be 'A'", i);
    }
    ck_assert_msg(str_get_data(test_str_io)[CHUNK_SIZE - 1] == '\0', "String should be null-terminated");

    fclose(temp_file);
}
END_TEST


START_TEST(test_str_read_word)
{
    // Use test_str_io string fixture
    FILE *temp_file = tmpfile();
    ck_assert_ptr_ne(temp_file, NULL);

    fputs("Word1  Word2\tWord3\nFinalWord", temp_file); // Multiple words with various delimiters
    rewind(temp_file);

    // Read first word
    Str_err_t err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "Word1");
    ck_assert_int_eq(str_get_size(test_str_io), 5);
    ck_assert_msg(CHECK_FLAG(test_str_io->flags, STR_FLAG_MODIFIED), "STR_FLAG_MODIFIED should be set after str_read_word");

    // Read second word (appends with a space as per implementation)
    err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "Word1 Word2");
    ck_assert_int_eq(str_get_size(test_str_io), 11);

    // Read third word
    err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "Word1 Word2 Word3");
    ck_assert_int_eq(str_get_size(test_str_io), 17);

    // Read final word (without trailing newline in source)
    err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    ck_assert_str_eq(str_get_data(test_str_io), "Word1 Word2 Word3 FinalWord");
    ck_assert_int_eq(str_get_size(test_str_io), 27);

    // Read past EOF
    err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_EMPTY); // Returns STR_EMPTY on EOF with no words
    ck_assert_str_eq(str_get_data(test_str_io), "Word1 Word2 Word3 FinalWord"); // String content should not have been modified

    fclose(temp_file);
}
END_TEST

START_TEST(test_str_read_word_buffer_overflow_prevention)
{
    // Use test_str_io string fixture
    FILE *temp_file = tmpfile();
    ck_assert_ptr_ne(temp_file, NULL);

    // `fscanf` in `str_read_word` uses CHUNK_SIZE_FSCANF which is CHUNK_SIZE - 1.
    // So `fscanf` will read up to (CHUNK_SIZE - 1) characters.
    size_t overflow_word_len = CHUNK_SIZE + 50; 
    char *word_too_long = (char *)malloc(overflow_word_len + 1);
    ck_assert_ptr_ne(word_too_long, NULL);
    memset(word_too_long, 'B', overflow_word_len);
    word_too_long[overflow_word_len] = '\0'; // This buffer is very long
    fputs(word_too_long, temp_file);
    fputs(" C", temp_file); // A separate character after the long word, potentially read by subsequent fscanf
    rewind(temp_file);

    // The str_read_word will internally use fscanf(stream, "%"XSTR(CHUNK_SIZE_FSCANF)"s", buffer)
    Str_err_t err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK); // It should successfully read the truncated part of the word

    // The length of the read word should be exactly CHUNK_SIZE - 1 (the `fscanf` buffer size)
    ck_assert_int_eq(str_get_size(test_str_io), CHUNK_SIZE - 1); 
    
    // Verify content to ensure truncation
    for (size_t i = 0; i < CHUNK_SIZE - 1; ++i) {
        ck_assert_msg(str_get_data(test_str_io)[i] == 'B', "Character at index %zu should be 'B'", i);
    }
    ck_assert_msg(str_get_data(test_str_io)[CHUNK_SIZE - 1] == '\0', "String should be null-terminated");

    // The rest of the `word_too_long` string from the file and " C" might still be in the buffer,
    // or next call will read it. This depends on `fscanf`'s behavior which leaves remaining characters.
    // If str_read_word is called again, it should continue from where `fscanf` left off.
    err = str_read_word(test_str_io, temp_file);
    ck_assert_int_eq(err, STR_OK);
    // Previous "Word1 Word2 Word3 FinalWord" + remaining of very long word + C.
    // For simpler testing here: assume 'C' was next.
    ck_assert_str_eq(str_get_data(test_str_io), "BB...B C"); // Length will be (CHUNK_SIZE - 1) + 1 + 1 (old word, space, 'C')
    ck_assert_int_ge(str_get_size(test_str_io), CHUNK_SIZE - 1 + 2); // Should be at least original plus new data + space

    free(word_too_long);
    fclose(temp_file);
}
END_TEST


// get_dyn_input directly interacts with stdin, so redirection is necessary.
// We manage this using dup and freopen, or by temporary file as done here.
START_TEST(test_get_dyn_input)
{
    // Save original stdin to restore later
    FILE *original_stdin = stdin;
    
    // Create a temporary file to use as our mocked stdin
    FILE *temp_in = tmpfile();
    ck_assert_ptr_ne(temp_in, NULL);

    fputs("Hello Dynamic Input Example\n", temp_in);
    fputs("This is a second line of text.\n", temp_in);
    fputs("Third line without newline at end", temp_in);
    rewind(temp_in); // Important: reset file pointer to the beginning

    stdin = temp_in; // Redirect stdin for the test duration

    char *input = NULL;

    // Test reading first line (longer than 20, but capped at 20)
    input = get_dyn_input(20); 
    ck_assert_ptr_ne(input, NULL);
    ck_assert_str_eq(input, "Hello Dynamic Inpu"); // Should be truncated
    ck_assert_int_eq(strlen(input), 20); // Exact length due to cap
    free(input);

    // Test reading second line (fits within max_str_size and ends with newline)
    input = get_dyn_input(100); 
    ck_assert_ptr_ne(input, NULL);
    ck_assert_str_eq(input, "This is a second line of text.");
    ck_assert_int_eq(strlen(input), 30);
    free(input);

    // Test reading third line (no newline at end of file)
    input = get_dyn_input(100); 
    ck_assert_ptr_ne(input, NULL);
    ck_assert_str_eq(input, "Third line without newline at end");
    ck_assert_int_eq(strlen(input), 33);
    free(input);

    // Attempt to read past EOF (should return NULL, meaning no characters read from EOF stream)
    input = get_dyn_input(10); 
    ck_assert_ptr_eq(input, NULL);

    fclose(temp_in);       // Close and delete the temporary file
    stdin = original_stdin; // Restore original stdin
}
END_TEST

START_TEST(test_get_dyn_input_large_input)
{
    // Save original stdin to restore later
    FILE *original_stdin = stdin;
    
    FILE *temp_in = tmpfile();
    ck_assert_ptr_ne(temp_in, NULL);

    // Create a very long line to rigorously test dynamic reallocations in get_dyn_input.
    char long_pattern[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t pattern_len = strlen(long_pattern);
    // Number of repetitions to create a string somewhat large, but still within system limits.
    // We aim for something substantial, like 1MB.
    size_t target_total_len = 1 * 1024 * 1024; // 1 MB
    size_t num_repeats = target_total_len / pattern_len; 
    
    // Write repeating pattern
    for (size_t i = 0; i < num_repeats; ++i) {
        fputs(long_pattern, temp_in);
    }
    fputs("ENDOFTEST\n", temp_in); // Terminator to avoid hanging in get_dyn_input for EOF

    rewind(temp_in);
    stdin = temp_in; // Redirect stdin

    char *input = get_dyn_input(STR_MAX_STRING_SIZE); // Request maximum possible size
    ck_assert_ptr_ne(input, NULL);
    
    // Check if the actual length matches our intended large input plus terminator
    size_t expected_final_len = (num_repeats * pattern_len) + strlen("ENDOFTEST");
    ck_assert_int_eq(strlen(input), expected_final_len);
    
    // Verify content (e.g., first few patterns and the terminator)
    char expected_start[pattern_len + 1];
    strncpy(expected_start, long_pattern, pattern_len);
    expected_start[pattern_len] = '\0';
    ck_assert(strncmp(input, expected_start, pattern_len) == 0);
    ck_assert(strcmp(input + expected_final_len - strlen("ENDOFTEST"), "ENDOFTEST") == 0);


    free(input);
    fclose(temp_in);
    stdin = original_stdin; // Restore original stdin
}
END_TEST

// No direct automated tests for str_check_err and str_print.
// They output to stdout/stderr which is complex to capture reliably across platforms within unit tests.
// Their correctness is usually verified visually or via a separate integration testing mechanism.

// --- Suite creation ---
Suite *strutil_suite(void) {
    Suite *s;
    TCase *tc_core;
    TCase *tc_mem_mgmt;
    TCase *tc_str_ops;
    TCase *tc_str_manip;
    TCase *tc_adv_str_ops;
    TCase *tc_str_fmt;
    TCase *tc_io;

    s = suite_create("strutil");

    // Core Functions Tests
    tc_core = tcase_create("Core");
    // Specific test for init/free is standalone
    tcase_add_test(tc_core, test_str_init_and_free); 
    // Other tests use the basic fixture
    tcase_add_checked_fixture(tc_core, setup_basic, teardown_basic);
    tcase_add_test(tc_core, test_str_clear);
    suite_add_tcase(s, tc_core);

    // Memory Management Tests
    tc_mem_mgmt = tcase_create("Memory_Management");
    tcase_add_checked_fixture(tc_mem_mgmt, setup_special_strings, teardown_special_strings); // Uses two strings & fixed size
    tcase_add_test(tc_mem_mgmt, test_str_alloc);
    tcase_add_test(tc_mem_mgmt, test_str_realloc_macro_basic_functionality);
    tcase_add_test(tc_mem_mgmt, test_str_realloc_fixed_size_fail); // Uses test_str_alloc_fixed (from special setup)
    tcase_add_test(tc_mem_mgmt, test_str_grow); // Uses test_str
    tcase_add_test(tc_mem_mgmt, test_str_copy); // Uses test_str and test_str2
    tcase_add_test(tc_mem_mgmt, test_str_copy_fixed_size_fail); // Uses test_str_alloc_fixed and test_str
    tcase_add_test(tc_mem_mgmt, test_str_mov); // Uses test_str and test_str2, carefully re-initializes test_str where needed within sub-tests for error paths
    suite_add_tcase(s, tc_mem_mgmt);

    // String Operations Tests
    tc_str_ops = tcase_create("String_Operations");
    tcase_add_checked_fixture(tc_str_ops, setup_special_strings, teardown_special_strings);
    tcase_add_test(tc_str_ops, test_str_add); // Uses test_str
    tcase_add_test(tc_str_ops, test_str_add_overflow); // Needs independent `struct str*` as it maxes out size
    tcase_add_test(tc_str_ops, test_str_add_fixed_size_fail); // Uses test_str_alloc_fixed
    tcase_add_test(tc_str_ops, test_str_set); // Uses test_str
    tcase_add_test(tc_str_ops, test_str_set_readonly_fail); // Uses test_str
    tcase_add_test(tc_str_ops, test_str_set_fixed_size_fail); // Uses test_str_alloc_fixed
    tcase_add_test(tc_str_ops, test_str_assign_n); // Uses test_str
    tcase_add_test(tc_str_ops, test_str_getters); // Uses test_str
    suite_add_tcase(s, tc_str_ops);

    // String Manipulation Tests
    tc_str_manip = tcase_create("String_Manipulation");
    tcase_add_checked_fixture(tc_str_manip, setup_special_strings, teardown_special_strings);
    tcase_add_test(tc_str_manip, test_str_to_upper); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_to_lower); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_to_title_case); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_reverse); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_remove_word); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_replace_word); // Uses test_str
    tcase_add_test(tc_str_manip, test_str_replace_word_fixed_size_fail); // Uses test_str_alloc_fixed
    suite_add_tcase(s, tc_str_manip);

    // Advanced String Operations Tests
    tc_adv_str_ops = tcase_create("Advanced_String_Operations");
    tcase_add_checked_fixture(tc_adv_str_ops, setup_special_strings, teardown_special_strings);
    tcase_add_test(tc_adv_str_ops, test_str_insert); // Uses test_str
    tcase_add_test(tc_adv_str_ops, test_str_insert_fixed_size_fail); // Uses test_str_alloc_fixed
    tcase_add_test(tc_adv_str_ops, test_str_find); // Uses test_str
    tcase_add_test(tc_adv_str_ops, test_str_starts_with); // Uses test_str
    tcase_add_test(tc_adv_str_ops, test_str_ends_with); // Uses test_str
    suite_add_tcase(s, tc_adv_str_ops);

    // String Formatting Tests
    tc_str_fmt = tcase_create("String_Formatting");
    tcase_add_checked_fixture(tc_str_fmt, setup_special_strings, teardown_special_strings);
    tcase_add_test(tc_str_fmt, test_str_pad_left); // Uses test_str
    tcase_add_test(tc_str_fmt, test_str_pad_right); // Uses test_str
    tcase_add_test(tc_str_fmt, test_str_trim_left); // Uses test_str
    tcase_add_test(tc_str_fmt, test_str_trim_right); // Uses test_str
    tcase_add_test(tc_str_fmt, test_str_trim); // Uses test_str
    suite_add_tcase(s, tc_str_fmt);

    // Input/Output Tests
    tc_io = tcase_create("Input_Output");
    tcase_add_checked_fixture(tc_io, setup_special_strings, teardown_special_strings); // Uses test_str_io for isolation
    tcase_add_test(tc_io, test_str_read_line);
    tcase_add_test(tc_io, test_str_read_line_long_input_truncation);
    tcase_add_test(tc_io, test_str_read_word);
    tcase_add_test(tc_io, test_str_read_word_buffer_overflow_prevention);
    tcase_add_test(tc_io, test_get_dyn_input);
    tcase_add_test(tc_io, test_get_dyn_input_large_input);
    suite_add_tcase(s, tc_io);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = strutil_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL); // CK_VERBOSE for more detail, CK_NORMAL for summary
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr); // Clean up runner

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE; // Return 0 if all tests pass
}