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

#include <gtest/gtest.h>
#include "managedMemory.h"
#include "dummyManagedMemory.h"
#include "cyclicManagedMemory.h"

using namespace rambrain;
/**
* @test Checks presence of a default manager at runtime without doing anything before
*/
TEST ( managedMemory, Unit_DefaultManagerPresent )
{
    EXPECT_TRUE ( managedMemory::defaultManager != NULL );
}

/**
* @test Checks whether we fall back to default when deleting our own memory manager instance
*/
TEST ( managedMemory, Unit_DefaultManagerPresentAfterAllocation )
{
    managedMemory *mm = new dummyManagedMemory();
    delete mm;

    EXPECT_TRUE ( managedMemory::defaultManager != NULL );
}

/**
* @test Checks basic manager usage stats for consistency under resize
*/
TEST ( managedMemory, Unit_BaseMemoryUsage )
{
    unsigned int mem = 1000u;
    managedMemory::defaultManager->setMemoryLimit ( mem );

    EXPECT_EQ ( mem, managedMemory::defaultManager->getMemoryLimit() );
    EXPECT_EQ ( 0u, managedMemory::defaultManager->getUsedMemory() );
    EXPECT_EQ ( 0u, managedMemory::defaultManager->getSwappedMemory() );
}

/**
* @test prints out a version info (git version + diff)
*/
TEST ( managedMemory, DISABLED_VersionInfo )
{
    managedMemory::versionInfo();
}

/**
 * @test Check if multiple managers are allocated and deleted (in the right order), that we can fallback to the original manager
 */
TEST ( managedMemory, Unit_FallbackToDefaultManagerChain )
{
    managedMemory *fallbackManager = managedMemory::defaultManager;

    dummyManagedMemory *man1 = new dummyManagedMemory();
    managedDummySwap *swap2 = new managedDummySwap ( 10 );
    cyclicManagedMemory *man2 = new cyclicManagedMemory ( swap2, 10 );
    dummyManagedMemory *man3 = new dummyManagedMemory();

    ASSERT_EQ ( man3, managedMemory::defaultManager );

    delete man3;
    ASSERT_EQ ( man2, managedMemory::defaultManager );

    delete man2;
    delete swap2;
    ASSERT_EQ ( man1, managedMemory::defaultManager );

    delete man1;
    ASSERT_EQ ( fallbackManager, managedMemory::defaultManager );
}

/**
 * @test Check what happens to the default manager if we delete a chain of managers in the wrong order
 */
TEST ( managedMemory, Unit_FallbackToDefaultManagerChainWrongOrder )
{
    managedMemory *fallbackManager = managedMemory::defaultManager;

    dummyManagedMemory *man1 = new dummyManagedMemory();
    managedDummySwap *swap2 = new managedDummySwap ( 10 );
    cyclicManagedMemory *man2 = new cyclicManagedMemory ( swap2, 10 );
    dummyManagedMemory *man3 = new dummyManagedMemory();

    ASSERT_EQ ( man3, managedMemory::defaultManager );

    delete man3;
    ASSERT_EQ ( man2, managedMemory::defaultManager );

    delete man1;
    ASSERT_EQ ( man2, managedMemory::defaultManager );

    delete man2;
    delete swap2;
    ASSERT_EQ ( man1, managedMemory::defaultManager );

    // The state of the system is now broken, since man1 does not exist anymore; Can never fall back to fallbackManager
}
