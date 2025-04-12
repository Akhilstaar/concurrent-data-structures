// #include "lockfreequeue.h"
// #include <pthread.h>
// #include <iostream>
// #include <atomic>
// #include <vector>

// // Shared atomic counters
// std::atomic<int> total_enqueues(0);
// std::atomic<int> total_dequeues(0);
// const int NUM_OPERATIONS = 10000000;
// const int NUM_PROD_THREADS = 4;
// const int NUM_CONS_THREADS = 4;

// struct ThreadArgs
// {
//     LockFreeQueue *queue;
//     int thread_id;
// };

// void *enqueue(void *arg)
// {
//     ThreadArgs *args = static_cast<ThreadArgs *>(arg);
//     LockFreeQueue *q = args->queue;

//     for (int i = 0; i < NUM_OPERATIONS; ++i)
//     {
//         int value = args->thread_id * NUM_OPERATIONS + i; // Unique values
//         q->enq(value);
//         total_enqueues.fetch_add(1, std::memory_order_relaxed);
//     }

//     delete args;
//     return nullptr;
// }

// void *dequeue(void *arg)
// {
//     LockFreeQueue *q = static_cast<LockFreeQueue *>(arg);

//     for (int i = 0; i < NUM_OPERATIONS; ++i)
//     {
//         int val = q->deq();
//         if (val != -1)
//         {
//             total_dequeues.fetch_add(1, std::memory_order_relaxed);
//         }
//     }

//     return nullptr;
// }

// int main()
// {
//     LockFreeQueue queue;
//     std::vector<pthread_t> enqueues(NUM_PROD_THREADS);
//     std::vector<pthread_t> dequeues(NUM_CONS_THREADS);

//     // Create enqueue threads
//     for (int i = 0; i < NUM_PROD_THREADS; ++i)
//     {
//         ThreadArgs *args = new ThreadArgs{&queue, i};
//         pthread_create(&enqueues[i], nullptr, enqueue, args);
//     }

//     // Wait for enqueues
//     for (auto &t : enqueues)
//     {
//         pthread_join(t, nullptr);
//     }
    
//     // Create dequeue threads
//     for (int i = 0; i < NUM_CONS_THREADS; ++i)
//     {
//         pthread_create(&dequeues[i], nullptr, dequeue, &queue);
//     }
    
//     // Wait for dequeues
//     for (auto &t : dequeues)
//     {
//         pthread_join(t, nullptr);
//     }

//     // Verify results
//     int remaining = 0;
//     while (true)
//     {
//         int val = queue.deq();
//         if (val == -1)
//             break;
//         remaining++;
//     }

//     std::cout << "Total enqueues: " << total_enqueues.load() << "\n";
//     std::cout << "Total dequeues: " << total_dequeues.load() << "\n";
//     std::cout << "Remaining elements: " << remaining << "\n";
//     std::cout << "Validation: "
//               << (total_enqueues == total_dequeues + remaining ? "PASS" : "FAIL")
//               << "\n";

//     return 0;
// }