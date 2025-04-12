// lockfreequeue.h
#include <atomic>
#include "MyPointerIntPair.h"

struct Node
{
    uint32_t val;
    std::atomic<Node *> next;
    Node(uint32_t x, Node *nxt = nullptr) : val(x), next(nxt) {}
};

using PIP = MyPointerIntPair<Node *>;
class LockFreeQueue
{
public:
    std::atomic<PIP> head;
    std::atomic<PIP> tail;

    LockFreeQueue()
    {
        Node *n = new Node(0);
        PIP initial(n, 0);
        head.store(initial, std::memory_order_relaxed);
        tail.store(initial, std::memory_order_relaxed);
    }

    ~LockFreeQueue()
    {
        // Basic cleanup. Not thread-safe. Assumes queue is quiescent.
        Node *current = head.load().getPtr();
        while (current != nullptr)
        {
            Node *next = current->next.load();
            delete current;
            current = next;
        }
    }

    bool enq(uint32_t x);
    int deq();
    void print();
};