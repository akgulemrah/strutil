#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

/*
 * String Manipulation Example
 * This example demonstrates various string manipulation operations
 */
int main() {
    struct str *s = str_init();
    if (!s) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }

    printf("\n=== String Manipulation Examples ===\n");

    // Case conversion
    if (str_set(s, "Hello World") != STR_OK) {
        fprintf(stderr, "Failed to set string\n");
        str_free(s);
        return 1;
    }

    printf("\n1. Case Conversion:\n");
    char *data = str_get_data(s);
    printf("Original: %s\n", data);
    free(data);
    
    str_to_upper(s);
    data = str_get_data(s);
    printf("Uppercase: %s\n", data);
    free(data);
    
    str_to_lower(s);
    data = str_get_data(s);
    printf("Lowercase: %s\n", data);
    free(data);
    
    str_to_title_case(s);
    data = str_get_data(s);
    printf("Title case: %s\n", data);
    free(data);

    // String trimming
    printf("\n2. String Trimming:\n");
    str_set(s, "   Hello World   ");
    data = str_get_data(s);
    printf("Original: '%s'\n", data);
    free(data);
    
    str_trim(s);
    data = str_get_data(s);
    printf("Trimmed: '%s'\n", data);
    free(data);
    
    str_set(s, "   Left and right   ");
    data = str_get_data(s);
    printf("\nOriginal: '%s'\n", data);
    free(data);
    
    str_trim_left(s);
    data = str_get_data(s);
    printf("Left trimmed: '%s'\n", data);
    free(data);
    
    str_trim_right(s);
    data = str_get_data(s);
    printf("Right trimmed: '%s'\n", data);
    free(data);

    // Padding
    printf("\n3. String Padding:\n");
    str_set(s, "Test");
    data = str_get_data(s);
    printf("Original: '%s'\n", data);
    free(data);
    
    str_pad_left(s, 10, '*');
    data = str_get_data(s);
    printf("Left padded: '%s'\n", data);
    free(data);
    
    str_set(s, "Test");
    str_pad_right(s, 10, '*');
    data = str_get_data(s);
    printf("Right padded: '%s'\n", data);
    free(data);

    // Word operations
    printf("\n4. Word Operations:\n");
    str_set(s, "Hello beautiful world");
    data = str_get_data(s);
    printf("Original: '%s'\n", data);
    free(data);
    
    str_remove_word(s, "beautiful");
    data = str_get_data(s);
    printf("After removing 'beautiful': '%s'\n", data);
    free(data);
    
    str_set(s, "Hello old world");
    str_replace_word(s, "old", "new");
    data = str_get_data(s);
    printf("After replacing 'old' with 'new': '%s'\n", data);
    free(data);

    // String searching
    printf("\n5. String Searching:\n");
    str_set(s, "Hello World! Hello Universe!");
    data = str_get_data(s);
    printf("Original: '%s'\n", data);
    free(data);
    
    size_t pos = str_find(s, "World", 0);
    if (pos != STR_NPOS) {
        printf("Found 'World' at position: %zu\n", pos);
    }
    
    printf("Starts with 'Hello': %s\n", 
           str_starts_with(s, "Hello") ? "yes" : "no");
    printf("Ends with 'Universe!': %s\n", 
           str_ends_with(s, "Universe!") ? "yes" : "no");

    // String reversal
    printf("\n6. String Reversal:\n");
    str_set(s, "Hello World");
    data = str_get_data(s);
    printf("Original: '%s'\n", data);
    free(data);
    
    str_reverse(s);
    data = str_get_data(s);
    printf("Reversed: '%s'\n", data);
    free(data);

    // Clean up
    str_free(s);
    return 0;
}