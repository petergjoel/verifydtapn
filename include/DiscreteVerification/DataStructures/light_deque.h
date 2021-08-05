/* 
 * File:   light_deque.h
 * Author: Peter G. Jensen <pgj@cs.aau.dk>
 *
 * Created on 03 December 2016, 12:00
 */

#ifndef LIGHT_DEQUE_H
#define LIGHT_DEQUE_H

#include <memory>
#include <vector>
#include <cassert>

template<typename T>
class light_deque {
private:
    size_t _front = 0;
    size_t _back = 0;
    size_t _size = 0;
    std::vector<T> _data{};
public:
    explicit light_deque(size_t initial_size = 64) {
        if (initial_size == 0) initial_size = 1;
        _data.resize(initial_size);
        _size = initial_size;
    }

    void push_back(T element) {
        _data[_back] = std::move(element);
        ++_back;
        if (_back == _size) {
            _size *= 2;
            _data.resize(_size);
        }

    }

    bool empty() const {
        return _front == _back;
    }
    
    size_t size() const {
        return _back - _front;
    }

    T front() const {
        assert(!empty());
        return _data[_front];
    }
    
    T back() const {
        assert(!empty());
        return _data[_back-1];
    }

    void pop_front() {
       assert(!empty());
        ++_front;
        if (_front >= _back) {
            _front = _back = 0;
        }
    }
    
    const T* begin() const {
        return &_data[_front];
    }

    const T* end() const {
        return &_data[_back];
    }
    
    void clear() {
        _front = _back = 0;
    }
};


#endif /* LIGHT_DEQUE_H */

