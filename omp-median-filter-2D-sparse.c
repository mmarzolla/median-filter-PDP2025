/****************************************************************************
 *
 * omp-median-filter-2D-sparse.c -- 2D image denoising using sparse histograms
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <omp.h>
#include "common.h"
#include "hist.h"

#define REPLICATE

static int IDX(int i, int j, int height, int width)
{
#ifdef REPLICATE
    i = (i<0 ? 0 : (i>=height ? height-1 : i));
    j = (j<0 ? 0 : (j>=width ? width-1 : j));
#else
    i = (i + height) % height;
    j = (j + width) % width;
#endif
    return (i*width + j);
}

/**
 * Compute the histogram of the values located within a window of
 * radius `radius` centered at (i,j).
 */
static void fill_histogram(Hist * restrict hist,
                           const data_t * restrict in,
                           int i, int j, int radius,
                           int width, int height)
{
    for (int di=-radius; di<=radius; di++) {
        for (int dj=-radius; dj<=radius; dj++) {
            const data_t val = in[IDX(i+di, j+dj, height, width)];
            hist_insert(hist, val, 1);
        }
    }
}

/**
 * Given an histogram for a window of radius `radius`` centered at (i,
 * j), update the histogram by shifting the window one position to the
 * right.
 */
static void shift_histogram(Hist * restrict hist,
                            const data_t * restrict in,
                            int i, int j, int radius,
                            int width, int height)
{
    for (int di=-radius; di<=radius; di++) {
        const data_t val_left = in[IDX(i+di, j-radius, height, width)];
        hist_delete(hist, val_left, 1);
        const data_t val_right = in[IDX(i+di, j+radius+1, height, width)];
        hist_insert(hist, val_right, 1);
    }
}

/**
 ** Histogram-based median filter. The histogram is NOT computed from
 ** scratch for each pixel; instead, when the window is shifted, the
 ** old histogram is updated.
 **
 ** Execution time: O(width * height * R * log(R) / P)
 **
 ** Additional memory: O(P * R)
 **
 ** where P is the number of OpenMP threads and B is the color depth in pixels.
 **/
void median_filter_2D_sparse_byrow( const data_t * restrict in,
                                    data_t * restrict out,
                                    const int *dims, int ndims, int radius )
{
    assert(ndims == 2);
    const int width = dims[DX];
    const int height = dims[DY];

#pragma omp parallel default(none) shared(width, height, in, out, radius)
    {
        Hist *hist = hist_create();
        assert(hist != NULL);
#pragma omp for
        for (int i=0; i<height; i++) {
            hist_clear(hist);
            fill_histogram(hist, in, i, 0, radius, width, height);
            // Note: the loop stops before the last column, so that we
            // do not perform a shift_histogram() out-of-bound
            int j;
            for (j=0; j<width-1; j++) {
                out[IDX(i, j, height, width)] = hist_median(hist);
                shift_histogram(hist, in, i, j, radius, width, height);
            }
            // Handle the last element of the current row
            out[IDX(i, j, height, width)] = hist_median(hist);
        }
        hist_destroy(hist);
    }
}
