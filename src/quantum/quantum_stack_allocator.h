/*
** Copyright 2018 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef QUANTUM_STACK_ALLOCATOR_H
#define QUANTUM_STACK_ALLOCATOR_H

#include <memory>
#include <assert.h>
#include <limits>
#include <type_traits>
#include <utility>
#include <quantum/quantum_spinlock.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                               Helpers
//==============================================================================
#ifndef __QUANTUM_DEFAULT_STACK_ALLOC_SIZE
#define __QUANTUM_DEFAULT_STACK_ALLOC_SIZE 1000
#endif

template <int N> //N == 0
struct index {
    using type = unsigned int;
};
template<>
struct index<1> {
    using type = unsigned short;
};
template <>
struct index<2> {
    using type = unsigned char;
};

template <int N>
struct pos_index {
    constexpr static int I = N <= std::numeric_limits<unsigned char>::max() ? 2 :
                             N <= std::numeric_limits<unsigned short>::max() ? 1 : 0;
    using type = typename index<I>::type;
};

//==============================================================================
//                            struct StackAllocator
//==============================================================================
/// @struct StackAllocator.
/// @brief Provides fast (quasi zero-time) in-place allocation for STL containers.
///        Types are allocated from a contiguous buffer on the stack. When the
///        buffer is exhausted, allocation is delegated to the heap. The default
///        buffer size is 1000.
/// @tparam T The type to allocate.
/// @tparam SIZE The size of the stack buffer.
/// @note This allocator is thread safe. For internal use only.
template <typename T, unsigned int SIZE>
struct StackAllocator
{
    //------------------------------ Typedefs ----------------------------------
    typedef StackAllocator<T, SIZE> this_type;
    typedef T                       value_type;
    typedef value_type*             pointer;
    typedef const value_type*       const_pointer;
    typedef value_type&             reference;
    typedef const value_type&       const_reference;
    typedef size_t                  size_type;
    typedef std::ptrdiff_t          difference_type;
    typedef std::false_type         propagate_on_container_move_assignment;
    typedef std::false_type         propagate_on_container_copy_assignment;
    typedef std::false_type         propagate_on_container_swap;
    typedef std::atomic_int         header_type;
    typedef std::true_type          is_always_equal;
    typedef typename pos_index<SIZE>::type index_type;
    typedef typename std::aligned_storage<sizeof(value_type),
                                          alignof(value_type)>::type buffer_type;
    template <typename U>
    struct rebind
    {
        typedef StackAllocator<U,SIZE> other;
    };
    //------------------------------- Methods ----------------------------------
    StackAllocator();
    StackAllocator(const this_type&) : StackAllocator() {}
    StackAllocator(this_type&&) : StackAllocator() {}
    template <typename U>
    StackAllocator(const StackAllocator<U,SIZE>&) : StackAllocator() {}
    StackAllocator& operator=(const this_type&) {}
    StackAllocator& operator=(this_type&&) {}
    template <typename U>
    StackAllocator& operator=(const StackAllocator<U,SIZE>&) {}
    
    pointer address(reference x) const;
    const_pointer address(const_reference x) const;
    size_type max_size() const;
    template <typename... Args >
    void construct(T* p, Args&&... args);
    void destroy(pointer p);
    pointer allocate(size_type = 1, std::allocator<void>::const_pointer = 0);
    void deallocate(pointer p, size_type = 1);
    template <typename... Args >
    pointer create(Args&&... args);
    void dispose(pointer p);
    size_t allocatedBlocks() const;
    size_t allocatedHeapBlocks() const;
    bool isFull() const;
    bool isEmpty() const;
    
private:
    pointer bufferStart();
    pointer bufferEnd();
    bool isStack(pointer p);
    index_type blockIndex(pointer p);

    //------------------------------- Members ----------------------------------
    buffer_type     _buffer[SIZE];
    index_type      _freeBlocks[SIZE];
    ssize_t         _freeBlockIndex{SIZE-1};
    size_type       _numHeapAllocatedBlocks{0};
    SpinLock        _spinlock;
};

}} //namespaces

#include <quantum/impl/quantum_stack_allocator_impl.h>

#endif //QUANTUM_STACK_ALLOCATOR_H