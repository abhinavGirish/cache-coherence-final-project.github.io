
#ifndef EVENT_HPP
#define EVENT_HPP


class Event
{
public:
    virtual void event() = 0;
    virtual ~Event() {};
};

#endif // EVENT_HPP