/*
 * example.c - Interactive demo for strutil library
 * 
 * This program demonstrates the features of strutil library
 * through an interactive menu system with comprehensive
 * error handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strutil.h"

#define STR_MAX_INPUT 1024
#define MENU_EXIT 0

//#define STR_DEMO_ERROR_HANDLING

// Helper function to clear input buffer
static void clear_input_buffer(void) {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

// Helper function to get user input safely
static void get_input(char *buffer, size_t size) {
	if (fgets(buffer, size, stdin) != NULL) {
		size_t len = strlen(buffer);
		if (len > 0 && buffer[len-1] == '\n') {
		buffer[len-1] = '\0';
		} else {
		clear_input_buffer();
		}
	}
}

// Helper function to print separator
static void print_separator(void) {
	printf("\n=====================================\n");
}

// Display main menu and get user choice
static int show_menu(void) {
	int choice = -1;
	
	print_separator();
	printf("StrUtil Library Demo\n");
	print_separator();
	printf("1. Basic String Operations\n");
	printf("2. String Manipulation\n");
	printf("3. Word Operations\n");
	printf("4. Input/Output Operations\n");
	printf("5. Error Handling Demo\n");
	printf("0. Exit\n");
	print_separator();
	
	printf("Enter your choice (0-5): ");
	if (scanf("%d", &choice) != 1) {
		choice = -1;
	}
	clear_input_buffer();
	return choice;
}

// Demo: Basic String Operations
static void demo_basic_operations(void) {
	print_separator();
	printf("Basic String Operations Demo\n");
	print_separator();
	
	struct str *s = str_init();
	if (s == NULL) {
		printf("Error: Failed to initialize string\n");
		return;
	}

	printf("Enter a string: ");
	if (str_input(s, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		return;
	}

	printf("\nCurrent string: %s\n", str_get_data(s));
	printf("String length: %zu\n", str_get_size(s));
	
	printf("\nEnter text to append: ");
	if (str_add_input(s, stdin) != STR_OK) {
		printf("Error: Failed to append input\n");
		str_free(s);
		return;
	}

	printf("After append: %s\n", str_get_data(s));

	str_free(s);
	printf("\nPress Enter to continue...");
	getchar();
}

// Demo: String Manipulation
static void demo_string_manipulation(void) {
	print_separator();
	printf("String Manipulation Demo\n");
	print_separator();
	
	struct str *s = str_init();
	if (s == NULL) {
		printf("Error: Failed to initialize string\n");
		return;
	}
	
	printf("Enter a string: ");
	if (str_input(s, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		return;
	}
	printf("\nOriginal string: %s\n", str_get_data(s));
	
	if (str_to_upper(s) != STR_OK) {
		printf("Error: Failed to convert to uppercase\n");
		str_free(s);
		return;
	}
	printf("Uppercase: %s\n", str_get_data(s));
	
	if (str_to_lower(s) != STR_OK) {
		printf("Error: Failed to convert to lowercase\n");
		str_free(s);
		return;
	}
	printf("Lowercase: %s\n", str_get_data(s));
	
	if (str_to_title_case(s) != STR_OK) {
		printf("Error: Failed to convert to title case\n");
		str_free(s);
		return;
	}
	printf("Title case: %s\n", str_get_data(s));
	
	if (str_reverse(s) != STR_OK) {
		printf("Error: Failed to reverse string\n");
		str_free(s);
		return;
	}
	printf("Reversed: %s\n", str_get_data(s));
	
	str_free(s);
	printf("\nPress Enter to continue...");
	getchar();
}

// Demo: Word Operations
static void demo_word_operations(void) {
	print_separator();
	printf("Word Operations Demo\n");
	print_separator();
	
	struct str *s = str_init();
	if (s == NULL) {
		printf("Error: Failed to initialize string\n");
		return;
	}

	printf("Enter a sentence: ");
	if (str_input(s, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		return;
	}
	printf("\nOriginal: %s\n", str_get_data(s));
	
	struct str *s2 = str_init();
	if (s2 == NULL) {
		printf("Error: Failed to initialize string\n");
		free(s);
		return;
	}

	printf("Enter word to remove: ");
	if (str_input(s2, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		str_free(s2);
		return;
	}
	
	if (str_rem_word(s, str_get_data(s2)) != STR_OK) {
		printf("Error: Word not found or removal failed\n");
	}
	printf("After removal: %s\n", str_get_data(s));
	
	printf("\nEnter word to replace: ");
	str_clear(s2);
	if (str_input(s2, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		str_free(s2);
		return;
	}

	struct str *replacement = str_init();
	if (replacement == NULL) {
		printf("Error: Failed to initialize string\n");
		free(s);
		free(s2);
		return;
	}
	printf("Enter replacement word: ");
	if (str_input(replacement, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		str_free(s2);
		str_free(replacement);
		return;
	}
	
	if (str_swap_word(s, str_get_data(s2), str_get_data(replacement)) != STR_OK) {
		printf("Error: Word not found or replacement failed\n");
	} else {
		printf("After replacement: %s\n", str_get_data(s));
	}
	
	str_free(s);
	str_free(s2);
	str_free(replacement);
	printf("\nPress Enter to continue...");
	getchar();
}

// Demo: Input/Output Operations
static void demo_io_operations(void) {
	print_separator();
	printf("Input/Output Operations Demo\n");
	print_separator();
	
	struct str *s = str_init();
	if (s == NULL) {
		printf("Error: Failed to initialize string\n");
		return;
	}
	
	printf("Enter text (press Enter when done):\n");
	if (str_input(s, stdin) != STR_OK) {
		printf("Error: Failed to read input\n");
		str_free(s);
		return;
	}
	printf("\nYou entered: %s\n", str_get_data(s));
	printf("Length: %zu characters\n", str_get_size(s));
	
	printf("\nEnter additional text to append:\n");
	if (str_add_input(s, stdin) != STR_OK) {
		printf("Error: Failed to append input\n");
	} else {
		printf("\nFinal string: %s\n", str_get_data(s));
		printf("Final length: %zu characters\n", str_get_size(s));
	}
	
	str_free(s);
	printf("\nPress Enter to continue...");
	getchar();
}

#if defined(STR_DEMO_ERROR_HANDLING)
// Demo: Error Handling
static void demo_error_handling(void) {
	print_separator();
	printf("Error Handling Demo\n");
	print_separator();
	
	struct str *s = str_init();
	if (s == NULL) {
		printf("Error: Failed to initialize string\n");
		return;
	}
	
	printf("Testing NULL pointer handling:\n");
	Str_err_t result = str_set(NULL, "test");
	printf("Set NULL string result: %s\n", 
		result == STR_NULL ? "Caught NULL error" : "Unexpected result");
	
	result = str_set(s, NULL);
	printf("Set NULL content result: %s\n",
		result == STR_NULL ? "Caught NULL error" : "Unexpected result");
	
	printf("\nTesting empty string handling:\n");
	result = str_set(s, "");
	printf("Set empty string result: %s\n",
		result == STR_INVALID ? "Caught invalid input" : "Unexpected result");
	
	printf("\nTesting memory management:\n");
	str_clear(s);
	printf("String empty after clear: %s\n",
		str_is_empty(s) ? "yes" : "no");
	
	str_free(s);
	printf("Resources freed successfully\n");
	
	printf("\nPress Enter to continue...");
	getchar();
}
#undef STR_DEMO_ERROR_HANDLING
#endif

int main(void) {
	int choice;
	
	while ((choice = show_menu()) != MENU_EXIT) {
		switch (choice) {
		case 1:
			demo_basic_operations();
			break;
		case 2:
			demo_string_manipulation();
			break;
		case 3:
			demo_word_operations();
			break;
		case 4:
			demo_io_operations();
			break;
		case 5:
#if defined(STR_DEMO_ERROR_HANDLING)
			demo_error_handling();
			break;
#else
			printf("Error handling demo is disabled in this build.\n");
			break;
#endif
		default:
			printf("Invalid choice. Please try again.\n");
			break;
		}
	}
	
	print_separator();
	printf("Thank you for using StrUtil Library Demo!\n");
	return 0;
}

