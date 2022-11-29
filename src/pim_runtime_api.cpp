/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is a property of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system, or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 * (Use of the Software is restricted to non-commercial, personal or academic, research purpose only)
 * 
 * Modifiaction History: 
 * Date             Version         Written By                  Description
 * 2022-08-09       0.1             Lukas Sommer                Initial Creation
 *                                  (Codeplay Software Limited)
 * 
 */ 

#include "pim_runtime_api.h"

#include "half.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

using half_t = half_float::half;

namespace pim {
namespace mock {

namespace {
enum ERROR_CODES : int {
  SUCCESS = 0,
  ALLOC_ERROR = -1,
  COPY_ERROR = -2,
  OPERATION_ERROR = -3
};
} // anonymous namespace

int PimInitialize(PimRuntimeType, PimPrecision) {
  // Currently nothing to do during initalization.
  return SUCCESS;
}

int PimDeinitialize() {
  // Currently nothing to do during de-initialization.
  return SUCCESS;
}

int PimSetDevice(uint32_t device_id) {
  (void)device_id;
  // Currently nothing to do to switch devices.
  return SUCCESS;
}

size_t PrecisionSize(const PimBo *bo) {
  assert(bo != nullptr && "Invalid buffer");
  switch (bo->precision) {
  case PIM_FP16:
    return sizeof(half_t);
  case PIM_INT8:
  default:
    return 1ul;
  }
}

namespace {

int AllocateMemory(PimBo *bo, void *user_ptr) {
  assert(bo != nullptr && "Buffer not valid");
  size_t typeSize = PrecisionSize(bo);
  auto bshape = bo->bshape;
  size_t size = bshape.n * bshape.c * bshape.h * bshape.w * typeSize;
  bo->size = size;
  if (user_ptr) {
    bo->data = user_ptr;
    bo->use_user_ptr = true;
    return SUCCESS;
  }
  auto *data = malloc(size);
  if (!data) {
    return ALLOC_ERROR;
  }
  bo->data = data;
  bo->use_user_ptr = false;
  return SUCCESS;
}

} // anonymous namespace

PimBo *PimCreateBo(int w, int h, int c, int n, PimPrecision precision,
                   PimMemType mem_type, void *user_ptr) {
  PimBShape shape{static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                  static_cast<uint32_t>(c), static_cast<uint32_t>(n), false};

  auto bo =
      std::unique_ptr<PimBo>(new PimBo{mem_type, shape, shape, precision});
  if (!bo) {
    return nullptr;
  }
  auto failed = AllocateMemory(bo.get(), user_ptr);
  if (failed) {
    return nullptr;
  }
  return bo.release();
}

PimBo *PimCreateBo(PimDesc *pim_desc, PimMemType mem_type, PimMemFlag,
                   void *user_ptr) {
  // We currently do not use the additional information from PimDesc or
  // PimMemFlag for alignment etc.
  auto bo = std::unique_ptr<PimBo>(new PimBo{
      mem_type, pim_desc->bshape, pim_desc->bshape_r, pim_desc->precision});
  if (!bo) {
    return nullptr;
  }
  auto failed = AllocateMemory(bo.get(), user_ptr);
  if (failed) {
    return nullptr;
  }
  return bo.release();
}

int PimDestroyBo(PimBo *pim_bo) {
  if (!pim_bo->use_user_ptr && pim_bo->data) {
    free(pim_bo->data);
  }
  delete pim_bo;
  return SUCCESS;
}

PimDesc *PimCreateDesc(int n, int c, int h, int w, PimPrecision precision,
                       PimOpType op_type) {
  PimBShape shape{static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                  static_cast<uint32_t>(c), static_cast<uint32_t>(n), false};
  auto desc =
      std::unique_ptr<PimDesc>(new PimDesc{shape, shape, precision, op_type});
  if (!desc) {
    return nullptr;
  }
  return desc.release();
}

int PimDestroyDesc(PimDesc *pim_desc) {
  delete pim_desc;
  return SUCCESS;
}

int PimAllocMemory(void **ptr, size_t size, PimMemType) {
  *ptr = malloc(size);
  if (!*ptr) {
    return ALLOC_ERROR;
  }
  return SUCCESS;
}

int PimAllocMemory(PimBo *pim_bo) {
  if (!pim_bo->use_user_ptr && pim_bo->data) {
    // Free the old memory before overriding it with a new allocation.
    free(pim_bo->data);
  }
  return AllocateMemory(pim_bo, nullptr);
}

int PimFreeMemory(void *ptr, PimMemType) {
  free(ptr);
  return SUCCESS;
}

int PimFreeMemory(PimBo *pim_bo) {
  if (!pim_bo->use_user_ptr && pim_bo->data) {
    free(pim_bo->data);
    pim_bo->data = nullptr;
    pim_bo->size = 0;
  }
  return SUCCESS;
}

int PimCopyMemory(void *dst, void *src, size_t size, PimMemCpyType) {
  if (!dst || !src || !size) {
    return COPY_ERROR;
  }
  std::memcpy(dst, src, size);
  return SUCCESS;
}

int PimCopyMemory(PimBo *dst, PimBo *src, PimMemCpyType) {
  if (!dst->data || !src->data || !src->size || src->size != dst->size) {
    return COPY_ERROR;
  }
  // FIXME(Lukas): We could use the PimMemType of the buffer objects to verify
  // that PimMemCpyType is actually applicable to the two buffers.
  std::memcpy(dst->data, src->data, src->size);
  return SUCCESS;
}

int PimCopyMemoryRect(const PimCopy3D *params) {
  if (!params->src_ptr && !params->src_bo) {
    // One of srcPtr and srcBo must be given
    return COPY_ERROR;
  }
  if (!params->dst_ptr && !params->dst_bo) {
    // One of dstPtr and dstBo must be given
    return COPY_ERROR;
  }

  const void *src = nullptr;
  size_t sPitch = 0;
  size_t sHeight = 0;
  if (params->src_bo != nullptr) {
    auto *bo = params->src_bo;
    src = bo->data;
    sPitch = bo->bshape.w * PrecisionSize(bo);
    sHeight = bo->bshape.h;
  } else {
    src = params->src_ptr;
    sPitch = params->src_pitch;
    sHeight = params->src_height;
  }

  if (!src || !sPitch || !sHeight) {
    return COPY_ERROR;
  }
  // Calculate offset source pointer
  src = static_cast<const void *>(static_cast<const char *>(src) +
                                  (params->src_z * sHeight + params->src_y) *
                                      sPitch +
                                  params->src_x_in_bytes);

  void *dst = nullptr;
  size_t dPitch = 0;
  size_t dHeight = 0;
  if (params->dst_bo != nullptr) {
    auto *bo = params->dst_bo;
    dst = bo->data;
    dPitch = bo->bshape.w * PrecisionSize(bo);
    dHeight = bo->bshape.h;
  } else {
    dst = params->dst_ptr;
    dPitch = params->dst_pitch;
    dHeight = params->dst_height;
  }

  if (!dst || !dPitch || !dHeight) {
    return COPY_ERROR;
  }
  // Calculate offset destination pointer
  dst = static_cast<void *>(static_cast<char *>(dst) +
                            (params->dst_z * dHeight + params->dst_y) * dPitch +
                            params->dst_x_in_bytes);

  // Host emulation does not have a rectangular copy, so perform the rectangular
  // copy as a series of row-wise copies.
  for (size_t d = 0; d < params->depth; ++d) {
    for (size_t h = 0; h < params->height; ++h) {
      size_t sOffset = (d * sHeight + h) * sPitch;
      const void *tmpSrc =
          static_cast<const void *>(static_cast<const char *>(src) + sOffset);
      size_t dOffset = (d * dHeight + h) * dPitch;
      void *tmpDst = static_cast<void *>(static_cast<char *>(dst) + dOffset);
      std::memcpy(tmpDst, tmpSrc, params->width_in_bytes);
    }
  }
  return SUCCESS;
}

size_t NumElements(const PimBo *bo) {
  assert(bo != nullptr && "Invalid buffer");
  auto bshape = bo->bshape;
  size_t numElements = bshape.n * bshape.c * bshape.h * bshape.w;
  assert(numElements * PrecisionSize(bo) == bo->size);
  return numElements;
}

int PimExecuteAdd(PimBo *output, PimBo *input1, PimBo *input2, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  if (!output->data || !input1->data || !input2->data ||
      input1->size != input2->size || input1->size != output->size) {
    return OPERATION_ERROR;
  }
  half_t *halfOut = static_cast<half_t *>(output->data);
  half_t *halfIn1 = static_cast<half_t *>(input1->data);
  half_t *halfIn2 = static_cast<half_t *>(input2->data);
  for (size_t i = 0; i < NumElements(output); ++i) {
    halfOut[i] = halfIn1[i] + halfIn2[i];
  }
  return SUCCESS;
}

int PimExecuteAdd(PimBo *output, void *scalar, PimBo *vector, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  if (!output->data || !vector->data || !scalar ||
      vector->size != output->size) {
    return OPERATION_ERROR;
  }
  half_t *halfOut = static_cast<half_t *>(output->data);
  half_t *halfVec = static_cast<half_t *>(vector->data);
  half_t halfScalar = *static_cast<half_t *>(scalar);
  for (size_t i = 0; i < NumElements(output); ++i) {
    halfOut[i] = halfVec[i] + halfScalar;
  }
  return SUCCESS;
}

int PimExecuteMul(PimBo *output, PimBo *input1, PimBo *input2, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  if (!output->data || !input1->data || !input2->data ||
      input1->size != input2->size || input1->size != output->size) {
    return OPERATION_ERROR;
  }
  half_t *halfOut = static_cast<half_t *>(output->data);
  half_t *halfIn1 = static_cast<half_t *>(input1->data);
  half_t *halfIn2 = static_cast<half_t *>(input2->data);
  for (size_t i = 0; i < NumElements(output); ++i) {
    halfOut[i] = halfIn1[i] * halfIn2[i];
  }
  return SUCCESS;
}

int PimExecuteMul(PimBo *output, void *scalar, PimBo *vector, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  if (!output->data || !vector->data || !scalar ||
      vector->size != output->size) {
    return OPERATION_ERROR;
  }
  half_t *halfOut = static_cast<half_t *>(output->data);
  half_t *halfVec = static_cast<half_t *>(vector->data);
  half_t halfScalar = *static_cast<half_t *>(scalar);
  for (size_t i = 0; i < NumElements(output); ++i) {
    halfOut[i] = halfVec[i] * halfScalar;
  }
  return SUCCESS;
}

int PimExecuteRelu(PimBo *output, PimBo *pim_data, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  if (!output->data || !pim_data->data || pim_data->size != output->size) {
    return OPERATION_ERROR;
  }
  half_t *halfOut = static_cast<half_t *>(output->data);
  half_t *halfData = static_cast<half_t *>(pim_data->data);
  for (size_t i = 0; i < NumElements(output); ++i) {
    halfOut[i] = (half_float::signbit(halfData[i])) ? 0.0 : halfData[i];
  }
  return SUCCESS;
}

int PimExecuteGemv(PimBo *output, PimBo *operand0, PimBo *operand1, void *,
                   bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.
  PimBo *op2 = (operand1) ? operand1 : output;
  if (!op2->data || !operand0->data) {
    return OPERATION_ERROR;
  }

  // The PIM SDK uses the following layout for the operands and result of GEMV,
  // each given as (w, h, c, n):
  // Operand0 (Vector): (X, 1, C, N)
  // Operand1 (Matrix): (X, Y, C, 1)
  // Output   (Result): (Y, 1, C, N)
  if (op2->bshape.n != 1 || output->bshape.n != operand0->bshape.n ||
      op2->bshape.c != operand0->bshape.c ||
      output->bshape.c != operand0->bshape.c ||
      op2->bshape.w != operand0->bshape.w ||
      output->bshape.w != op2->bshape.h ||
      output->bshape.h != operand0->bshape.h) {
    return OPERATION_ERROR;
  }

  if (operand0->bshape.h != 1 || output->bshape.h != 1) {
    // Just GEMV, not GEMM
    return OPERATION_ERROR;
  }

  half_t *out = static_cast<half_t *>(output->data);
  half_t *vec = static_cast<half_t *>(operand0->data);
  half_t *mat = static_cast<half_t *>(op2->data);

  for (size_t n = 0; n < output->bshape.n; ++n) {
    for (size_t c = 0; c < output->bshape.c; ++c) {
      for (size_t h = 0; h < output->bshape.h; ++h) {
        for (size_t w = 0; w < output->bshape.w; ++w) {
          // From the test examples, it looks as if the matrix doesn't have n !=
          // 1, but the same weight matrix is used for all vectors in a batch.
          size_t offsetMat =
              w * op2->bshape.w + c * op2->bshape.h * op2->bshape.w;
          size_t offsetVec = c * operand0->bshape.w +
                             n * operand0->bshape.c * operand0->bshape.w;
          half_t acc;
          for (size_t k = 0; k < operand0->bshape.w; ++k) {
            acc += mat[offsetMat + k] * vec[offsetVec + k];
          }
          size_t offsetOut =
              n * output->bshape.c * output->bshape.w + c * output->bshape.w;
          out[offsetOut + w] = acc;
        }
      }
    }
  }
  return SUCCESS;
}

int PimExecuteGemvAdd(PimBo *output, PimBo *operand0, PimBo *operand1, void *,
                      bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.

  // According to the documentation in the header, this is supposed to
  // calculate 'output = output + GEMV(operand0, operand1)'.
  // Create an intermediate buffer to hold the result of the GEMV operation.
  PimBo temp{output->mem_type,
             output->bshape,
             output->bshape_r,
             output->precision,
             output->size,
             nullptr,
             false};
  int failed = PimAllocMemory(&temp);
  if (!failed) {
    failed |= PimExecuteGemv(&temp, operand0, operand1);
    failed |= PimExecuteAdd(output, output, &temp);
  }
  failed |= PimFreeMemory(&temp);
  return failed;
}

int PimExecuteGemvAdd(PimBo *output, PimBo *operand0, PimBo *operand1,
                      PimBo *operand2, bool relu, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.

  // Guessing from the documentation in the header, this is supposed to
  // calculate 'output = operand2 + GEMV(operand0, operand1)' and potentially
  // apply RELU to the output before returning.
  int failed = PimExecuteGemv(output, operand0, operand1);
  failed |= PimExecuteAdd(output, output, operand2);
  if (relu) {
    failed |= PimExecuteRelu(output, output);
  }
  return failed;
}

int PimExecuteBN(PimBo *output, PimBo *pim_data, PimBo *beta, PimBo *gamma,
                 PimBo *mean, PimBo *variance, double epsilon, void *, bool) {
  // FIXME(Lukas): We ignore the non-blocking mode. We could implement
  // asynchronous/non-blocking operation in the future.

  // The PIM SDK uses the following layout for the operands and result of BN,
  // each given as (w, h, c, n):
  // output:    (W, H, C, N)
  // input:     (W, H, C, N)
  // beta:      (1, 1, C, 1)
  // gamma:     (1, 1, C, 1)
  // mean:      (1, 1, C, 1)
  // variance:  (1, 1, C, 1)
  // epsilon:   single scalar

  size_t numChannels = pim_data->bshape.c;
  if (output->size != pim_data->size || beta->bshape.c != numChannels ||
      gamma->bshape.c != numChannels || mean->bshape.c != numChannels ||
      variance->bshape.c != numChannels) {
    return OPERATION_ERROR;
  }

  auto dataShape = pim_data->bshape;
  auto *inPtr = static_cast<half_t *>(pim_data->data);
  auto *outPtr = static_cast<half_t *>(output->data);
  for (size_t n = 0; n < dataShape.n; ++n) {
    for (size_t c = 0; c < dataShape.c; ++c) {
      size_t dataOffset = n * dataShape.c * dataShape.h * dataShape.w +
                          c * dataShape.h * dataShape.w;
      // Assuming that n, h and w are all '1' for these buffers.
      auto sBeta = static_cast<half_t *>(beta->data)[c];
      auto sGamma = static_cast<half_t *>(gamma->data)[c];
      auto sMean = static_cast<half_t *>(mean->data)[c];
      auto sDivisor = sqrt(static_cast<half_t *>(variance->data)[c] +
                           static_cast<half_t>(epsilon));
      for (size_t i = 0; i < dataShape.h * dataShape.w; ++i) {
        auto x = inPtr[dataOffset + i];
        auto x_norm = (x - sMean) / sDivisor;
        outPtr[dataOffset + i] = sGamma * x_norm + sBeta;
      }
    }
  }
  return SUCCESS;
}

int PimSynchronize(void *) {
  // Currently nothing to do for synchronization.
  return SUCCESS;
}

int PimExecuteDummy() {
  // Nothing to do here.
  return SUCCESS;
}

} // namespace mock
} // namespace pim
