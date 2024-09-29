#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <ranges> 
#include <algorithm>
#include <thread>

#include "Event.h"
#include "IEventProcessor.h"

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

// void ReaderFunction(IEventProcessor* processor) {
//     while (true) {
//         // Извлекаем событие и обрабатываем его
//         IEvent* event = nullptr;
//         if (!processor->ProcessEvents()) {  // Условие остановки (буфер пуст)
//             break;
//         }
//     }
// }

// Функция, выполняющая роль потока-читателя
void ReaderFunction(IEventProcessor* processor) {
    IEvent* event = nullptr;
    while (true) {
        if (!processor->PopEvent(event)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Ожидание, если буфер пуст
            continue;
        }
        if (event) {
            event->Process();
        }
    }
}

// void WriterFunction(IEventProcessor* processor, int writer_id, int events_per_writer) {
//     for (int i = 0; i < events_per_writer; ++i) {
//         auto reserved_event = processor->Reserve<Event>(writer_id * events_per_writer + i);
//         if (reserved_event.IsValid()) {
//             processor->Commit(reserved_event.GetSequenceNumber());
//         }
//     }
// }

// Функция, выполняющая роль потока-писателя
void WriterFunction(IEventProcessor* processor, int writer_id, int events_per_writer) {
    for (int j = 0; j < events_per_writer; ++j) {
        auto reserved_events = processor->ReserveRange(2); // Пример использования ReserveRange
        if (reserved_events.empty()) {
            std::cerr << "ERROR: ReserveRange() failed" << std::endl;
            continue;
        }
        std::ranges::for_each(
            reserved_events,
            [&](IEventProcessor::ReservedEvents &reserved_events) {
                if (!reserved_events.IsValid()) {
                    std::cerr << "ERROR: Reserve() failed" << std::endl;
                } else {
                    for (size_t k = 0; k < reserved_events.Count(); ++k) {
                        reserved_events.Emplace<Event>(k, writer_id * events_per_writer + j + k);
                    }
                    processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
                }
            }
        );
    }
}

int main()
{
    constexpr size_t buffer_size = 1024;
    constexpr int num_writers = 16;
    constexpr int events_per_writer = 10'000; // Пример: 10 тысяч событий на каждый поток

    IEventProcessor event_processor(buffer_size);

    // Запуск потока-читателя
    std::thread reader(ReaderFunction, &event_processor);

    // Запуск потоков-писателей
    std::vector<std::thread> writers;
    for (int i = 0; i < num_writers; ++i) {
        writers.emplace_back(WriterFunction, &event_processor, i, events_per_writer);
    }

    // Ожидание завершения работы всех писателей
    for (auto& writer : writers) {
        writer.join();
    }

    // Ожидание завершения работы потока-читателя
    reader.join();

    return 0;
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



    // // queue 1 event
    // IEventProcessor* event_processor = new IEventProcessor();
    // auto reserved_event = event_processor->Reserve<Event>(2);
    // if (!reserved_event.IsValid())
    // {
    //     // ERROR: Reserve() failed ...
    // }
    // else
    // {
    //     event_processor->Commit(reserved_event.GetSequenceNumber());
    // }

    // // queue multiple events
    // auto reserved_events_collection = event_processor->ReserveRange(2); // It can reserve less items than
    // //requested !You should always check how many events have been reserved !
    // if (reserved_events_collection.empty())
    // {
    //     // ERROR: ReserveRange() failed
    // }
    // else
    // {
    //     std::ranges::for_each(
    //         reserved_events_collection,
    //         [&](IEventProcessor::ReservedEvents &reserved_events)
    //         {
    //             if (!reserved_events.IsValid())
    //             {
    //                 // ERROR: Reserve() failed
    //                 std::cerr << "ERROR: Reserve() failed" << std::endl;
    //             }
    //             else
    //             {
    //                 for (size_t i = 0; i < reserved_events.Count(); ++i)
    //                     reserved_events.Emplace<Event>(i, static_cast<int>(i + 3));

    //                 event_processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
    //             }
    //         }
    //     );
    // }
    // delete event_processor;