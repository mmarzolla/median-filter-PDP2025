This archive contains the parallel median filter programs described in
the paper:

> Moreno Marzolla, Michele Ravaioli, Elena Loli Piccolomini, "Parallel
> Median Filter with Arbitrary Window Size and Image Depth"

to appear in the proceedings of the 33rd Euromicro International
Conference on Parallel, Distributed and Network-Based Processing ([PDP
2025](https://pdp2025.org/)), 12--14 March 2025, Turin (Italy).

To compile the program, you need the `gcc` compiler and NVidia CUDA
SDK. The program has been compiled under Ubuntu Linux 22.04, using
`gcc` 11.4.0 and `nvcc` V12.6.77.

The included `Makefile` takes care of the compilation. The command

        make

produces two executables, `median-filter` and `random-image`.

`median-filter` is the actual program. Run

        ./median-filter -h

for usage instructions.

`random-image` can be used to generate a random image with
user-specified width and height, suitable for processing with
`median-filter`. The image is a raw streams of $X \times Y \times
Z$ random values of the datatype `data_t` defined in `common.h`.

The syntax is:

        ./random-image [-X xsize] [-Y ysize] [-Z zsize] [outfile]

where _xsize_, _ysize_ and _zsize_ are the image sizes (default 1);
indeed, the program can generate a 3D image that is stored in the
output file as a sequence of XY matrices. The output file is just a
sequence of xsize * ysize * zsize random words of type `data_t`.

The script `test-driver.sh` produces the raw data used for Figure 3 in
the paper.  The script `plot.gp` reads the data and produces the
actual figure; this script requires [gnuplot](http://gnuplot.info/).
