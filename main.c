#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

static const long Num_To_Add = 1000000000;
static const double Scale = 10.0 / RAND_MAX;

// structure for message queue
struct msg_buffer {
    long msg_type;
    long msg_number;
};

long add_serial(const char *numbers) {
    long sum = 0;
    for (long i = 0; i < Num_To_Add; i++) {
        sum += numbers[i];
    }
    return sum;
}

long add_parallel(const char *numbers) {
    long sum = 0;
    long sum_array_1[2] = {0, 0};
    long sum_2 = 0;
    long my_sum, my_first_i, my_last_i, my_i;
    key_t key;
    int msgid;
    int thread_num;
    int number_of_threads = omp_get_max_threads();
    struct msg_buffer message;

    /* ftok to generate unique key
    key = ftok("progfile", 65);

    // msgget creates a message queue
    // and returns identifier
    if ((msgid = msgget(key, 0666)) < 0) {
        printf("Failure!");
    }*/

    #pragma omp parallel private(my_sum, my_first_i, my_last_i, my_i, message, thread_num) num_threads(number_of_threads) reduction(+:sum)
    {
        thread_num = omp_get_thread_num();
        my_sum = 0;
        my_first_i = omp_get_thread_num() * Num_To_Add / number_of_threads;
        my_last_i = my_first_i + Num_To_Add / number_of_threads;
        for (my_i = my_first_i; my_i < my_last_i; my_i++) {
            my_sum += numbers[my_i];
        }

        //printf("Sum for Thread %i: %i\n", omp_get_thread_num(), my_sum);
        //message.msg_type = 1;

        if (thread_num == 0)
        {
            while (sum_array_1[0] == 0);
            my_sum = my_sum + sum_array_1[0];
            while (sum_2 == 0);
            sum = my_sum + sum_2;
        }
        else if (thread_num == 1)
        {
            sum_array_1[0] = my_sum;
        }
        else if (thread_num == 2)
        {
            while (sum_array_1[1] == 0);
            sum_2 = my_sum + sum_array_1[1];
        }
        else if (thread_num == 3)
        {
            sum_array_1[1] = my_sum;
        }

        /*if (omp_get_thread_num() == 0)
        {
            while (sum_array_2[0] == 0 || sum_array_2[1] == 0);
            sum = sum_array_2[0] + sum_array_2[1];
        }*/

        /*if (omp_get_thread_num() == 0){
            while (sum_array_2[0] == 0 || sum_array_2[1] == 0);
            sum = sum_array_2[0] + sum_array_2[1];
        }*/

        /*if (omp_get_thread_num() == 0)
        {
            while (sum_array[1] == 0 || sum_array[2] == 0 || sum_array[3] == 0);
            sum = my_sum + sum_array[1] + sum_array[2] + sum_array[3];
        }
        else
        {
            sum_array[omp_get_thread_num()] = my_sum;
        }

        /*if (omp_get_thread_num() == 0)
        {
            sum = my_sum;
            for (my_i = 1; my_i < omp_get_max_threads(); my_i++) {
                msgrcv(msgid, &message, sizeof(message), 1, 0);
                sum += message.msg_number;
            }
            msgctl(msgid, IPC_RMID, NULL);
        }
        else
        {
            message.msg_number = my_sum;
            // msgsnd to send message
            msgsnd(msgid, &message, sizeof(message), 0);
        }*/
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

