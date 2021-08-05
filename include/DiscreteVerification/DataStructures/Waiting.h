/* 
 * File:   Waiting.h
 * Author: Peter G. Jensen
 *
 * Created on 27 September 2015, 15:09
 */

#ifndef WAITING_H
#define    WAITING_H

#include "NonStrictMarkingBase.hpp"
#include "DiscreteVerification/SearchStrategies/WeightQueryVisitor.hpp"
#include "Core/QueryParser/AST.hpp"


#include <queue>
#include <stack>
#include <vector>
#include <ctime>

namespace VerifyTAPN { namespace DiscreteVerification {
    class WeightQueryVisitor;

    template<typename T>
    class weightedqueue_t {
    public:
        virtual
        void push(T item) = 0;

        virtual
        T pop() = 0;

        virtual
        bool empty() = 0;
        
        virtual ~weightedqueue_t() = default;
    };


    template<typename T>
    class bfs_queue : public weightedqueue_t<T> {
    protected:
        std::queue<T> queue;

    public:
        virtual
        void push(T item) {
            queue.push(item);
        }

        virtual
        T pop() {
            T item = queue.front();
            queue.pop();
            return item;
        }

        virtual
        bool empty() {
            return queue.empty();
        }
    };

    template<typename T>
    class dfs_queue : public weightedqueue_t<T> {
    protected:
        std::stack<T> stack;

    public:
        virtual
        void push(T item) {
            stack.push(item);
        }

        virtual
        T pop() {
            T item = stack.top();
            stack.pop();
            return item;
        }

        virtual
        bool empty() {
            return stack.empty();
        }
    };

    template<typename T>
    class random_queue : public weightedqueue_t<T> {
    protected:
        std::vector<T> container;

    public:
        virtual
        void push(T item) {
            container.push_back(item);
        }

        virtual
        T pop() {
            size_t index = rand() % container.size();
            T item = container[index];
            container.erase(container.begin() + index);
            return item;
        }

        virtual
        bool empty() {
            return container.empty();
        }
    };

    template<typename T>
    class covermost_queue : public weightedqueue_t<T> {
    protected:
        struct weighteditem_t {
            size_t weight;

            T item;

            bool operator<(const weighteditem_t &other) const {
                return weight > other.weight;
            }
        };

        AST::Query *query;
        std::priority_queue<weighteditem_t> queue;
        size_t weight = 0;

    public:
        explicit covermost_queue(AST::Query *q) : query(q) {
        }

        virtual
        void set_weight(NonStrictMarking *marking) {
            // because circular reference, new here!
/*                WeightQueryVisitor* visitor = new WeightQueryVisitor(*marking);
                IntResult weight_c;
                query->accept(*visitor, weight_c);
                weight = weight_c.value;*/
        }

        virtual
        void push(T item) {
            weighteditem_t weighted = {weight:weight, item:item};
            queue.push(weighted);
        }

        virtual
        T pop() {
            T item = queue.top().item;
            queue.pop();
            return item;
        }

        virtual
        bool empty() {
            return queue.empty();
        }
    };
} }

#endif    /* WAITING_H */

