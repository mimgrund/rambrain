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

#ifndef INITIALISATION_H
#define INITIALISATION_H

#include "configreader.h"
#include "managedSwap.h"
#include "managedMemory.h"
#include "common.h"

namespace rambrain
{

namespace rambrainglobals
{

/**
 * @brief Main class for handling configuration throughout the library and for the user.
 */
class rambrainConfig
{
public:
    /**
     * @brief Construct a new config handling class
     *
     * Reads in the config and sets up the system.
     * @note Do not do this yourself, rather use the supplied instance rambrainglobals::config
     */
    rambrainConfig();
    /**
     * @brief Destructs the throught the config initiated swap and manager
     */
    ~rambrainConfig();

    /**
     * @brief Reinitialises the system
     * @param reread If the / a config file should be rereaad before resetting up the main classes
     */
    void reinit ( bool reread = true );


    /**
     * @brief Simple setter
     */
    inline void setCustomConfigPath ( const string &path ) {
        config.setCustomConfigPath ( path );
    }

    /**
     * @brief Simple getter
     */
    inline const configuration &getConfig() const {
        return config.getConfig();
    }

    /**
     * @brief Simple setter
     */
    void resizeMemory ( global_bytesize memory );
    /**
     * @brief Simple setter
     * @warning Currently only extending may be supported, ergo this means to reinitiate the main classes and to throw away the old ones deleting all existing managed pointers
     */
    void resizeSwap ( global_bytesize memory );

private:
    /**
     * Initialise the system
     */
    void init();
    /**
     * Clean up the system and delete instances
     */
    void clean();

    configReader config;
    managedSwap *swap;
    managedMemory *manager;

};

/// You will find the object in managedMemory.cpp as we have to define it in some 'used' file in the linker sense.
extern rambrainConfig config;

}

}

#endif // INITIALISATION_H

