
#ifndef MESI_CONST_HPP
#define MESI_CONST_HPP

#include <string>

class MigCohState
{
public:
    static constexpr const size_t INVALID = 0;
    static constexpr const size_t SHARED = 1;
    static constexpr const size_t EXCLUSIVE = 2;
    static constexpr const size_t DIRTY = 3;
    static constexpr const size_t SHARED2 = 4;
    static constexpr const size_t MIGRATORY_CLEAN = 5;
    static constexpr const size_t MIGRATORY_DIRTY = 6;


    std::string to_string(size_t state)
    {
        switch(state)
        {
            case INVALID:
                return "INVALID";

            case SHARED:
                return "SHARED";

            case EXCLUSIVE:
                return "EXCLUSIVE";

            case DIRTY:
                return "DIRTY";

            case SHARED2:
                return "SHARED2";

            case MIGRATORY_CLEAN:
                return "MIGRATORY_CLEAN";

            case MIGRATORY_DIRTY:
                return "MIGRATORY_DIRTY";
        }

        return "unknown state"; // should never happen - to silence warning
    }
};

class MigBusFlag
{
public:
    static constexpr const size_t SHARED = 1;
    static constexpr const size_t MIGRATORY = 1 << 1;
    static constexpr const size_t TRANSMIT = 1 << 2;

    std::string to_string(size_t state)
    {
        switch(state)
        {

            case SHARED:
                return "SHARED";

            case MIGRATORY:
                return "MIGRATORY";

            case TRANSMIT:
                return "TRANSMIT";
        }

        return "unknown flag"; // should never happen - to silence warning
    }
};


#endif // MESI_CONST_HPP