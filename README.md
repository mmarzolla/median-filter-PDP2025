This repository contains the parallel median filter described in:

> Moreno Marzolla, Michele Ravaioli, Elena Loli Piccolomini, _Parallel
> Median Filter with Arbitrary Window Size and Image Depth_,
> proc. 33rd Euromicro International Conference on Parallel,
> Distributed, and Network-Based Processing (PDP), march 12--14 2025,
> Turin, Italy, pp. 9--12, IEEE Computer Society Conference Publishing
> Services (CPS) ISBN: 979-8-3315-2493-7 ISSN: 2377-5750

See [CITATION.md](CITATION.md) for the BibTeX citation block.

To compile the program, you need the `gcc` compiler and NVidia CUDA
SDK. The program has been developed under Ubuntu Linux 22.04 using
`gcc` 11.4.0 and `nvcc` V12.6.77.

The included [Makefile](Makefile) takes care of the compilation. The command

        make

produces two executables, `median-filter` and `random-image`.

`median-filter` is the actual program. Run

        ./median-filter -h

for usage instructions.

`random-image` can be used to generate a random image with
user-specified width and height, suitable for processing with
`median-filter`. The image is a raw stream of $X \times Y \times Z$
random values of the datatype `data_t` defined in `common.h`.

The syntax is:

        ./random-image [-X xsize] [-Y ysize] [-Z zsize] [outfile]

where _xsize_, _ysize_ and _zsize_ are the image sizes (default 1);
the program can generate a 3D image that is stored in the output file
as a sequence of XY matrices. The output is a sequence of (xsize *
ysize * zsize) random words of type `data_t`.

The script [test-driver.sh](test-driver.sh) produces the data used for
Figure 3 in the paper. The script [plot.gp](plot.gp) reads the data
and produces the actual figure; this script requires
[gnuplot](http://gnuplot.info/).
