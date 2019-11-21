#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ipc.h>

static const long Num_To_Add = 1000000000;
static const double Scale = 10.0 / RAND_MAX;

long add_serial(const char *numbers) {
    long sum = 0;
    for (long i = 0; i < Num_To_Add; i++) {
        sum += numbers[i]; // calculate sum sequentially
    }
    return sum;
}

long add_parallel(const char *numbers) {
    long sum = 0; // final sum value
    long sum_array[3] = {0, 0, 0}; // holds intermediary sum values

    long my_sum, my_first_i, my_last_i, my_i; //private variables for computing sums in individual threads
    int thread_num;
    int number_of_threads = omp_get_max_threads();

    #pragma omp parallel private(my_sum, my_first_i, my_last_i, my_i, thread_num) num_threads(number_of_threads) reduction(+:sum)
    {
        thread_num = omp_get_thread_num();
        my_sum = 0;
        my_first_i = omp_get_thread_num() * Num_To_Add / number_of_threads; // start at appropriate location in array
        my_last_i = my_first_i + Num_To_Add / number_of_threads; // end at appropriate location in array
        for (my_i = my_first_i; my_i < my_last_i; my_i++) {
            my_sum += numbers[my_i]; // compute sum for subsection of array
        }

        if (thread_num == 0)
        {
            while (sum_array[0] == 0); // spin until thread 1 is finished adding its sum to the array
            my_sum = my_sum + sum_array[0]; // add thread 1's value
            while (sum_array[2] == 0); // spin until thread 2 has finished adding the sum of its value and thread 3's value to the array
            sum = my_sum + sum_array[2]; // compute final sum
        }
        else if (thread_num == 1)
        {
            sum_array[0] = my_sum; // add value to array
        }
        else if (thread_num == 2)
        {
            while (sum_array[1] == 0); // spin until thread 3 is finished adding its sum to the array
            sum_array[2] = my_sum + sum_array[1]; // add value and value from thread 3 and add to array
        }
        else if (thread_num == 3)
        {
            sum_array[1] = my_sum; // add value to array
        }
    };

    return sum;
}

int main() {
    char *numbers = malloc(sizeof(long) * Num_To_Add);

    long chunk_size = Num_To_Add / omp_get_max_threads();
    #pragma omp parallel num_threads(omp_get_max_threads())
    {
        int p = omp_get_thread_num();
        unsigned int seed = (unsigned int) time(NULL) + (unsigned int) p;
        long chunk_start = p * chunk_size;
        long chunk_end = chunk_start + chunk_size;
        for (long i = chunk_start; i < chunk_end; i++) {
            numbers[i] = (char) (rand_r(&seed) * Scale);
        }
    }

    struct timeval start, end;

    printf("Timing sequential...\n");
    gettimeofday(&start, NULL);
    long sum_s = add_serial(numbers);
    gettimeofday(&end, NULL);
    printf("Took %f seconds\n\n", end.tv_sec - start.tv_sec + (double) (end.tv_usec - start.tv_usec) / 1000000);

    printf("Timing parallel...\n");
    gettimeofday(&start, NULL);
    long sum_p = add_parallel(numbers);
    gettimeofday(&end, NULL);
    printf("Took %f seconds\n\n", end.tv_sec - start.tv_sec + (double) (end.tv_usec - start.tv_usec) / 1000000);

    printf("Sum serial: %ld\nSum parallel: %ld", sum_s, sum_p);

    free(numbers);
    return 0;
}

