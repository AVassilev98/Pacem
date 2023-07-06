#pragma once

#include "RingBuffer.h"
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

template <typename T, typename IdxType>
class Handle;

template <typename T, typename IdxType = uint16_t>
class Pool
{
  public:
    template <class... Args>
    [[nodiscard]] Handle<T, IdxType> create(Args &&...args)
    {
        Handle<T, IdxType> returnIndex;
        if (!m_freeList.isEmpty())
        {
            IdxType freeIndex = m_freeList.pop();
            m_data[freeIndex] = T(std::forward<Args>(args)...);
            m_generations[freeIndex]++;

            return Handle<T, IdxType>(freeIndex, m_generations[freeIndex]);
        }
        else
        {
            m_data.emplace_back(std::forward<Args>(args)...);
            m_generations.push_back(0);
            return Handle<T, IdxType>(m_data.size() - 1, 0);
        }
    }

    [[nodiscard]] T *get(Handle<T, IdxType> handle)
    {
        if (handle.m_idx >= m_data.size() || m_generations[handle.m_idx] != handle.m_gen)
        {
            return nullptr;
        }

        return &m_data[handle.m_idx];
    }

    void destroy(Handle<T, IdxType> handle)
    {
        if (handle.m_idx >= m_data.size() || m_generations[handle.m_idx] != handle.m_gen)
        {
            return;
        }

        m_data[handle.m_idx].destroy();
        m_generations[handle.m_idx]++;
        m_freeList.push(handle.m_idx);
    }

  private:
    std::vector<T> m_data;
    std::vector<IdxType> m_generations;
    RingBuffer<IdxType, IdxType> m_freeList;
};

template <typename T, typename IdxType = uint16_t>
class Handle
{
    static_assert(std::is_unsigned<IdxType>::value, "IdxType should be an unsigned integral type");
    template <typename PoolTA, typename PoolTC>
    friend class Pool;

  public:
    Handle() = default;

  private:
    Handle(IdxType idx, IdxType gen)
        : m_idx(idx)
        , m_gen(gen)
    {
    }

    // TODO: Fix this! It's dangerous in the rare case that this "invalid state"
    // happens to match up to something
    IdxType m_idx = std::numeric_limits<IdxType>::max();
    IdxType m_gen = std::numeric_limits<IdxType>::max();
};