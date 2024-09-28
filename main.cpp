#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <ranges> 
#include <algorithm>

using namespace std;

class LogDuration
{
public:
    LogDuration(std::string id)
        : id_(std::move(id))
    {
    }

    ~LogDuration()
    {
        const auto end_time = std::chrono::steady_clock::now();
        const auto dur = end_time - start_time_;
        std::cout << id_ << ": ";
        std::cout << "operation time"
                  << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
                  << " ms" << std::endl;
    }

private:
    const std::string id_;
    const std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
};

class IEvent
{
public:
    virtual ~IEvent() = default;
    virtual void Process() = 0;
};

class Event : public IEvent
{
public:
    explicit Event(const int value) : value_(value) {}
    virtual ~Event() override = default;

    Event(const Event &) = delete;
    Event &operator=(const Event &) = delete;

    Event(Event &&) = delete;
    Event &operator=(Event &&) = delete;

    virtual void Process() override
    {
        // do something with value_
    }

private:
    int value_;
};


class IEventProcessor
{
public:
    using Integer = int64_t;

    struct ReservedEvent
    {
        ReservedEvent();
        ReservedEvent(const Integer sequence_number, void *const event);

        ReservedEvent(const ReservedEvent &) = delete;
        ReservedEvent &operator=(const ReservedEvent &) = delete;

        ReservedEvent(ReservedEvent &&) = delete;
        ReservedEvent &operator=(ReservedEvent &&) = delete;

        Integer GetSequenceNumber() const { return sequence_number_; }
        void *GetEvent() const { return event_; }

    private:
        Integer sequence_number_;
        void *event_;
    };

    struct ReservedEvents
    {
        ReservedEvents(const Integer sequence_number,
                       void *const event,
                       const size_t count,
                       const size_t event_size);

        ReservedEvents(const ReservedEvents &) = delete;
        ReservedEvents &operator=(const ReservedEvents &) = delete;

        ReservedEvents(ReservedEvents &&) = delete;
        ReservedEvents &operator=(ReservedEvents &&) = delete;

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args &&...args)
        {
            auto event = static_cast<TEvent *>(GetEvent(index));
            if (event)
                std::construct_at(event, std::forward<Args>(args)...);
        }

        Integer GetSequenceNumber() const { return sequence_number_; }
        void *GetEvent(const size_t index) const;

        size_t Count() const { return count_; }

    private:
        Integer sequence_number_;
        void *events_;
        size_t count_;
        size_t event_size_;
    };

    template <typename T>
    std::pair<size_t, void *const> ReserveEvent();

    template <class T, class... Args>
    ReservedEvent Reserve(Args &&...args)
    {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second)
            return ReservedEvent();

        std::construct_at(reservation.second, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    std::vector<ReservedEvents> ReserveRange(const size_t size); // TODO: static vector?
    void Commit(const Integer sequence_number);
    void Commit(const Integer sequence_number, const size_t count);
};

int main()
{
    // queue 1 event
    IEventProcessor* event_processor = new IEventProcessor();
    auto reserved_event = event_processor->Reserve<Event>(2);
    if (!reserved_event.IsValid())
    {
        // ERROR: Reserve() failed ...
    }
    else
    {
        event_processor->Commit(reserved_event.GetSequenceNumber());
    }

    // queue multiple events
    auto reserved_events_collection = event_processor->ReserveRange(2); // It can reserve less items than
    //requested !You should always check how many events have been reserved !
    if (reserved_events_collection.empty())
    {
        // ERROR: ReserveRange() failed
    }
    else
    {
        std::ranges::for_each(
            reserved_events_collection,
            [&](IEventProcessor::ReservedEvents &reserved_events)
            {
                if (!reserved_events.IsValid())
                {
                    // ERROR: Reserve() failed
                }
                else
                {
                    for (size_t i = 0; i < reserved_events.Count(); ++i)
                        reserved_events.Emplace<Event>(i, static_cast<int>(i + 3));

                    event_processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
                }
            }
        );
    }
    delete event_processor;
}

// LogDuration ld("Total");
// cout << "Hello!!" << endl;

// Vector<uint8_t> big_vector(1e9, 0);

// LogDuration ld1("Total1");
// cout << "print " << '\n';
// {
//     LogDuration ld("vector copy");
//     // vector<uint8_t> reciever(big_vector);
//     Vector<uint8_t> reciever(std::move(big_vector));
// }
// cout << "size of big_vector is " << big_vector.size() << '\n';
