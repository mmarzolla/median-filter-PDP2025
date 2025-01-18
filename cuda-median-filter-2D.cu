/****************************************************************************
 *
 * cuda-median-filter-2D.cu -- GPU median filter using multilevel histograms
 *
 * Copyright (C) 2023, 2025 Moreno Marzolla, Michele Ravaioli
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

#include "hpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"

// threads per CUDA block
#define BLKDIM 1024
// threads per CUDA 2D block (BLKDIM_2D * BLKDIM_2D)
#define BLKDIM_2D 32
// CUDA warp size
#define WARP_SIZE 32
// number of warps per block
#define NUM_WARPS (BLKDIM / WARP_SIZE)

#define HIST_SIZE 256

/* The following kernel assumes that threads are organized into a 2D
   grid, and that there are as many threads as pixels in the extended
   image, i.e., image with ghost area.

   Image `in` must have size width * height;

   Image `out` must have size (width + 2*radius) * (height + 2*radius)

   `width` and `height` represent the size of the input image, and
   must not include the ghost area.
*/
static __global__
void init_ghost_area( const data_t *in, data_t *out, int width, int height, int radius )
{
    // These coordinates refer to the output image (with ghost area)
    const int DST_X = threadIdx.x + blockIdx.x * blockDim.x;
    const int DST_Y = threadIdx.y + blockIdx.y * blockDim.y;
    const int ext_width = width + 2*radius;
    const int ext_height = height + 2*radius;

    if (DST_X >= ext_width || DST_Y >= ext_height)
        return;

    // These coordinates refer to the input image (without ghost area)
    int SRC_X = DST_X - radius, SRC_Y = DST_Y - radius;

    if (SRC_X < 0)
        SRC_X = 0;
    else if (SRC_X >= width)
        SRC_X = width-1;

    if (SRC_Y < 0)
        SRC_Y = 0;
    else if (SRC_Y >= height)
        SRC_Y = height-1;

    out[DST_X + DST_Y*ext_width] = in[SRC_X + SRC_Y*width];
}

/**
 * All threads in the same warp cooperate to zero histogram `hist[]`
 * of length `HIST_SIZE`.
 */
static __device__
void warp_init_hist( int* hist, int lane_id )
{
#pragma unroll
    for (int i=lane_id; i<HIST_SIZE; i+=WARP_SIZE) {
        hist[i] = 0;
    }
}

static __global__
void median_filter_kernel_generic( const data_t* in,
                                   data_t* out,
                                   const int WIDTH,
                                   const int HEIGHT,
                                   const int radius )
{
    // ID of the current warp
    const int WARP_ID = threadIdx.x / WARP_SIZE;
    // ID of the thread within the warp (0 <= LAND_ID < WARP_SIZE)
    const int LANE_ID = threadIdx.x % WARP_SIZE;

    // Histograms in shared memory
    __shared__ int sh_hist[NUM_WARPS][HIST_SIZE];

    // the histogram handled by the current warp
    int *warp_hist = &sh_hist[WARP_ID][0];

    const int NPASSES = DATA_SIZE;

    __shared__ data_t mask[NUM_WARPS];
    __shared__ data_t key[NUM_WARPS];
    __shared__ int median_pos[NUM_WARPS];
    __shared__ int shift_amount[NUM_WARPS];

    // Pixel coordinates
    const int pixelX = threadIdx.x / WARP_SIZE + blockIdx.x * NUM_WARPS;
    const int pixelY = threadIdx.y + blockIdx.y * blockDim.y;

    if (pixelY >= HEIGHT || pixelX >= WIDTH)
        return;

    const int WINDOW_L = (2 * radius) + 1;
    const int WINDOW_SIZE = WINDOW_L * WINDOW_L;
    const int EXT_WIDTH = (2 * radius) + WIDTH;

    if (0 == LANE_ID) {
        shift_amount[WARP_ID] = 8*(NPASSES - 1);
        mask[WARP_ID] = 0;
        key[WARP_ID] = 0;
        median_pos[WARP_ID] = WINDOW_SIZE / 2;
    }
    __syncwarp();

#pragma unroll
    for (int pass=0; pass<NPASSES; pass++) {
        // Init histogram
        warp_init_hist(warp_hist, LANE_ID);
        __syncwarp();

        // Fill histogram; all threads in this warp contribute to
        // filling the histogram of the filter region centered at
        // (pixelX, pixelY)
        for (int i=LANE_ID; i<WINDOW_SIZE; i+=WARP_SIZE) {
            // window pixel coords
            const int win_pX = (i % WINDOW_L) + pixelX;
            const int win_pY = (i / WINDOW_L) + pixelY;

            const data_t val = in[win_pX + (win_pY * EXT_WIDTH)];
            if ((val & mask[WARP_ID]) == key[WARP_ID]) {
                const int idx = (val >> shift_amount[WARP_ID]) & 0xff;
                atomicAdd(&warp_hist[idx], 1);
            }
        }
        __syncwarp();

        // Search median from histogram (only the master of each warp)
        if (LANE_ID == 0) {
            int k=0;
            for (k=0; (k<HIST_SIZE-1) && (median_pos[WARP_ID] >= warp_hist[k]); k++)
                median_pos[WARP_ID] -= warp_hist[k];
            key[WARP_ID] |= k << shift_amount[WARP_ID];
            mask[WARP_ID] |= 0xff << shift_amount[WARP_ID];
            shift_amount[WARP_ID] -= 8;
        }
        __syncwarp();
    }

    if (0 == LANE_ID) {
        out[pixelX + (pixelY * WIDTH)] = key[WARP_ID];
    }
}

extern "C"
void cuda_median_2D_hist_generic( const data_t *in, data_t *out,
                                  const int *dims, int ndims, int radius )
{
    data_t *d_in, *d_out;

    assert(ndims == 2);
    const int width = dims[DX];
    const int height = dims[DY];

    const int EXT_WIDTH = (2 * radius) + width;
    const int EXT_HEIGHT = (2 * radius) + height;

    const size_t SIZE = width * height * DATA_SIZE;
    const size_t EXT_SIZE = EXT_WIDTH * EXT_HEIGHT * DATA_SIZE;

    cudaSafeCall( cudaMalloc((void**)&d_in, EXT_SIZE) );
    cudaSafeCall( cudaMalloc((void**)&d_out, SIZE) );

    // Initialize the ghost area
    cudaSafeCall( cudaMemcpy(d_out, in, SIZE, cudaMemcpyHostToDevice) );
    const dim3 INIT_BLOCK(BLKDIM_2D, BLKDIM_2D);
    const dim3 INIT_GRID((EXT_WIDTH + BLKDIM_2D - 1) / BLKDIM_2D,
                         (EXT_HEIGHT + BLKDIM_2D - 1) / BLKDIM_2D);
    init_ghost_area<<< INIT_GRID, INIT_BLOCK >>>(d_out, d_in, width, height, radius);
    cudaCheckError();

    // Start computation
    const dim3 GRID((width + NUM_WARPS - 1) / NUM_WARPS, height);
    median_filter_kernel_generic<<< GRID, BLKDIM >>>(d_in, d_out, width, height, radius);
    cudaCheckError();
    cudaSafeCall( cudaMemcpy(out, d_out, SIZE, cudaMemcpyDeviceToHost) );

    cudaFree(d_in);
    cudaFree(d_out);
}
