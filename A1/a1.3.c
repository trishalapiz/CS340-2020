/*
    The sorting program to use for Operating Systems Assignment 1 2020
    written by Robert Sheehan
    Modified by: Trisha Lapiz
    UPI: tlap632
    By submitting a program you are claiming that you and only you have made
    adjustments and additions to this code.
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <sys/times.h>
#include <pthread.h>

#define SIZE    10
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // intialises 'cond' statically
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int done = -1;
bool busy = true;

struct block {
    int size;
    int *data;
};

struct block new_block;

void print_data(struct block my_data) {
    for (int i = 0; i < my_data.size; ++i)
        printf("%d ", my_data.data[i]);
    printf("\n");
}

/* Split the shared array around the pivot, return pivot index. */
int split_on_pivot(struct block my_data) {
    int right = my_data.size - 1;
    int left = 0;
    int pivot = my_data.data[right];
    while (left < right) {
        int value = my_data.data[right - 1];
        if (value > pivot) {
            my_data.data[right--] = value;
        } else {
            my_data.data[right - 1] = my_data.data[left];
            my_data.data[left++] = value;
        }
    }
    my_data.data[right] = pivot;
    return right;
}

/* Quick sort the data. */
void quick_sort(struct block my_data) {
    if (my_data.size < 2)
        return;
    int pivot_pos = split_on_pivot(my_data);

    struct block left_side, right_side;

    left_side.size = pivot_pos;
    left_side.data = my_data.data;
    right_side.size = my_data.size - pivot_pos - 1;
    right_side.data = my_data.data + pivot_pos + 1;

    if (!busy) { // main checks if second thread not busy
        busy = true; // now it's busy cause it has work to do
        new_block.data = left_side.data;
        new_block.size = left_side.size;
        pthread_cond_signal(&cond); // main tells the second thread to continue
        quick_sort(right_side);
    } else {
        quick_sort(left_side);
        quick_sort(right_side);
    }

}

void* temp_func(void* arg) { // arg = points to the address of the sublist
    struct block* my_data = (struct block*) arg; // my_data is a struct 'block' pointer type, and it's pointing to the sublist
    quick_sort(*my_data);

    pthread_mutex_lock(&lock);
    while (done != 0) { // if second thread is busy
        busy = false;
        pthread_cond_wait(&cond, &lock); 
        // unlocks the mutex, 'cond' waits to be signalled, 
        // SECOND THREAD is suspended until 'cond' is signalled
        // releases associated mutex lock before blocking
        busy = true; // second thread is bust again
        quick_sort(new_block);
        pthread_cond_signal(&cond);
        // unblock ONE of the threads that are WAITING on 'cond'
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* Check to see if the data is sorted. */
bool is_sorted(struct block my_data) {
    bool sorted = true;
    for (int i = 0; i < my_data.size - 1; i++) {
        if (my_data.data[i] > my_data.data[i + 1])
            sorted = false;
    }
    return sorted;
}

/* Fill the array with random data. */
void produce_random_data(struct block my_data) {
    srand(1); // the same random data seed every time
    for (int i = 0; i < my_data.size; i++) {
        my_data.data[i] = rand() % 1000;
    }
}

int main(int argc, char *argv[]) {
	long size;

	if (argc < 2) {
		size = SIZE;
	} else {
		size = atol(argv[1]);
	}
    struct block start_block;
    start_block.size = size;
    start_block.data = (int *)calloc(size, sizeof(int));
    if (start_block.data == NULL) {
        printf("Problem allocating memory.\n");
        exit(EXIT_FAILURE);
    }

    produce_random_data(start_block);

    if (start_block.size < 1001)
        print_data(start_block);

    struct tms start_times, finish_times;
    times(&start_times);
    printf("start time in clock ticks: %ld\n", start_times.tms_utime);

    // PARTITION THE DATA INITIALLY
    int pivot_pos = split_on_pivot(start_block);

    struct block left_side, right_side;

    left_side.size = pivot_pos;
    left_side.data = start_block.data;
    right_side.size = start_block.size - pivot_pos - 1;
    right_side.data = start_block.data + pivot_pos + 1;
    
    pthread_t somethread;

    // BOTH THREADS RUN AT THE SAME TIME
    pthread_create(&somethread, NULL, (void *)temp_func, (void *)&left_side); // SECOND THREAD
    quick_sort(right_side); // main thread
    done = 0;
    pthread_join(somethread, NULL);

    times(&finish_times);
    printf("finish time in clock ticks: %ld\n", finish_times.tms_utime);

    if (start_block.size < 1001)
        print_data(start_block);

    printf(is_sorted(start_block) ? "sorted\n" : "not sorted\n");
    free(start_block.data);
    exit(EXIT_SUCCESS);
}

