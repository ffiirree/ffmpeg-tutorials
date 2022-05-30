#ifndef PLAYER_RING_BUFFER_H
#define PLAYER_RING_BUFFER_H

#include <mutex>
#include <cstring>

class RingBuffer {
public:
    explicit RingBuffer(size_t size)
    {
        max_size_ = size;
        buffer_ = new char[max_size_];
    }

    ~RingBuffer()
    {
        max_size_ = 0;
        delete[] buffer_;
    }

    size_t write(const char *buffer, size_t size)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if(empty_wo_lock()) reset_wo_lock();

        if (full_) return 0;
        if (!buffer) return 0;

        size_t w_size = std::min<size_t>(size, max_size_ - size_wo_lock());

        // write
        std::memcpy(buffer_ + w_idx_, buffer, std::min<size_t>(max_size_ - w_idx_, w_size));
        if (w_idx_ + w_size > max_size_) {
            std::memcpy(buffer_, buffer + (max_size_ - w_idx_), w_size - (max_size_ - w_idx_));
        }

        //
        if (max_size_ - size_wo_lock() <= size) full_ = true;
        w_idx_ = (w_idx_ + w_size) % max_size_;

        return w_size;
    }

    // Non-thread-safe
    char * write_ptr(size_t & w_size)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if(empty_wo_lock()) reset_wo_lock();

        char * ptr = buffer_ + w_idx_;
        w_size = std::min<size_t>(continuous_free_size_wo_lock(), w_size); // continuous size
        w_idx_ = (w_idx_ + w_size) % max_size_;

        return ptr;
    }

    char* read_ptr(size_t& r_size)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (empty_wo_lock()) reset_wo_lock();

        char* ptr = buffer_ + r_idx_;
        r_size = std::min<size_t>(continuous_size_wo_lock(), r_size); // continuous size
        r_idx_ = (r_idx_ + r_size) % max_size_;

        if (r_size > 0) full_ = false;
        return ptr;
    }

    // make both used and unused memory continuous
    void defrag()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if(empty_wo_lock()) {
            reset_wo_lock();
            return;
        }

        if (w_idx_ > r_idx_) {
            std::memmove(buffer_, buffer_ + r_idx_, w_idx_ - r_idx_);
        }
        else {
            char * temp = new char[w_idx_];
            std::memcpy(temp, buffer_, w_idx_);
            std::memmove(buffer_, buffer_ + r_idx_, max_size_ - r_idx_);
            std::memcpy(buffer_ + (max_size_ - r_idx_), temp, w_idx_);
            delete[] temp;
        }

        auto size = size_wo_lock();
        r_idx_ = 0;
        w_idx_ = size;
    }


    size_t read(char * ptr, size_t size)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!ptr) return 0;

        if(empty_wo_lock()) {
            reset_wo_lock();
            return 0;
        }

        size_t r_size = std::min<size_t>(size, size_wo_lock());

        // read
        std::memcpy(ptr, buffer_ + r_idx_, std::min<size_t>(max_size_ - r_idx_, r_size));
        if(r_idx_ + r_size > max_size_) {
            std::memcpy(ptr + max_size_ - r_idx_, buffer_, r_size - (max_size_ - r_idx_));
        }

        r_idx_ = (r_idx_ + r_size) % max_size_;

        if(r_size > 0) full_ = false;
        return r_size;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        reset_wo_lock();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return empty_wo_lock();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return size_wo_lock();
    }

    size_t max_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return max_size_;
    }

    size_t free_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return max_size_ - size_wo_lock();
    }

    bool continuous()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return w_idx_ > r_idx_ || r_idx_ == w_idx_ == 0;
    }

    bool continuous_free()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return r_idx_ == 0 || r_idx_ < w_idx_ || empty_wo_lock();
    }

    size_t continuous_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        return continuous_size_wo_lock();
    }

    size_t continuous_free_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(full_) return 0;

        return continuous_free_size_wo_lock();
    }

    bool full()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return full_;
    }

private:
    size_t size_wo_lock()
    {
        if (full_) {
            return max_size_;
        }
        return ((w_idx_ >= r_idx_) ? (w_idx_) : (w_idx_ + max_size_)) - r_idx_;
    }

    size_t continuous_size_wo_lock()
    {
        return ((w_idx_ > r_idx_ ? w_idx_ : max_size_)) - r_idx_;
    }

    size_t continuous_free_size_wo_lock()
    {
        return (r_idx_ > w_idx_ ? r_idx_ : max_size_) - w_idx_;
    }

    [[nodiscard]] bool empty_wo_lock() const
    {
        return !full_ && r_idx_ == w_idx_;
    }

    void reset_wo_lock()
    {
        r_idx_ = 0;
        w_idx_ = 0;
        full_ = false;

    }

    size_t r_idx_{ 0 };
    size_t w_idx_{ 0 };
    bool full_{ false };

    char * buffer_{ nullptr };
    size_t max_size_{ 0 };
    std::mutex mtx_;
};
#undef EMPTY
#endif // !PLAYER_RING_BUFFER_H