
#include "countdown.hpp"


Countdown::Countdown()
{
    reset();
}

Countdown::Countdown(int target)
{
    reset(target);
}

void Countdown::reset()
{
    remaining = 0;
    done = true;
}

void Countdown::reset(int target)
{
    remaining = target;
    if(remaining <= 0)
    {
        done = true;
    }
    else
    {
        done = false;
    }
}


bool Countdown::tick()
{
    if(done)
    {
        return done;
    }

    remaining--;

    if(remaining <= 0)
    {
        done = true;
    }

    return done;
}
