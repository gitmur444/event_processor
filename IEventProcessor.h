#include <thread>
#include "RingBuffer.h"


class IEventProcessor
{
public:
    using Integer = int64_t;

    struct ReservedEvent
    {
        ReservedEvent() : sequence_number_(-1), event_(nullptr) {}
        ReservedEvent(const Integer sequence_number, void *const event)
            : sequence_number_(sequence_number), event_(event) {}

        // ReservedEvent(const ReservedEvent &) = delete;
        // ReservedEvent &operator=(const ReservedEvent &) = delete;

        // ReservedEvent(ReservedEvent &&) = delete;
        // ReservedEvent &operator=(ReservedEvent &&) = delete;

        bool IsValid() const { return event_ != nullptr; }

        Integer GetSequenceNumber() const { return sequence_number_; }
        void *GetEvent() const { return event_; }

    private:
        Integer sequence_number_;
        void *event_;
    };

    struct ReservedEvents
    {
        ReservedEvents(
            const Integer sequence_number,
            void* const events,
            const size_t count,
            const size_t event_size) 
            :
            sequence_number_(sequence_number), 
            events_(events), 
            count_(count), 
            event_size_(event_size) {}

        // ReservedEvents(const ReservedEvents &) = delete;
        // ReservedEvents &operator=(const ReservedEvents &) = delete;

        // ReservedEvents(ReservedEvents &&) = delete;
        // ReservedEvents &operator=(ReservedEvents &&) = delete;

        bool IsValid() const { return events_ != nullptr; }

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args &&...args)
        {
            auto event = static_cast<TEvent *>(GetEvent(index));
            if (event)
                std::construct_at(event, std::forward<Args>(args)...);
        }

        
        void* GetEvent(const size_t index) const {
            if (index >= count_) return nullptr;
            return static_cast<char*>(events_) + index * event_size_;
        }

        Integer GetSequenceNumber() const { return sequence_number_; }
        size_t Count() const { return count_; }

    private:
        Integer sequence_number_;
        void* events_;
        size_t count_;
        size_t event_size_;
    };

    IEventProcessor()
        : ring_buffer_() {}

    ~IEventProcessor() {
    }

    // template <typename T>
    // std::pair<size_t, void *const> ReserveEvent()
    // {

    // };

    // template <class T, class... Args>
    // ReservedEvent Reserve(Args &&...args)
    // {
    //     const auto reservation = ReserveEvent<T>();
    //     if (!reservation.second)
    //         return ReservedEvent();

    //     std::construct_at(reservation.second, std::forward<Args>(args)...);
    //     return ReservedEvent(reservation.first, reservation.second);
    // }

    // Зарезервировать место для одного события
    template <typename TEvent, class... Args>
    ReservedEvent Reserve(Args&&... args) {
        size_t index;
        if (!ring_buffer_.Reserve(index)) {
            return ReservedEvent(); // Буфер полон
        }

        void* location = GetLocation(index);
        return ReservedEvent(index, location);
    }

    // // Зарезервировать место для нескольких событий
    // std::vector<ReservedEvents> ReserveRange(const size_t size); // TODO: static vector?
    //     std::vector<ReservedEvents> reserved_events;
    //     for (size_t i = 0; i < size; ++i) {
    //         ReservedEvent event = Reserve<Event>();
    //         if (event.IsValid()) {
    //             reserved_events.emplace_back(event.GetSequenceNumber(), event.GetEvent(), 1, sizeof(Event));
    //         }
    //     }
    //     return reserved_events;
    // }

        // Зарезервировать место для нескольких событий
    std::vector<ReservedEvents> ReserveRange(const size_t count) {
        size_t start_index;
        if (!ring_buffer_.ReserveRange(start_index, count)) {
            return {}; // Буфер не смог зарезервировать необходимое количество элементов
        }

        std::vector<ReservedEvents> reserved_events;
        reserved_events.emplace_back(start_index, GetLocation(start_index), count, sizeof(Event));
        return reserved_events;
    }

    void Commit(const Integer sequence_number) {}
    void Commit(const Integer sequence_number, const size_t count) {}

        // Обработка событий
    bool PopEvent(IEvent*& event) {
        return ring_buffer_.Pop(event);
    }

private:
    RingBuffer<IEvent*, 1024> ring_buffer_; 

    void* GetLocation(size_t index) {
        return reinterpret_cast<void*>(ring_buffer_.GetBufferPointer(index));
    }
};
