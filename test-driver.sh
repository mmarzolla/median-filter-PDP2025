#!/bin/bash

## Test driver for the `median-filter` program. Executes the OpenMP
## and CUDA implementations of the median filter on different image
## sizes and filter radiuses. Each measurement is repeated multiple
## times with different random input images; the mean execution time
## is printed to standard output and to suitably named output files.

## Written on 2024-10-30 by Moreno Marzolla
## Last modified 2025-01-18 by Moreno Marzolla

IMG_SIZES="1024 2048 4096"
RADIUS="16 32 64 128 256"
ALGOS="omp-hist-sparse-byrow cuda-hist-generic"
BPP="16 32"
NREP=5
EXE=./median-filter

# Test different image depths
for B in $BPP ; do
    make clean && CFLAGS=-DBPP=$B NVCFLAGS=-DBPP=$B make || exit -1
    echo
    echo "algorithm"
    echo "|                     img size"
    echo "|                     |    filter radius"
    echo "|                     |    |   bpp"
    echo "|                     |    |   |   times"
    echo "|                     |    |   |   |"
    for X in $IMG_SIZES ; do
        IMG_NAME="img-X${X}-Y${X}-B${B}.raw"
        for A in $ALGOS ; do
            OUT_FILE="test-${A}-X${X}-B${B}.txt"
            #echo ${OUT_FILE}
            rm -f ${OUT_FILE} && touch ${OUT_FILE}
            for R in $RADIUS ; do
                #echo "SIZE=${X} ALGO=${A} RADIUS=${R} BPP=${B}"
                printf "%20s %4d %2d %3d " ${A} ${X} ${B} ${R}
                TT=0
                for rep in `seq $NREP`; do
                    ./random-image -X $X -Y $X $IMG_NAME
                    EXEC_TIME="$( $EXE -X $X -Y $X -r $R -a $A -o /dev/null $IMG_NAME 2>&1 | grep "Execution time" | sed 's/Execution time\.\. //' )"
                    TT=$( echo "$TT + $EXEC_TIME" | bc )
                    echo -n " $EXEC_TIME"
                done
                echo
                echo "$R " $( echo "scale=4; $TT / $NREP" | bc ) >> ${OUT_FILE}
            done
        done
    done
done
