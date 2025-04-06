#include "lockfreequeue.h"

template <typename T>
void LockFreeQueue<T>::enq(T x)
{
    Node curr = new Node(x);
    while (true)
    {
        Node last = tail.load();
        Node next = last.next.load();
        if (last == tail.load())
        {
            if (next == nullptr)
            {
                if (last.next.compare_exchange_strong(next, curr))
                {
                    tail.compare_exchange_strong(last, curr);
                    return;
                }
            }
            else
            {
                tail.compare_exchange_strong(last, next);
            }
        }
    }
}

// TODO: deq() operation on an empty queue should return a negative integer, i.e., it will not block.
template <typename T>
void LockFreeQueue<T>::deq(T x)
{
    while (true)
    {
        Node first = head.load();
        Node last = tail.load();
        Node next = first.next.load();
        if (first == head.load()){ // Double check
            if (first == last){
                if (next == nullptr)
                {
                    // add code here
                }
                tail.compare_exchange_strong(last, next);
            }
            else
            {
                T val = next.val;
                if (head.compare_exchange_strong(first, next))
                return val;
            }
        }
    }
}

template <typename T>
void LockFreeQueue<T>::print()
{
    Node first = head.load();
    Node nxt = first.next.load();

    while (nxt != nullptr)
    {
        cout << nxt.val() << " ";
        nxt = nxt.next.load();
    }
    cout << '\n';
    return;
}