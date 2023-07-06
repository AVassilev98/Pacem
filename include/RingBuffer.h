#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <type_traits>

// *** Should only be used for cases where sizeof(T) is small and MaxElems is small,
// as this will do a lot of copying and will pre-allocate its buffer ***
template <typename T, typename IndexType = uint16_t>
class RingBuffer
{
    static_assert(sizeof(T) <= sizeof(uint64_t), "Should not use this data structure for types > 8 bytes");
    static_assert(std::is_unsigned<IndexType>::value, "IndexType Must be an unsigned integral type");

  public:
    RingBuffer()
    {
        m_data = (T *)malloc(sizeof(T) * ((size_t)std::numeric_limits<IndexType>::max() + 1));
    }

    ~RingBuffer()
    {
        free(m_data);
    }

    [[nodiscard]] bool isEmpty()
    {
        return m_empty;
    }

    [[nodiscard]] bool isFull()
    {
        return m_head == m_tail && !m_empty;
    }

    void push(T elem)
    {
        assert(!isFull() && "Attempting to push onto full buffer!");
        m_data[m_tail++] = elem;
        m_empty = false;
    }

    [[nodiscard]] T pop()
    {
        assert(!isEmpty() && "Attempting to pop off empty buffer!");
        T returnVal = m_data[m_head++];
        m_empty = m_head == m_tail ? true : false;
        return returnVal;
    }

  private:
    bool m_empty = true;
    T *m_data;
    IndexType m_head = 0;
    IndexType m_tail = 0;
};