CUDA_LIB_PATH := /usr/local/cuda/lib64
CFLAGS+=-std=c99 -Wall -Wpedantic -fopenmp -O2 -DNDEBUG
NVCFLAGS+=-Xcompiler -fopenmp -O2 # --ptxas-options=-v
NVCC?=nvcc
OBJ=hist-bst.o omp-median-filter-2D-sparse.o median-filter.o cuda-median-filter-2D.o
# algorithms to test
ALGOS:=omp-hist-sparse-byrow

.PHONY: clean check

ALL: median-filter random-image

random-image: random-image.c common.h

median-filter: LDFLAGS+=-fopenmp -O2
median-filter: CXXFLAGS+=-fopenmp -O2 -DNDEBUG
median-filter: LDLIBS+=-lcudart -L$(CUDA_LIB_PATH)
median-filter: $(OBJ) cuda-median-filter-2D.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@ && cuobjdump -res-usage $@

median-filter.o: median-filter.c common.h

hist.o: hist-bst.c hist.h
	$(CC) $(CFLAGS) -c hist-bst.c -o hist.o

omp-median-filter-2D-sparse.o: omp-median-filter-2D-sparse.c common.h hist.h

cuda-median-filter-2D.o: cuda-median-filter-2D.cu common.h
	$(NVCC) $(NVCFLAGS) -c $< -o $@

clean:
	\rm -f *.o median-filter random-image

distclean: clean
	\rm -f *.raw test-*.txt
