
#ifndef COUNTER_HPP
#define COUNTER_HPP

class Countdown
{
private:
    int remaining;
    bool done = true;

public:

    Countdown();
    Countdown(int target);

    void reset();
    void reset(int target);

    bool tick();
    bool is_done() { return done; }
    void finish() { done = true; } 
    int get_remaining() { return remaining; }
};






#endif // COUNTER_HPP
