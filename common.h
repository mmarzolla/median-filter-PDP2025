/****************************************************************************
 *
 * common.h -- this file is part of median filter
 *
 * Copyright (C) 2023, 2024 Moreno Marzolla, Michele Ravaioli
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
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#if BPP == 8
typedef uint8_t data_t;
#elif BPP == 16
typedef uint16_t data_t;
#else
typedef uint32_t data_t;
#endif
#define DATA_SIZE (sizeof(data_t))
#define DX 0
#define DY 1
#define DZ 2

void median_filter_2D_sparse_byrow( const data_t *in, data_t *out, const int *dims, int ndims, int radius );

#ifdef __cplusplus
extern "C" {
#endif
    void cuda_median_2D_hist_generic( const data_t *in, data_t *out, const int *dims, int ndims, int radius );
#ifdef __cplusplus
}
#endif

#endif
