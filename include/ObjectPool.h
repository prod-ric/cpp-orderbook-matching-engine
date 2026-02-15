#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <new>       // for placement new

namespace engine {

// A simple object pool that pre-allocates a block of memory
// and hands out slots without calling new/delete during trading
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t capacity)
        : capacity_(capacity)
        , size_(0)
    {
        // Allocate raw memory for all objects at once — one big block
        // This is the ONLY heap allocation. Everything else is just pointer math.
        storage_ = static_cast<T*>(std::malloc(capacity * sizeof(T)));
        if (!storage_) {
            throw std::bad_alloc();
        }

        // Build the free list — every slot is available
        freeSlots_.reserve(capacity);
        for (size_t i = capacity; i > 0; --i) {
            freeSlots_.push_back(i - 1);  // push in reverse so slot 0 is used first
        }
    }

    ~ObjectPool() {
        // We don't call destructors here because we manage that in release()
        // Just free the raw memory block
        std::free(storage_);
    }

    // No copying — there's only one pool
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Acquire a slot and construct an object in it
    // Args are forwarded to T's constructor
    template <typename... Args>
    T* acquire(Args&&... args) {
        if (freeSlots_.empty()) {
            throw std::runtime_error("Object pool exhausted");
        }

        // Pop a free slot index
        size_t index = freeSlots_.back();
        freeSlots_.pop_back();
        size_++;

        // Construct the object in the pre-allocated memory
        // This is "placement new" — it doesn't allocate, just constructs
        T* slot = &storage_[index];
        new (slot) T(std::forward<Args>(args)...);

        return slot;
    }

    // Release an object — call its destructor and return the slot
    void release(T* ptr) {
        if (!ptr) return;

        // Calculate which slot index this pointer corresponds to
        size_t index = static_cast<size_t>(ptr - storage_);

        // Call the destructor (cleanup the object)
        ptr->~T();

        // Return the slot to the free list
        freeSlots_.push_back(index);
        size_--;
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    size_t available() const { return freeSlots_.size(); }

private:
    T* storage_;                    // one contiguous block of memory
    std::vector<size_t> freeSlots_; // indices of available slots
    size_t capacity_;
    size_t size_;
};

} // namespace engine
