#!/bin/bash

TestRepetitions=10


RunMatrixTranspose1=0
#MatrixTransposeSize1=(1000 2000 3000 4000 5000)
#MatrixTransposeMemory=(10 10 10 10 10)
declare -a MatrixTransposeSize1
declare -a MatrixTransposeMemory1
for i in `seq 0 20`;
do
  MatrixTransposeSize1[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixTransposeSize1[$i]=${MatrixTransposeSize1[$i]%.*}
  MatrixTransposeMemory1[$i]=10
done


RunMatrixTranspose2=0
declare -a MatrixTransposeSize2
declare -a MatrixTransposeMemory2
for i in `seq 0 20`;
do
  MatrixTransposeSize2[$i]=2000
  MatrixTransposeMemory2[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixTransposeMemory2[$i]=${MatrixTransposeMemory2[$i]%.*}
done
#!/bin/bash

TestRepetitions=10


RunMatrixCleverTranspose1=1
#MatrixCleverTransposeSize1=(1000 2000 3000 4000 5000)
#MatrixCleverTransposeMemory=(10 10 10 10 10)
declare -a MatrixCleverTransposeSize1
declare -a MatrixCleverTransposeMemory1
for i in `seq 0 20`;
do
  MatrixCleverTransposeSize1[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixCleverTransposeSize1[$i]=${MatrixTransposeSize1[$i]%.*}
  MatrixCleverTransposeMemory1[$i]=100
done


RunMatrixCleverTranspose2=1
declare -a MatrixCleverTransposeSize2
declare -a MatrixCleverTransposeMemory2
for i in `seq 0 20`;
do
  MatrixCleverTransposeSize2[$i]=2000
  MatrixCleverTransposeMemory2[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixCleverTransposeMemory2[$i]=${MatrixTransposeMemory2[$i]%.*}
done

RunMatrixCleverTransposeOpenMP1=1
#MatrixCleverTransposeOpenMPSize1=(1000 2000 3000 4000 5000)
#MatrixCleverTransposeOpenMPMemory=(10 10 10 10 10)
declare -a MatrixCleverTransposeOpenMPSize1
declare -a MatrixCleverTransposeOpenMPMemory1
for i in `seq 0 20`;
do
  MatrixCleverTransposeOpenMPSize1[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixCleverTransposeOpenMPSize1[$i]=${MatrixTransposeSize1[$i]%.*}
  MatrixCleverTransposeOpenMPMemory1[$i]=100
done


RunMatrixCleverTransposeOpenMP2=1
declare -a MatrixCleverTransposeOpenMPSize2
declare -a MatrixCleverTransposeOpenMPMemory2
for i in `seq 0 20`;
do
  MatrixCleverTransposeOpenMPSize2[$i]=2000
  MatrixCleverTransposeOpenMPMemory2[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 20.0 + 1.0)}")
  MatrixCleverTransposeOpenMPMemory2[$i]=${MatrixTransposeMemory2[$i]%.*}
done