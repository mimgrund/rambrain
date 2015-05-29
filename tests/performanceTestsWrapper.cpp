#include "tester.h"
#include "performanceTestClasses.h"

int main ( int argc, char **argv )
{
    // Repetitions overwritten by first parameter (optional)
    unsigned int repetitions = 10;
    if ( argc > 1 ) {
        repetitions = atoi ( argv[1] );
    }

    // Eventually make changes to test parameters of allocated test classes if desired

    // Run tests
    matrixTransposeTestInstance.runTests ( repetitions );
    matrixCleverTransposeTestInstance.runTests ( repetitions );
    matrixCleverTransposeOpenMPTestInstance.runTests ( repetitions );
    matrixCleverBlockTransposeOpenMPTestInstance.runTests ( repetitions );
}
