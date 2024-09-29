#include <new>        // Для placement new
#include <memory>     // Для std::aligned_storage
#include <iostream>
#include <atomic>

template <typename T, size_t Size>
class RingBuffer {
public:
    RingBuffer() : write_index_(0), read_index_(0) {
        buffer_.fill(nullptr);
    }

    ~RingBuffer() {
        // Вручную вызываем деструкторы объектов в буфере перед удалением
        for (size_t i = 0; i < Size; ++i) {
            if (occupied_[i]) {
                reinterpret_cast<T*>(&buffer_[i])->~T();
            }
        }
    }

    bool Reserve(size_t& index) {
        size_t current_write_index = write_index_.fetch_add(1, std::memory_order_relaxed);
        index = current_write_index % Size;
        return current_write_index < (read_index_.load(std::memory_order_acquire) + Size);
    }

    template <class... Args>
    void Commit(size_t index, Args&&... args) {
        // Создание объекта на выделенной памяти с использованием placement new
        new (&buffer_[index]) T(std::forward<Args>(args)...);
        occupied_[index] = true;
    }

    bool Pop(T& element) {
        size_t current_read_index = read_index_.fetch_add(1, std::memory_order_relaxed);
        if (current_read_index >= write_index_.load(std::memory_order_acquire)) {
            return false; // Буфер пуст
        }

        // Копируем объект в элемент, переданный по ссылке
        element = std::move(*reinterpret_cast<T*>(&buffer_[current_read_index % Size]));
        reinterpret_cast<T*>(&buffer_[current_read_index % Size])->~T(); // Вызов деструктора
        occupied_[current_read_index % Size] = false;
        return true;
    }

private:
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    StorageType buffer_[Size];               // Выделение памяти без создания объектов
    std::atomic<bool> occupied_[Size] = {};  // Флаг наличия объекта
    std::atomic<size_t> write_index_;        // Атомарный индекс записи
    std::atomic<size_t> read_index_;         // Атомарный индекс чтения
};
