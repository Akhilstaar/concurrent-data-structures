#include "lockfreequeue.h"
#include "mypointerintpair.h"
#include <iostream>

bool LockFreeQueue::enq(uint32_t x)
{
    Node *curr = new Node(x);
    while (true)
    {
        PIP tail_pip = tail.load(std::memory_order_acquire);
        Node *last = tail_pip.getPtr();
        Node *next = last->next.load(std::memory_order_acquire);

        if (tail_pip == tail.load(std::memory_order_acquire)) // double check
        {
            if (next == nullptr)
            {
                if (last->next.compare_exchange_strong(next, curr, std::memory_order_release, std::memory_order_relaxed))
                {
                    PIP new_tail_pip(curr, tail_pip.getCnt() + 1); // cnt will wrap
                    tail.compare_exchange_strong(tail_pip, new_tail_pip, std::memory_order_release, std::memory_order_relaxed);
                    return true;
                }
            }
            else
            {
                PIP new_tail_pip(next, tail_pip.getCnt() + 1);
                tail.compare_exchange_strong(tail_pip, new_tail_pip, std::memory_order_release, std::memory_order_relaxed);
            }
        }
    }
}

int LockFreeQueue::deq()
{
    while (true)
    {
        PIP head_pip = head.load(std::memory_order_acquire);
        Node *first = head_pip.getPtr();
        PIP tail_pip = tail.load(std::memory_order_acquire);
        Node *last = tail_pip.getPtr();
        Node *next = first->next.load(std::memory_order_acquire);
        if (head_pip == head.load(std::memory_order_acquire))
        {
            if (first == last)
            {
                if (next == nullptr)
                    return -1;
                PIP new_tail_pip(next, tail_pip.getCnt() + 1);
                tail.compare_exchange_strong(tail_pip, new_tail_pip, std::memory_order_release, std::memory_order_relaxed);
            }
            else
            {
                int val = next->val;
                PIP new_head(next, head_pip.getCnt() + 1);
                if (head.compare_exchange_strong(head_pip, new_head, std::memory_order_release, std::memory_order_relaxed))
                {
                    delete first;
                    return val;
                }
            }
        }
    }
}

void LockFreeQueue::print()
{
    PIP head_pip = head.load();
    Node *current = head_pip.getPtr()->next.load();
    while (current != nullptr)
    {
        std::cout << current->val << " ";
        current = current->next.load();
    }
    std::cout << "\n";
}