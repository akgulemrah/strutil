/*
 * example.c - Usage examples for strutil library
 * 
 * This file demonstrates the main features and proper usage
 * of the strutil string manipulation library.
 */

#include <stdio.h>
#include "strutil.h"

// Helper function to print a separator line
static void print_separator(void) {
    printf("\n----------------------------------------\n");
}

int main(void) {
    // Basic String Operations
    print_separator();
    printf("1. Basic String Operations:\n");
    
    str *s1 = str_init();
    if (s1 == NULL) {
        printf("Failed to initialize string\n");
        return 1;
    }

    // Setting and getting string content
    str_set(s1, "Hello");
    printf("Initial string: %s\n", str_get(s1));
    
    // Adding content
    str_add(s1, ", World!");
    printf("After append: %s\n", str_get(s1));
    
    // Getting string size
    printf("String length: %zu\n", str_get_size(s1));

    // String Manipulation
    print_separator();
    printf("2. String Manipulation:\n");
    
    // Convert to uppercase
    str_to_upper(s1);
    printf("Uppercase: %s\n", str_get(s1));
    
    // Convert to lowercase
    str_to_lower(s1);
    printf("Lowercase: %s\n", str_get(s1));
    
    // Convert to title case
    str_to_title_case(s1);
    printf("Title case: %s\n", str_get(s1));
    
    // Reverse string
    str_reverse(s1);
    printf("Reversed: %s\n", str_get(s1));
    
    // Word Operations
    print_separator();
    printf("3. Word Operations:\n");
    
    str *s2 = str_init();
    str_set(s2, "The quick brown fox jumps");
    
    printf("Original: %s\n", str_get(s2));
    
    // Remove a word
    str_rem_word(s2, "quick");
    printf("After removing 'quick': %s\n", str_get(s2));
    
    // Swap words
    str_swap_word(s2, "brown", "black");
    printf("After replacing 'brown' with 'black': %s\n", str_get(s2));

    // Input/Output Operations
    print_separator();
    printf("4. Input/Output Operations:\n");
    
    str *s3 = str_init();
    printf("Enter some text: ");
    str_input(s3);
    printf("You entered: %s\n", str_get(s3));
    
    printf("Enter more text to append: ");
    str_add_input(s3);
    printf("Final string: %s\n", str_get(s3));

    // Error Handling
    print_separator();
    printf("5. Error Handling:\n");
    
    // Try to set NULL content
    Str_err_t result = str_set(s1, NULL);
    if (result == STR_NULL) {
        printf("Caught NULL pointer error successfully\n");
    }
    
    // Try to use NULL string
    result = str_add(NULL, "test");
    if (result == STR_NULL) {
        printf("Caught NULL string error successfully\n");
    }

    // Memory Management
    print_separator();
    printf("6. Memory Management:\n");
    
    // Clear string content
    str_clear(s1);
    printf("After clear, is empty: %s\n", str_is_empty(s1) ? "yes" : "no");
    
    // Clean up
    str_free(s1);
    str_free(s2);
    str_free(s3);
    printf("All resources freed successfully\n");

    return 0;
} 
