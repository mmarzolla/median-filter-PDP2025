/****************************************************************************
 *
 * random-image.c -- generate random image
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
 * --------------------------------------------------------------------------
 *
 * Generate a random image with user-specified width and height,
 * suitable for `median-filter-cuda`. The image is a raw streams of $X
 * \times Y \times Z$ random values of type `data_t` as defined in
 * `common.h`.
 *
 * The syntax is:
 *
 *      ./random-image [-X xsize] [-Y ysize] [-Z zsize] [outfile]
 *
 * where _xsize_, _ysize_ and _zsize_ are the image sizes (default 1);
 * indeed, the program can generate a 3D image that is stored in the
 * output file as a sequence of XY matrices. The output file is just a
 * sequence of xsize * ysize * zsize random words of type `data_t`.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include "common.h"

int main(int argc, char** argv)
{
    const char *outfile = "image.raw";
    int dims[3] = {1024, 768, 1};
    int opt;

    while ((opt = getopt(argc, argv, "X:Y:Z:")) != -1) {
        switch (opt) {
        case 'X':
            dims[DX] = atoi(optarg);
            break;
        case 'Y':
            dims[DY] = atoi(optarg);
            break;
        case 'Z':
            dims[DZ] = atoi(optarg);
            break;
        default:
            fprintf(stderr, "FATAL: unrecognized option %c\n", opt);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        outfile = argv[optind];
    }

    const size_t N_PIXELS = dims[0] * dims[1] * dims[2];
    data_t *img = (data_t*)malloc(N_PIXELS * sizeof(data_t));
    assert(img != NULL);

    FILE* fileout = fopen(outfile, "w");
    if (fileout == NULL) {
        fprintf(stderr, "FATAL: can not create output file \"%s\"\n", argv[3]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    for (size_t i=0; i<N_PIXELS; i++) {
        img[i] = (data_t)rand();
    }

    const size_t nwritten = fwrite(img, sizeof(data_t), N_PIXELS, fileout);
    (void)nwritten; /* avoid warning */
    assert(nwritten == N_PIXELS);
    fclose(fileout);
    // printf("Created image X=%d Y=%d Z=%d (%ld pixels)\n", dims[DX], dims[DY], dims[DZ], (unsigned long)N_PIXELS);

    free(img);

    return EXIT_SUCCESS;
}
