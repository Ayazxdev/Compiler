#pragma once
// High-throughput bump allocator (Arena) for AST node allocations in Luna.
// AST nodes are allocated here; freeing is O(1) (By reset the bump ptr).
//
// Interface is deliberately compatible with PMRArena (PMRArena.hpp) so the
// parser only needs to accept an Arena* and can transparently switch backing.
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

namespace luna::memory {

class Arena {
public:
    static constexpr std::size_t kDefaultChunk = 64 * 1024; // 64 KiB

    explicit Arena(std::size_t chunk_bytes = kDefaultChunk)
        : chunk_(chunk_bytes) {
        add_chunk();
    }

    // Non-copyable, movable
    Arena(const Arena&)            = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&)                 = default;
    Arena& operator=(Arena&&)      = default;

    ~Arena() = default; // unique_ptr chunks clean up automatically

    // Allocate & construct
    template<typename T, typename... Args>
    [[nodiscard]] T* alloc(Args&&... args) {
        void* p = allocate(sizeof(T), alignof(T));
        return ::new (p) T{std::forward<Args>(args)...};
    }

    // Raw allocation (aligned)
    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t align = alignof(std::max_align_t)) {
        assert(align > 0 && (align & (align - 1)) == 0 && "alignment must be power of 2");
        // Align current pointer
        std::size_t offset  = (ptr_ + align - 1) & ~(align - 1);
        if (offset + bytes > end_) {
            // Current chunk is full — get a new one
            if (bytes > chunk_) {
                // Oversized: allocate a dedicated chunk exactly this big
                add_chunk(bytes);
            } else {
                add_chunk();
            }
            offset = ptr_; // new chunk is already aligned to max_align_t
        }
        ptr_ = offset + bytes;
        return reinterpret_cast<void*>(static_cast<char*>(current_) + (offset - base_));
    }

    // Reset — O(1), keeps all memory for re-use
    void reset() noexcept {
        // Keep the first chunk, discard all extras
        chunks_.resize(1);
        ptr_     = base_;
        end_     = base_ + chunk_;
        current_ = chunks_[0].get();
    }

    // Stats
    [[nodiscard]] std::size_t num_chunks() const noexcept { return chunks_.size(); }

private:
    std::size_t chunk_; // default chunk size in bytes

    // Raw memory chunks
    std::vector<std::unique_ptr<char[]>> chunks_;
    void*       current_{nullptr}; // pointer to start of current raw chunk
    std::size_t base_{0};          // byte offset of chunk start within chunk
    std::size_t ptr_{0};           // current bump pointer (byte offset)
    std::size_t end_{0};           // one past the end of current chunk

    void add_chunk(std::size_t size = 0) {
        std::size_t sz = (size > chunk_) ? size : chunk_;
        auto mem = std::make_unique<char[]>(sz);
        current_ = mem.get();
        chunks_.push_back(std::move(mem));
        base_ = 0;
        ptr_  = 0;
        end_  = sz;
    }
};

} // namespace luna::memory
