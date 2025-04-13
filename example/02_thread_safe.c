#include <stdio.h>
#include <pthread.h>
#include "strutil.h"

#define NUM_THREADS 4

struct thread_data {
    struct str *shared_str;
    const char *text;
    int id;
};

void* thread_function(void *arg) {
    struct thread_data *data = (struct thread_data*)arg;
    
    // Each thread tries to append its text to the shared string
    printf("Thread %d: Attempting to add text...\n", data->id);
    
    // The library handles thread safety internally
    if (str_add(data->shared_str, data->text) != STR_OK) {
        fprintf(stderr, "Thread %d: Failed to append text\n", data->id);
        return NULL;
    }
    
    printf("Thread %d: Successfully added text\n", data->id);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    struct thread_data thread_data_array[NUM_THREADS];
    const char *texts[] = {" Hello", " World", " from", " threads!"};
    
    // Initialize shared string
    struct str *shared_str = str_init();
    if (!shared_str) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }
    
    printf("\n=== Thread-Safe String Operations ===\n");
    
    // Set initial empty string
    if (str_set(shared_str, "") != STR_OK) {
        fprintf(stderr, "Failed to set initial string\n");
        str_free(shared_str);
        return 1;
    }
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_array[i].shared_str = shared_str;
        thread_data_array[i].text = texts[i];
        thread_data_array[i].id = i;
        
        if (pthread_create(&threads[i], NULL, thread_function, 
                         (void*)&thread_data_array[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            str_free(shared_str);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print final string
    printf("\nFinal string: %s\n", str_get_data(shared_str));
    printf("Final length: %zu\n", str_get_size(shared_str));
    
    // Clean up
    str_free(shared_str);
    return 0;
}