/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "timer.h"

namespace rambrain
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

