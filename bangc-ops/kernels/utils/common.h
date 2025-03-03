/*************************************************************************
 * Copyright (C) [2022] by Cambricon, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *************************************************************************/

// public functions are stored in this file
#ifndef KERNELS_UTILS__COMMON_H_
#define KERNELS_UTILS__COMMON_H_

#include <type_traits>
#include "float.h"
#include "kernels/kernel.h"

#define HALFMAX 65504

template <typename T>
__mlu_func__ inline T __mluop_min(T a, T b) {
  return a < b ? a : b;
}

template <typename T>
__mlu_func__ inline T __mluop_max(T a, T b) {
  return a > b ? a : b;
}

/******************************************************************************************
 * MLUOPS FUNC: computeRecip
 * param 'nram_dst' is the nram destination address, which supports half or
 * float data type. param 'nram_src' is the nram source address, which has the
 * same data type as nram_dst. param 'nram_addition' is the nram addition
 * address. Pass NULL if the data type of nram_src is float, otherwise the space
 * size is at least twice as much as nram_src. param 'is_high_precision' is the
 * precision flag. param 'deal_num' is the num of input data. remarks:
 *   1. nram_dst and nram_src can be homologous operand.
 *   2. On MLU2XX, input must be in the range [0.00391, 2e6] for float and
 * [0.00391, 65504] for half. Please refer to bangC Developer Guide for detailed
 * information.
 ******************************************************************************************/
template <typename T>
static __mlu_func__ void computeRecip(T *nram_dst, T *nram_src,
                                      void *nram_addition,
                                      const bool is_high_precision,
                                      const uint32_t deal_num) {
  if (sizeof(T) == sizeof(float)) {
#if __BANG_ARCH__ >= 300
    __bang_recip((float *)nram_dst, (float *)nram_src, deal_num);
#else
    __bang_active_reciphp((float *)nram_dst, (float *)nram_src, deal_num);
#endif
  } else if (sizeof(T) == sizeof(half)) {
#if __BANG_ARCH__ >= 300
    __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
    __bang_recip((float *)nram_addition, (float *)nram_addition, deal_num);
    __bang_float2half_rn((half *)nram_dst, (float *)nram_addition, deal_num);
#else
    if (is_high_precision) {
      __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
      __bang_active_reciphp((float *)nram_addition, (float *)nram_addition,
                            deal_num);
      __bang_float2half_rd((half *)nram_dst, (float *)nram_addition, deal_num);
    } else {
      __bang_active_reciphp((half *)nram_dst, (half *)nram_src, deal_num);
    }
#endif
  } else {
    return;
  }
}

/******************************************************************************
 * MLUOPS FUNC: computeExp
 * param 'nram_dst' is the nram destination address, which supports half or
 * float data type. param 'nram_src' is the nram source address, which has the
 * same data type as nram_dst. param 'nram_addition' is the nram addition
 * address. Pass NULL if the data type of nram_src is float, otherwise the space
 * size is at least twice as much as nram_src. param 'is_high_precision' is the
 * precision flag. param 'deal_num' is the num of input data. remarks: nram_dst
 * and nram_src can be homologous operand.
 ******************************************************************************/
template <typename T>
static __mlu_func__ void computeExp(T *nram_dst, T *nram_src,
                                    void *nram_addition,
                                    const int is_high_precision,
                                    const int deal_num) {
  if (sizeof(T) == sizeof(float)) {
#if __BANG_ARCH__ >= 300
    int x2d = 0x3fb8aa3b;
    float log2e = *(float *)&x2d;
    __bang_mul_scalar((float *)nram_dst, (float *)nram_src, (float)log2e,
                      deal_num);
    __bang_pow2((float *)nram_dst, (float *)nram_dst, deal_num);
#else
    __bang_active_exphp((float *)nram_dst, (float *)nram_src, deal_num);
#endif
  } else if (sizeof(T) == sizeof(half)) {
#if __BANG_ARCH__ >= 300
    int x2d = 0x3fb8aa3b;
    float log2e = *(float *)&x2d;
    __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
    __bang_mul_scalar((float *)nram_addition, (float *)nram_addition,
                      (float)log2e, deal_num);
    __bang_pow2((float *)nram_addition, (float *)nram_addition, deal_num);
    __bang_float2half_rn((half *)nram_dst, (float *)nram_addition, deal_num);
#else
    if (is_high_precision) {
      __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
      __bang_active_exphp((float *)nram_addition, (float *)nram_addition,
                          deal_num);
      __bang_float2half_rd((half *)nram_dst, (float *)nram_addition, deal_num);
    } else {
      __bang_active_exphp((half *)nram_dst, (half *)nram_src, deal_num);
    }
#endif
  } else {
    return;
  }
}

/******************************************************************************
 * MLUOPS FUNC: computeSigmoid
 * param 'nram_dst' is the nram destination address, which supports half or
 * float data type. param 'nram_src' is the nram source address, which has the
 * same data type as nram_dst. param 'nram_addition' is the nram addition
 * address. Pass NULL if the data type of nram_src is float, otherwise the space
 * size is at least twice as much as nram_src. param 'is_high_precision' is the
 * precision flag. param 'deal_num' is the num of input data. remarks: nram_dst
 * and nram_src can be homologous operand.
 ******************************************************************************************/
template <typename T>
static __mlu_func__ void computeSigmoid(T *nram_dst, T *nram_src,
                                        void *nram_addition,
                                        const int is_high_precision,
                                        const int deal_num) {
  if (sizeof(T) == sizeof(float)) {
#if __BANG_ARCH__ >= 300
    __bang_mul_scalar((float *)nram_src, (float *)nram_src, (float)-1.0,
                      deal_num);
    computeExp((float *)nram_src, (float *)nram_src, NULL, 0, deal_num);
    __bang_add_scalar((float *)nram_src, (float *)nram_src, (float)1.0,
                      deal_num);
    computeRecip((float *)nram_dst, (float *)nram_src, NULL, 0, deal_num);
#else
    __bang_active_sigmoid((float *)nram_dst, (float *)nram_src, deal_num);
#endif
  } else if (sizeof(T) == sizeof(half)) {
#if __BANG_ARCH__ >= 300
    __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
    __bang_mul_scalar((float *)nram_addition, (float *)nram_addition,
                      (float)-1.0, deal_num);
    computeExp((float *)nram_addition, (float *)nram_addition, NULL, 0,
               deal_num);
    __bang_add_scalar((float *)nram_addition, (float *)nram_addition,
                      (float)1.0, deal_num);
    computeRecip((float *)nram_dst, (float *)nram_addition, NULL, 0, deal_num);
    __bang_float2half_rn((half *)nram_dst, (float *)nram_dst, deal_num);
#else
    if (is_high_precision) {
      __bang_half2float((float *)nram_addition, (half *)nram_src, deal_num);
      __bang_active_sigmoid((float *)nram_addition, (float *)nram_addition,
                            deal_num);
      __bang_float2half_rd((half *)nram_dst, (float *)nram_addition, deal_num);
    } else {
      __bang_active_sigmoid((half *)nram_dst, (half *)nram_src, deal_num);
    }
#endif
  } else {
    return;
  }
}

/*****************************************************************************
 * MLUOPS FUNC: __int322float
 * param 'dst' is the destination pointer in NRAM, same memory space as src
 * required in NRAM
 * param 'dst_addition' is the addition workspace of dst, requiring the same
 * amount of space as dst in NRAM
 * param 'src' is the source pointer in NRAM
 * param 'src_addition' is the addition workspace of src, requiring only 128B
 * space in NRAM
 * param 'src_count' is the src element count
 * Notes:
 *   the sapces pointed by dst and src can not overlap
 *   src_count*sizeof(float) should be divisible by 128
 *   src input must be in range of [-2^23, 2^23-1] for MLU270 and MLU290
 *****************************************************************************/
__mlu_func__ void __int322float(float *dst, float *dst_addition, int32_t *src,
                                float *src_addition, int32_t src_count) {
#if __BANG_ARCH__ >= 300
  __bang_int322float((float *)dst, (int32_t *)src, src_count, 0);
#else
  // get sign bit
  int32_t seg_elem_count = 32;  // 128/sizeof(float) = 32
  int32_t float_size = 4;       // sizeof(float) = 4
  int32_t align_128 = 128;
  float move_23bit = 8388608.0;
  // 0x80000000 = 1,000000000,0000000000000000000000000000
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x80000000);
  __bang_cycle_band((char *)dst_addition, (char *)src, (char *)src_addition,
                    src_count * float_size, align_128);
  // get 1 or 0 from sign bit
  // judge is Odd
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x00000001);
  __bang_cycle_bor((char *)dst_addition, (char *)dst_addition,
                   (char *)src_addition, src_count * float_size, align_128);
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x80000001);
  __bang_cycle_eq(dst_addition, dst_addition, src_addition, src_count,
                  seg_elem_count);
  // minus xor, positive num invariant
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0xffffffff);
  __bang_cycle_mul(dst, dst_addition, src_addition, src_count, seg_elem_count);
  __bang_bxor((char *)dst, (char *)src, (char *)dst, src_count * float_size);
  // convert int32 to float32
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x7fffff);
  __bang_cycle_band((char *)dst, (char *)dst, (char *)src_addition,
                    src_count * float_size, align_128);
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x4b000000);
  __bang_cycle_bor((char *)dst, (char *)dst, (char *)src_addition,
                   src_count * float_size, align_128);
  __bang_sub_scalar(dst, dst, move_23bit, src_count);
  // add one
  __bang_add(dst, dst, dst_addition, src_count);
  // set sign for float32
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0xffffffff);
  __bang_cycle_mul(dst_addition, dst_addition, src_addition, src_count,
                   seg_elem_count);

  // fix on MLU300
  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x00000001);
  __bang_cycle_add(dst_addition, dst_addition, src_addition, src_count,
                   seg_elem_count);
  // end fix

  __bang_write_value((unsigned *)src_addition, seg_elem_count,
                     (unsigned)0x80000000);
  __bang_cycle_band((char *)dst_addition, (char *)dst_addition,
                    (char *)src_addition, src_count * float_size, align_128);
  __bang_bor((char *)dst, (char *)dst, (char *)dst_addition,
             src_count * float_size);
#endif
}

/*****************************************************************************
 * MLUOPS FUNC: __float2int32
 * param 'dst' is the destination pointer in NRAM, same memory space as src
 * required in NRAM
 * param 'dst_addition' is the addition workspace of dst, requiring the same
 * amount of space as dst in NRAM
 * param 'src' is the source pointer in NRAM
 * param 'src_addition' is the addition workspace of src, requiring only 128B
 * space in NRAM
 * param 'src_count' is the src element count
 * Notes:
 *   the sapces pointed by dst and src can not overlap
 *   src_count*sizeof(float) should be divisible by 128
 *   src input must be in range of [-2^23, 2^23-1] for MLU270 and MLU290
 *****************************************************************************/
__mlu_func__ void __float2int32(int32_t *dst, float *dst_addition, float *src,
                                float *src_addition, int32_t src_count) {
#if __BANG_ARCH__ >= 322
  __bang_float2int32_tz((int32_t *)dst, (float *)src, src_count, 0);
#else
  // sign ===> src_addition
  // dst=-1.0 : when src[i] is a negative number
  // dst=+1.0 : when src[i] is a positive number
  int32_t floatDchar = sizeof(float) / sizeof(char);
  __bang_active_sign((float *)dst, src, src_count);
  // dst_addition = abs(src)
  __bang_mul(dst_addition, src, (float *)dst, src_count);
  // if dst_addition < 1.0, then src_addition + 1. to fix add error
  __bang_write_value((float *)src_addition, NFU_ALIGN_SIZE / sizeof(float),
                     1.0f);
  __bang_cycle_lt(dst_addition, dst_addition, (float *)src_addition, src_count,
                  NFU_ALIGN_SIZE / sizeof(float));
  __bang_add_tz((float *)dst, (float *)dst, (float *)dst_addition, src_count);
  __bang_write_value((unsigned *)src_addition, NFU_ALIGN_SIZE / sizeof(float),
                     0xbf800000);
  // set negative flag -1.0 = 0xbf80000
  __bang_cycle_eq(
      (float *)dst, (float *)dst, (float *)src_addition, src_count,
      NFU_ALIGN_SIZE / sizeof(float));  // to mask all src in [x < -1.0]
  __bang_active_abs(dst_addition, src, src_count);
  __bang_write_value((float *)src_addition, NFU_ALIGN_SIZE / sizeof(float),
                     8388608.0f);
  // mask shift move 23
  __bang_cycle_add_tz(
      dst_addition, dst_addition, src_addition, src_count,
      NFU_ALIGN_SIZE / sizeof(float));  // right shift move 23bit
  // dst=1.0, when src < -1.0
  // dst=0.0, when src >=-1.0
  __bang_sub(dst_addition, dst_addition, (float *)dst, src_count);
  // to fix max value
  __bang_mul_scalar((float *)dst, (float *)dst, 16777215.0, src_count);
  __bang_bxor((char *)dst_addition, (char *)dst_addition, (char *)dst,
              src_count * floatDchar);
  // get log 23bit
  __bang_write_value((unsigned *)src_addition, NFU_ALIGN_SIZE / sizeof(float),
                     (unsigned)0x007fffff);
  // mask low 23bit is 1
  __bang_cycle_band((char *)dst_addition, (char *)dst_addition,
                    (char *)src_addition, src_count * floatDchar,
                    NFU_ALIGN_SIZE / sizeof(char));

  __bang_write_value(src_addition, NFU_ALIGN_SIZE / sizeof(float), 0x3f800000);
  __bang_cycle_and((float *)dst, (float *)dst, src_addition, src_count,
                   NFU_ALIGN_SIZE / sizeof(float));
  // src or dst_addition
  __bang_bor((char *)dst_addition, (char *)dst, (char *)dst_addition,
             src_count * floatDchar);
  __bang_mul_scalar((float *)dst, (float *)dst, -2.0, src_count);
  __bang_bor((char *)dst, (char *)dst, (char *)dst_addition,
             src_count * floatDchar);
#endif
}

#endif  // KERNELS_UTILS__COMMON_H_
