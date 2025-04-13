#include <stdio.h>
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
    printf("Original: %s\n", str_get_data(s));
    
    str_to_upper(s);
    printf("Uppercase: %s\n", str_get_data(s));
    
    str_to_lower(s);
    printf("Lowercase: %s\n", str_get_data(s));
    
    str_to_title_case(s);
    printf("Title case: %s\n", str_get_data(s));

    // String trimming
    printf("\n2. String Trimming:\n");
    str_set(s, "   Hello World   ");
    printf("Original: '%s'\n", str_get_data(s));
    
    str_trim(s);
    printf("Trimmed: '%s'\n", str_get_data(s));
    
    str_set(s, "   Left and right   ");
    printf("\nOriginal: '%s'\n", str_get_data(s));
    
    str_trim_left(s);
    printf("Left trimmed: '%s'\n", str_get_data(s));
    
    str_trim_right(s);
    printf("Right trimmed: '%s'\n", str_get_data(s));

    // Padding
    printf("\n3. String Padding:\n");
    str_set(s, "Test");
    printf("Original: '%s'\n", str_get_data(s));
    
    str_pad_left(s, 10, '*');
    printf("Left padded: '%s'\n", str_get_data(s));
    
    str_set(s, "Test");
    str_pad_right(s, 10, '*');
    printf("Right padded: '%s'\n", str_get_data(s));

    // Word operations
    printf("\n4. Word Operations:\n");
    str_set(s, "Hello beautiful world");
    printf("Original: '%s'\n", str_get_data(s));
    
    str_rem_word(s, "beautiful");
    printf("After removing 'beautiful': '%s'\n", str_get_data(s));
    
    str_set(s, "Hello old world");
    str_swap_word(s, "old", "new");
    printf("After replacing 'old' with 'new': '%s'\n", str_get_data(s));

    // String searching
    printf("\n5. String Searching:\n");
    str_set(s, "Hello World! Hello Universe!");
    printf("Original: '%s'\n", str_get_data(s));
    
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
    printf("Original: '%s'\n", str_get_data(s));
    
    str_reverse(s);
    printf("Reversed: '%s'\n", str_get_data(s));

    // Clean up
    str_free(s);
    return 0;
}