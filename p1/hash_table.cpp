#ifndef USE_TBB
#include "hash_table.h"
#include <iostream>
#include <pthread.h>
#include <vector>

void HashTable::resize()
{
    size_t old_cap = table.capacity.load();

    // acquire every lock
    for (auto &lck : locks)
    {
        lck.lock();
    }

    if (old_cap != table.capacity.load())
    {
        // Someone already resized it
        for (auto &lck : locks)
        {
            lck.unlock();
        }
        return;
    }

    // resize
    size_t new_cap = old_cap * 2;
    std::vector<List *> new_tbl(new_cap);
    for (size_t i = 0; i < new_cap; ++i)
    {
        new_tbl[i] = new List();
    }

    // new hash function
    // size_t new_hash(unsigned int key) const
    // {
    //     return key % new_cap;
    // }

    // Rehash entries
    for (size_t i = 0; i < old_cap; ++i)
    {
        Node *curr = table.tbl[i]->top;
        while (curr != nullptr)
        {
            size_t new_idx = curr->key % new_cap; // TODO: Change it to use the function to compute hash.
            new_tbl[new_idx]->insert(curr->key, curr->value);
            curr = curr->next;
        }
    }

    for (List *list : table.tbl)
    {
        delete list;
    }
    table.tbl = new_tbl;
    table.capacity.store(new_cap);

    // Release all locks
    for (auto &mtx : locks)
    {
        mtx.unlock();
    }
}

bool HashTable::contains(unsigned int key)
{
    size_t idx = table.hash(key);
    std::lock_guard<std::recursive_mutex> lock(locks[idx % lock_length]);
    return table.tbl[idx]->contains(key);
}

bool HashTable::insert(unsigned int key, unsigned int val)
{
    size_t idx = table.hash(key);
    locks[idx % lock_length].lock();
    bool success = table.tbl[idx]->insert(key, val);
    // release the lock first, before contending for resize
    locks[idx % lock_length].unlock();
    if (success)
    {
        table.size++;
        if (needs_resize())
        {
            resize();
            return success;
        }
    }
    return success;
}

bool HashTable::remove(unsigned int key)
{
    size_t idx = table.hash(key);
    std::lock_guard<std::recursive_mutex> lock(locks[idx % lock_length]);
    bool result = table.tbl[idx]->del(key);
    if (result)
        table.size--;
    return result;
}

std::pair<bool, unsigned int> HashTable::get_value(unsigned int key)
{
    size_t idx = table.hash(key);
    std::lock_guard<std::recursive_mutex> lock(locks[idx % lock_length]);
    // pair<bool,unsigned int> can be used for query of different type
    return table.tbl[idx]->getval(key);
}

#endif