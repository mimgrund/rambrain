#include "timer.h"

namespace membrain
{

timer_t Timer::timerid;
struct sigevent Timer::sev;
struct itimerspec Timer::its;

bool Timer::initialised = false;
bool Timer::running = false;

Timer::Timer()
{
}

void Timer::startTimer ( long seconds, long nanoseconds )
{
    if ( !initialised ) {
        initialiseTimer();
    }

    if ( running ) {
        stopTimer();
    }

    its.it_value.tv_sec = seconds;
    its.it_value.tv_nsec = nanoseconds;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    timer_settime ( timerid, 0, &its, NULL );

    running = true;
}

void Timer::stopTimer()
{
    if ( running ) {
        memset ( ( void * ) &its, 0, sizeof ( its ) );
        timer_settime ( timerid, 0, &its, NULL );

        running = false;
    }
}

void Timer::initialiseTimer()
{
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &timerid;
    timer_create ( CLOCK_REALTIME, &sev, &timerid );

    initialised = true;
}

}
