#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>

namespace rambrain
{

/**
 * @brief Provides an interface to a timer which periodically sends SIGUSR1 signals
 * @note There can always be only one timer in place since it is regulated via static methods
 */
class Timer
{

public:

    /**
     * @brief Start the timer
     * @param seconds How many seconds until it fires a signal
     * @param nanoseconds How many nanoseconds on top of the seconds until it fires
     */
    static void startTimer ( long seconds, long nanoseconds );
    /**
     * @brief Stop the timer
     */
    static void stopTimer();

    /**
     * @brief Simple getter
     */
    static inline bool isRunning() {
        return running;
    }

private:
    /**
     * @brief Create a new timer
     * @note Unused
     */
    Timer();

    /**
     * @brief Set up the timer
     */
    static void initialiseTimer();

    static timer_t timerid;
    static struct sigevent sev;
    static struct itimerspec its;

    static bool initialised, running;

};

}

#endif // TIMER_H
