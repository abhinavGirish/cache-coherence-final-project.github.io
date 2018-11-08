
#ifndef MESI_CONST_HPP
#define MESI_CONST_HPP

#include <string>

class MESICoherenceState
{
public:
    static constexpr const size_t STATE_INVALID = 0;
    static constexpr const size_t STATE_SHARED = 1;
    static constexpr const size_t STATE_EXCLUSIVE = 2;
    static constexpr const size_t STATE_MODIFIED = 3;

    std::string to_string(size_t state)
    {
        switch(state)
        {
            case  STATE_INVALID:
                return "STATE_INVALID";

            case STATE_SHARED:
                return "STATE_SHARED";

            case STATE_EXCLUSIVE:
                return "STATE_EXCLUSIVE";

            case STATE_MODIFIED:
                return "STATE_MODIFIED";
        }

        return "unknown state"; // should never happen - to silence warning
    }
};

class MESIBusFlag
{
public:
    static constexpr const size_t BUS_FLAG_SHARED = 1;
    static constexpr const size_t BUS_FLAG_TRANSMIT = 1 << 1;

    std::string to_string(size_t state)
    {
        switch(state)
        {

            case BUS_FLAG_SHARED:
                return "BUS_FLAG_SHARED";

            case BUS_FLAG_TRANSMIT:
                return "BUS_FLAG_TRANSMIT";
        }

        return "unknown flag"; // should never happen - to silence warning
    }
};


#endif // MESI_CONST_HPP