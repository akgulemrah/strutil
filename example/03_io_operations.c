#include <stdio.h>
#include "strutil.h"

/*
 * I/O Operations Example
 * This example demonstrates file and stream operations
 */
int main() {
    struct str *s = str_init();
    if (!s) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }

    printf("\n=== I/O Operations Example ===\n");

    // Write sample data to file
    FILE *fp = fopen("sample.txt", "w");
    if (!fp) {
        fprintf(stderr, "Failed to create file\n");
        str_free(s);
        return 1;
    }
    fprintf(fp, "First line\nSecond line\nThird line\n");
    fclose(fp);

    // Read entire file content
    fp = fopen("sample.txt", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file\n");
        str_free(s);
        return 1;
    }

    printf("\nReading entire file at once:\n");
    if (str_input(s, fp) != STR_OK) {
        fprintf(stderr, "Failed to read file\n");
        fclose(fp);
        str_free(s);
        return 1;
    }
    printf("File content: %s\n", str_get_data(s));
    
    // Reset file position and clear string
    rewind(fp);
    str_clear(s);

    // Read file word by word
    printf("\nReading file word by word:\n");
    while (str_add_input(s, fp) == STR_OK) {
        printf("Read word: %s\n", str_get_data(s));
        str_clear(s);
    }

    fclose(fp);

    // Dynamic input demonstration
    printf("\nDynamic input demonstration:\n");
    printf("Enter some text (max 100 chars): ");
    char *dynamic_input = get_dyn_input(100);
    if (dynamic_input) {
        printf("You entered: %s\n", dynamic_input);
        free(dynamic_input);
    }

    // Clean up
    str_free(s);
    remove("sample.txt");  // Delete the temporary file
    return 0;
}