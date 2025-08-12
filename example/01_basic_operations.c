#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

/*
 * Basic String Operations Example
 * This example demonstrates the fundamental operations of the strutil library
 */
int main() {
    // Initialize a new string
    struct str *s = str_init();
    if (!s) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }

    // Basic string operations
    printf("\n=== Basic String Operations ===\n");
    
    // Set initial content
    if (str_set(s, "Hello") != STR_OK) {
        fprintf(stderr, "Failed to set string\n");
        str_free(s);
        return 1;
    }
    char *s_data = str_get_data(s);
    if (!s_data) {
        fprintf(stderr, "Failed to get string data\n");
        str_free(s);
        return 1;
    }
    printf("Initial string: %s\n", s_data);
    free(s_data);
    
    // Append to string
    if (str_add(s, " World!") != STR_OK) {
        fprintf(stderr, "Failed to append string\n");
        str_free(s);
        return 1;
    }
    s_data = str_get_data(s);
    if (!s_data) {
        fprintf(stderr, "Failed to get string data after append\n");
        str_free(s);
        return 1;
    }
    printf("After append: %s\n", s_data);
    free(s_data);
    
    // Get string properties
    printf("Length: %zu\n", str_get_size(s));
    printf("Capacity: %zu\n", str_get_capacity(s));
    printf("Is empty: %s\n", str_is_empty(s) ? "yes" : "no");

    // Clear the string
    str_clear(s);
    printf("After clear - Is empty: %s\n", str_is_empty(s) ? "yes" : "no");

    // Clean up
    str_free(s);
    return 0;
}