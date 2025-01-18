/****************************************************************************
 *
 * median-filter.c -- Image denoising using median filter
 *
 * Copyright 2018--2024 Moreno Marzolla, Michele Ravaioli
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#if _XOPEN_SOURCE < 600
#define _XOPEN_SOURCE 600
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <omp.h>
#include "common.h"

double hpc_gettime( void )
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts );
    return ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

typedef void (*median_filter_algo_t)( const data_t *in,
                                      data_t *out,
                                      const int *dims, /* array of 2 or 3 elements */
                                      int ndims, /* either 2 or 3 */
                                      int radius );

struct {
    const char *name;
    const char *description;
    median_filter_algo_t fun;
} median_filter_algos[] = { {"omp-hist-sparse-byrow", "Optimized sparse histogram-based median (OpenMP)", median_filter_2D_sparse_byrow},
                            {"cuda-hist-generic", "Histogram-based median, works with any data type  (CUDA)", cuda_median_2D_hist_generic},
                            {NULL, NULL, NULL}
};

void print_usage( const char *exe_name )
{
    fprintf(stderr,
            "Usage: %s [-h] [-a algo] [-X dimx] [-Y dimy] [-Z dimz] [-r radius] [-o outfile] infile\n\n"
            "-h\t\tprint help\n"
            "-a algo\t\tset algorithm (see below)\n"
            "-X dimx\tX dimension (width)\n"
            "-Y dimy\tY dimension (height)\n"
            "-Z dimz\tZ dimension (depth)\n"
            "-r radius\tfilter radius\n"
            "-o outfile\toutput file name\n"
            "infile\t\tinput file name\n\n"
            "Valid algorithm names:\n\n", exe_name);
    for (int i=0; median_filter_algos[i].name; i++) {
        fprintf(stderr, "%-20s\t%s%s\n",
                median_filter_algos[i].name,
                median_filter_algos[i].description,
                i == 0 ? " (default)" : "");
    }
    fprintf(stderr, "\n");
}

int main( int argc, char *argv[] )
{
    int radius = 41;
    const char *infile = NULL, *outfile = "out.raw";
    int i, opt;
    int dims[3] = {-1, -1, -1};
    int ndims;

    const char *algo_name = median_filter_algos[0].name;
    median_filter_algo_t algo_fun = median_filter_algos[0].fun;

    while ((opt = getopt(argc, argv, "ha:X:Y:Z:r:o:")) != -1) {
        switch(opt) {
        case 'a':
            i = 0;
            while (median_filter_algos[i].name && strcmp(optarg, median_filter_algos[i].name)) {
                i++;
            }
            if (median_filter_algos[i].name) {
                algo_name = median_filter_algos[i].name;
                algo_fun = median_filter_algos[i].fun;
            } else {
                fprintf(stderr, "\nFATAL: invalid algorithm %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'X': /* width */
            dims[DX] = atoi(optarg);
            break;
        case 'Y': /* height */
            dims[DY] = atoi(optarg);
            break;
        case 'Z': /* depth */
            dims[DZ] = atoi(optarg);
            break;
        case 'r':
            radius = atoi(optarg);
            break;
        case 'o':
            outfile = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            fprintf(stderr, "\nFATAL: Unrecognized option %s\n\n", argv[optind]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (dims[0] < 0 || dims[1] < 0) {
        fprintf(stderr, "\nFATAL: You must specify width and height\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    ndims = (dims[2] < 0 ? 2 : 3);

    if (optind >= argc) {
        fprintf(stderr, "\nFATAL: No input file given\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    infile = argv[optind];

    FILE* filein = fopen(infile, "r");
    if (filein == NULL) {
        fprintf(stderr, "\nFATAL: can not open input file \"%s\"\n", infile);
        return EXIT_FAILURE;
    }

    const size_t N_PIXELS = (ndims == 2 ?
                             dims[0] * dims[1] :
                             dims[0] * dims[1] * dims[2]);
    const size_t IMG_SIZE = N_PIXELS * DATA_SIZE;

    data_t *img = (data_t*)malloc(IMG_SIZE); assert(img != NULL);
    data_t *out = (data_t*)malloc(IMG_SIZE); assert(out != NULL);
    const size_t nread = fread(img, DATA_SIZE, N_PIXELS, filein);
    (void)nread; // dummy access to suppress warning (unused variable `nread`)
    assert(nread == N_PIXELS);
    fclose(filein);

    fprintf(stderr,
            "Algorithm....... %s\n"
            "Input........... %s\n"
            "X dim........... %d\n"
            "Y dim........... %d\n"
            "Z dim........... %d\n"
            "Data size (B)... %d\n"
            "Dimensions...... %d\n"
            "Radius.......... %d\n"
            "Output.......... %s\n",
            algo_name,
            infile,
            dims[DX],
            dims[DY],
            dims[DZ],
            (int)DATA_SIZE,
            ndims,
            radius,
            outfile);
    const double tstart = hpc_gettime();
    algo_fun(img, out, dims, ndims, radius);
    const double elapsed = hpc_gettime() - tstart;
    fprintf(stderr, "\nExecution time.. %f\n\n", elapsed);

    FILE* fileout = fopen(outfile, "w");
    if (fileout == NULL) {
        fprintf(stderr, "FATAL: can not create output file \"%s\"\n", outfile);
        return EXIT_FAILURE;
    }

    const size_t nwritten = fwrite(out, DATA_SIZE, N_PIXELS, fileout);
    (void)nwritten; // dummy write to suppress warning (unused variable `nwritten`)
    assert(nwritten == N_PIXELS);
    fclose(fileout);

    free(img);
    free(out);

    return EXIT_SUCCESS;
}
