#ifndef CONSISTENCY_CHECKER_INTERFACE_HPP
#define CONSISTENCY_CHECKER_INTERFACE_HPP


class ConsistencyCheckerInterface
{
private:


public:

    virtual ~ConsistencyCheckerInterface() {}
    virtual void check_states() = 0;
    virtual void check_requests() = 0;
    virtual void check() = 0;
};




#endif // CONSISTENCY_CHECKER_INTERFACE_HPP
