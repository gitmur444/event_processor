#include <iostream>
#include <atomic>
#include <bit>
#include <concepts>
#include <memory>
#include <cstring>
#include <new>
#include <ranges>
#include <type_traits>
#include <utility>

// Концепт для ограничения типов, которые можно хранить в буфере
template <typename T>
concept Bufferable = std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && std::is_destructible_v<T>;

template <Bufferable T, size_t Size>
class RingBuffer {
public:
    RingBuffer() : writeIndex(0), readIndex(0), isFull(false) {
        std::memset(buffer, 0, sizeof(buffer)); // Инициализация памяти
    }

    ~RingBuffer() {
        Clear(); // Уничтожение всех объектов в буфере при разрушении буфера
    }

    // Зарезервировать место для одного элемента
    bool Reserve(size_t& index) {
        if (isFull.load(std::memory_order_acquire)) {
            return false; // Если буфер полон, возвращаем false
        }

        size_t currentWriteIndex = writeIndex.load(std::memory_order_relaxed);
        size_t nextWriteIndex = (currentWriteIndex + 1) % Size;

        // Проверка переполнения (writeIndex не должен догонять readIndex)
        if (nextWriteIndex == readIndex.load(std::memory_order_acquire)) {
            isFull.store(true, std::memory_order_release);
            return false;
        }

        index = currentWriteIndex;
        return true;
    }

    // Зарезервировать диапазон элементов в буфере
    bool ReserveRange(size_t& startIndex, size_t count) {
        if (isFull.load(std::memory_order_acquire)) {
            return false; // Если буфер полон, резервирование невозможно
        }

        size_t currentWriteIndex = writeIndex.load(std::memory_order_relaxed);
        size_t nextWriteIndex = (currentWriteIndex + count) % Size;

        // Проверка переполнения при резервировании диапазона
        if ((nextWriteIndex + 1) % Size == readIndex.load(std::memory_order_acquire)) {
            isFull.store(true, std::memory_order_release);
            return false;
        }

        startIndex = currentWriteIndex;
        return true;
    }

    // Добавить элемент в буфер в зарезервированное место
    template <class... Args>
    void Commit(size_t index, Args&&... args) {
        if (index >= Size) {
            throw std::out_of_range("Index out of range");
        }

        std::construct_at(GetBufferPointer(index), std::forward<Args>(args)...); // Создаем объект на зарезервированном месте
        occupied[index].store(true, std::memory_order_release);

        size_t nextWriteIndex = (index + 1) % Size;
        writeIndex.store(nextWriteIndex, std::memory_order_release);

        if (nextWriteIndex == readIndex.load(std::memory_order_acquire)) {
            isFull.store(true, std::memory_order_release); // Если после записи writeIndex догнал readIndex, буфер полон
        }
    }

    // Извлечь элемент из буфера
    bool Pop(T& element) {
        if (IsEmpty()) {
            return false; // Если буфер пуст, извлечение невозможно
        }

        size_t currentReadIndex = readIndex.load(std::memory_order_relaxed);
        if (occupied[currentReadIndex].load(std::memory_order_acquire)) {
            T* objectPtr = GetBufferPointer(currentReadIndex);
            element = std::move(*objectPtr);
            objectPtr->~T(); // Явный вызов деструктора для корректного удаления объекта
            occupied[currentReadIndex].store(false, std::memory_order_release);

            readIndex.store((currentReadIndex + 1) % Size, std::memory_order_release);
            isFull.store(false, std::memory_order_release); // Если что-то извлечено, то буфер больше не полон
            return true;
        }
        return false;
    }

    // Проверка, пуст ли буфер
    bool IsEmpty() const {
        return (!isFull.load(std::memory_order_acquire) && 
               (writeIndex.load(std::memory_order_acquire) == readIndex.load(std::memory_order_acquire)));
    }

    // Метод для получения указателя на объект в буфере
    T* GetBufferPointer(size_t index) {
        if (index >= Size) {
            throw std::out_of_range("Index out of range");
        }
        return std::bit_cast<T*>(&buffer[index]); // Возвращаем указатель на объект типа T
    }

    // Очистка буфера и удаление всех объектов
    void Clear() {
        for (size_t i = 0; i < Size; ++i) {
            if (occupied[i].load(std::memory_order_acquire)) {
                T* objectPtr = GetBufferPointer(i);
                objectPtr->~T(); // Уничтожаем объект вручную
                occupied[i].store(false, std::memory_order_release);
            }
        }
        writeIndex.store(0, std::memory_order_release);
        readIndex.store(0, std::memory_order_release);
        isFull.store(false, std::memory_order_release); // После очистки буфер больше не полон
    }

private:
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    StorageType buffer[Size];                            // Массив памяти с использованием aligned_storage
    std::atomic<size_t> writeIndex;                      // Атомарный индекс записи
    std::atomic<size_t> readIndex;                       // Атомарный индекс чтения
    std::atomic<bool> occupied[Size] = {};               // Флаги занятости ячеек
    std::atomic<bool> isFull;                            // Флаг переполнения буфера
};
