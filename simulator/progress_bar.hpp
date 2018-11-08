
#ifndef PROGRESS_BAR_HPP
#define PROGRESS_BAR_HPP


#include <iostream>

class ProgressBar
{
private:
    double curr;
    double max;
    double step_size;
    const int BARLEN = 50;
    bool done = false;
public:
    ProgressBar(float max) : max(max) 
    {
        step_size = max/BARLEN;
    }


    void tick()
    {
        if(done)
        {
            return;
        }
        curr += 1;
        draw();
    }

    void set(double current)
    {
        if(current > max)
        {
            curr = max;
        }
        else
        {
            curr = current;
        }
        draw();
    }

    void draw()
    {
        int percent = int ((curr/max) * 100.0);
        if(percent >= 100.0)
        {
            percent = 100.0;
            done = true;
        }

        int ticks = (int)(curr / step_size);
        
        std::cout << "\r[";
        for(int i=0; i<ticks; i++)
        {
            std::cout << "#";
        }
        for(int i=0; i<BARLEN-ticks;i++)
        {
            std::cout << " ";
        }
        std::cout << "] " << percent << "% " << curr << "/" << max << std::flush;

        if(done)
        {
            std::cout << std::endl;
        }
    }

     
};



#endif // PROGRESS_BAR_HPP