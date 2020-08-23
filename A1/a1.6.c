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
#include <sys/mman.h>

#define SIZE    10
int min_size = 10000;

struct block {
    int size;
    int *data;
};

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

    int a_pipe[2];

    

    

    if (left_side.size > min_size || right_side.size > min_size) {
        if (pipe(a_pipe) == -1) {
            exit(EXIT_FAILURE);
        } 
        pid_t id = fork();
        int result = 0;
        int bytes = left_side.size * sizeof(int); 
        if (id != 0) { // PARENT
            close(a_pipe[1]); // not writing
            do {
                result = result + read(a_pipe[0], left_side.data + (result/sizeof(int)), bytes); // can't read all 64000+ bytes in one go, so need to do multiple reads
            } while (result < bytes);
            wait(NULL);
            quick_sort(right_side);
            close(a_pipe[0]); // stop reading
        } else { // CHILD
            close(a_pipe[0]); // not reading
            quick_sort(left_side);
            result += write(a_pipe[1], left_side.data, bytes); // number of bytes of the data that is sorted is written into the pipe, and it is incremented to 'result'
            close(a_pipe[1]); // stop writing
            exit(EXIT_SUCCESS);
            printf("Test\n");
        }
    } else {
        quick_sort(left_side);
        quick_sort(right_side);
    }   

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
    start_block.data = (int*) mmap(NULL, start_block.size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // space in memory sharing with every process
    // start_block.data = holds starting memory address of the shared block
    // every process shares start_block.data
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
    
    quick_sort(start_block);

    times(&finish_times);
    printf("finish time in clock ticks: %ld\n", finish_times.tms_utime);

    if (start_block.size < 1001)
        print_data(start_block);

    printf(is_sorted(start_block) ? "sorted\n" : "not sorted\n");
    exit(EXIT_SUCCESS);
}