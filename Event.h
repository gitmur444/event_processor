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
        std::cout << "Processing Event with value: " << value_ << std::endl;
    }

private:
    int value_;
};
