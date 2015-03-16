#include <gtest/gtest.h>
#include "dummyManagedMemory.h"
#include "exceptions.h"

TEST(dummyManagedMemory, ThrowsExceptions)
{
    dummyManagedMemory mgr;

    EXPECT_THROW(mgr.setMemoryLimit(0), dummyObjectException);
    EXPECT_THROW(mgr.getMemoryLimit(0), dummyObjectException);
    EXPECT_THROW(mgr.getUsedMemory(), dummyObjectException);
    EXPECT_THROW(mgr.getSwappedMemory(), dummyObjectException);

    EXPECT_THROW(mgr.setUse(managedMemory::root), dummyObjectException);
    EXPECT_THROW(mgr.unsetUse(managedMemory::root), dummyObjectException);
    //! \todo what about set/unsetUse which take a memoryChunk?

    EXPECT_THROW(mgr.getNumberOfChildren(managedMemory::root), dummyObjectException);
    EXPECT_THROW(mgr.printTree(), dummyObjectException);
}
