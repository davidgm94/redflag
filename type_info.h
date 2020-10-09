//
// Created by david on 8/10/20.
//

#ifndef REDFLAG_TYPE_INFO_H
#define REDFLAG_TYPE_INFO_H

#include "types.h"

struct TypeInfo
{
    size_t size;
    size_t alignment;

    template<typename T>
    static constexpr TypeInfo make()
    {
        return {sizeof(T), alignof(T)};
    }
};
#endif //REDFLAG_TYPE_INFO_H
