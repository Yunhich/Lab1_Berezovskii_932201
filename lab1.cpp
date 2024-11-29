#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;

// Переменные
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;

// Функция-поставщик
void* producer(void* arg) {
    while (true) {
        pthread_mutex_lock(&lock);

        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        ready = 1;
        cout << "provided\n";
        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

// Функция-потребитель
void* consumer(void* arg) {
    while (true) {
        pthread_mutex_lock(&lock);

        while (ready == 0) {
            pthread_cond_wait(&cond1, &lock);
            cout << "awoke\n";
        }

        ready = 0;
        cout << "consumed\n";
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

int main() {
    pthread_t producerThread, consumerThread;

    pthread_create(&producerThread, nullptr, producer, nullptr);
    pthread_create(&consumerThread, nullptr, consumer, nullptr);

    return 0;
}
