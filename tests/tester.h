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

#ifndef TESTER_H
#define TESTER_H

#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

/**
 * @brief A basic class to be used by tests. Provides helper methods and functionality e.g. time measurements
 */
class tester
{

public:
    /**
     * @brief tester Create a new tester
     * @param name Name of the current test
     */
    tester ( const char *name = "" );
    /**
     * @brief Cleans up and destroys the tester
     */
    ~tester();

    /**
     * @brief Add a new parameter to the list of parameters
     * @param param The parameter value
     */
    void addParameter ( char *param );
    /**
     * @brief Saves the current timestamp
     */
    void addTimeMeasurement();
    /**
     * @brief Add a duration to the timing list
     */
    void addExternalTime ( std::chrono::duration<double> );
    /**
     * @brief Simple setter
     */
    void addComment ( const char *comment );
    /**
     * @brief Set a new seed for random number generation
     * @param seed The seed
     */
    void setSeed ( unsigned int seed = time ( NULL ) );

    /**
     * @brief Get a random number (integer)
     * @param max The upper limit for generated random numbers
     * @return The number
     */
    int random ( int max ) const;
    /**
     * @brief Get a random number (long integer)
     * @param max The upper limit for generated random numbers
     * @return The number
     */
    uint64_t random ( uint64_t max ) const;
    /**
     * @brief Get a random number (double)
     * @param max The upper limit for generated random numbers
     * @return The number
     */
    double random ( double max = 1.0 ) const;

    /**
     * @brief Starts a new cycle of time measurements
     *
     * Different cycles are independent. In each cycle durations are measured w.r.t. the last measurement
     */
    void startNewTimeCycle();
    /**
     * @brief Starts a new cycle of random numbers
     *
     * Allows to set a new seed afterwards without loosing the seed information before
     */
    void startNewRNGCycle();

    /**
     * @brief Write all collected information to file
     *
     * The file name consists of the test name followed by a hash seperated list of the parameter values
     */
    void writeToFile();

    /**
     * @brief Take the current cycle of time measurements and calculate all durations in betwen
     * @return A list of durations with size of time measurements minus one
     */
    std::vector<int64_t> getDurationsForCurrentCycle() const;

private:
    const char *name;
    std::vector<char *> parameters;
    std::vector<std::vector<std::chrono::high_resolution_clock::time_point> > timeMeasures;
    std::string comment;
    std::vector<unsigned int> seeds;
    std::vector<bool> seeded;

};

#ifdef CAN_IGNORE_WARNINGS
#define STRINGIFY(a) #a
#define IGNORE_WARNING(warning) _Pragma(STRINGIFY(GCC diagnostic ignored #warning))
#define IGNORE_TEST_WARNINGS IGNORE_WARNING(-Wunknown-pragmas); \
                             IGNORE_WARNING(-Wunused-variable); \
                             IGNORE_WARNING(-Wdeprecated-declarations); \
                             IGNORE_WARNING(-Wsign-compare); \
                             IGNORE_WARNING(-Wunused-but-set-variable)
#define RESTORE_WARNING _Pragma("GCC diagnostic pop")
#define RESTORE_WARNINGS RESTORE_WARNING; RESTORE_WARNING; RESTORE_WARNING; RESTORE_WARNING; RESTORE_WARNING;
#else
#define IGNORE_WARNING(warning)
#define IGNORE_TEST_WARNINGS
#define RESTORE_WARNING
#define RESTORE_WARNINGS
#endif

#endif // TESTER_H

