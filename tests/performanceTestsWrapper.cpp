#include "tester.h"
#include "performanceTestClasses.h"

int main ( int argc, char **argv )
{
    // Repetitions overwritten by first parameter (optional)
    unsigned int repetitions = 10;
    if ( argc > 1 ) {
        repetitions = atoi ( argv[1] );
    }

    // Allocate test classes and eventually make changes to test parameters if desired
    matrixTransposeTest matrixTranspose;

    // Run tests
    matrixTranspose.runTests ( repetitions );
}
