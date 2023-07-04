#pragma once

#include "RingBuffer.h"
#include <cstdint>
#include <type_traits>
#include <vector>

template <typename T, typename IdxType>
class Handle;

template <typename T, typename IdxType = uint16_t>
class Pool
{
  public:
    [[nodiscard]] Handle<T, IdxType> create(const typename T::State &&createState);
    void destroy(Handle<T, IdxType> handle);

    [[nodiscard]] T *get(Handle<T, IdxType> handle);

  private:
    std::vector<T> m_data;
    std::vector<IdxType> m_generations;
    RingBuffer<IdxType, IdxType> m_freeList;
};

template <typename T, typename IdxType = uint16_t>
class Handle
{
    static_assert(std::is_unsigned<IdxType>::value(), "IdxType should be an unsigned integral type");

    friend class Pool<T, IdxType>;
    IdxType m_idx;
    IdxType m_gen;
};
