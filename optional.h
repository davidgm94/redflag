//
// Created by david on 10/14/20.
//

#pragma once

template<typename T>
struct Optional {
    T value;
    bool is_some;

    static inline Optional<T> some(T x) {
        return {x, true};
    }

    static inline Optional<T> none() {
        return {{}, false};
    }

    inline bool unwrap(T *res) {
        *res = value;
        return is_some;
    }
};
