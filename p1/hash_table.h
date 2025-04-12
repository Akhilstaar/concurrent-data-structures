// hash_table.h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>

struct Node
{
    unsigned int key;
    unsigned int value;
    Node *next;

    Node(unsigned int k, unsigned int v, Node *n = nullptr) : key(k), value(v), next(n) {}
};

// euivalent of atomicmarkable reference
// atomic<PointerIntPair<V *, 1>>::compare_exchange_weak(PointerIntPair<V *, 1>& expectedPair, PointerIntPair<V *, 1> newPair)
class List
{
    // private:
public:
    Node *top;
    std::atomic<int> m_count;

    List() : top(nullptr), m_count(0) {}

    ~List()
    {
        Node *curr = top;
        while (curr != nullptr)
        {
            Node *tmp = curr;
            curr = curr->next;
            delete tmp;
        }
    }

    bool insert(unsigned int key, unsigned int val)
    {
        // check if the key-val already exists
        Node *curr = top;
        while (curr != nullptr)
        {
            if (curr->key == key)
            {
                return false;
            }
            curr = curr->next;
        }
        // else, insert at top
        Node *newNode = new Node(key, val, top);
        top = newNode;
        m_count++;
        return true;
    }

    bool del(unsigned int key)
    {
        Node *curr = top;
        Node *prev = nullptr;

        while (curr != nullptr)
        {
            if (curr->key == key)
            {
                if (prev == nullptr)
                {
                    top = curr->next;
                }
                else
                {
                    prev->next = curr->next;
                }
                delete curr;
                m_count--;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    bool contains(unsigned int key)
    {
        Node *curr = top;
        while (curr != nullptr)
        {
            if (curr->key == key)
                return true;
            curr = curr->next;
        }
        return false;
    }

    std::pair<bool, unsigned int> getval(unsigned int key) const
    {
        Node *curr = top;
        while (curr != nullptr)
        {
            if (curr->key == key)
            {
                return {true, curr->value};
            }
            curr = curr->next;
        }
        return {false, 0}; // return 0 for failed search.
    }
};

class Table
{
public:
    std::vector<List *> tbl;
    std::atomic<size_t> size;
    std::atomic<size_t> capacity;

    Table(size_t cap) : size(0), capacity(cap)
    {
        tbl.resize(cap);
        for (size_t i = 0; i < cap; ++i)
        {
            tbl[i] = new List();
        }
    }

    ~Table()
    {
        for (List *list : tbl)
        {
            delete list;
        }
    }

    // TODO: hash should be a function pointer, which can change based on the size of the hash table.
    size_t hash(unsigned int key) const
    {
        return std::hash<uint32_t>{}(key) % capacity.load(std::memory_order_relaxed);
    }
};

class HashTable
{
private:
    size_t lock_length;
    Table table;
    // pthread_mutex_t *locks; // make it reentrant
    std::vector<std::recursive_mutex> locks; // I'm not sure, which one will work better

    void resize();
    bool needs_resize() const
    {
        // TODO: add some good logic for checking resizing condition
        return table.size.load() >= table.capacity.load() * 0.75;
    }

public:
    HashTable(size_t cap) : lock_length(cap), table(cap), locks(cap)
    {
        // locks = new pthread_mutex_t[cap];
        // pthread_mutexattr_t attr;
        // pthread_mutexattr_init(&attr);
        // pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); // reentrant

        // for (size_t i = 0; i < cap; ++i)
        // {
        //     pthread_mutex_init(&locks[i], &attr);
        // }

        // pthread_mutexattr_destroy(&attr);
    }

    ~HashTable() = default;
    // { delete[] locks;}

    // void acquire(unsigned int key)
    // {
    //     size_t lock_index = table.hash(key) % lock_length;

    //     while (pthread_mutex_lock(&locks[lock_index]) != 0)
    //     {
    //         std::this_thread::sleep_for(std::chrono::seconds(1));
    //     }

    //     return;
    // }

    // void release(unsigned int key)
    // {
    //     pthread_mutex_unlock(&locks[table.hash(key) % lock_length]);
    //     return;
    // }

    bool contains(unsigned int key);

    bool insert(unsigned int key, unsigned int val);

    bool remove(unsigned int key);

    std::pair<bool, unsigned int> get_value(unsigned int key);
};