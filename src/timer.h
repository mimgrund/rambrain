#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>

namespace membrain
{

class Timer
{

public:
    static void startTimer ( long seconds, long nanoseconds );
    static void stopTimer();

    static inline bool isRunning() {
        return running;
    }

private:
    Timer();

    static void initialiseTimer();

    static timer_t timerid;
    static struct sigevent sev;
    static struct itimerspec its;

    static bool initialised, running;

};

}

#endif // TIMER_H
