#pragma once
#include "boost/atomic.hpp"

namespace Allomere {
    template<typename T>
    class LockFreeQueue {
    public:
        struct node {
            T data;
            node* next;
        };
        void push(const T& data)
        {
            node* n = new node;
            n->data = data;
            node* stale_head = head_.load(boost::memory_order_relaxed);
            do {
                n->next = stale_head;
            } while (!head_.compare_exchange_weak(stale_head, n, boost::memory_order_release));
        }

        node* pop_all(void)
        {
            node* last = pop_all_reverse(), * first = 0;
            while (last) {
                node* tmp = last;
                last = last->next;
                tmp->next = first;
                first = tmp;
            }
            return first;
        }

        LockFreeQueue() : head_(0) {}

        // alternative interface if ordering is of no importance
        node* pop_all_reverse(void)
        {
            return head_.exchange(0, boost::memory_order_consume);
        }
    private:
        boost::atomic<node*> head_;
    };
}