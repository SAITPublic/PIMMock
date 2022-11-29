/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is a property of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system, or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 * (Use of the Software is restricted to non-commercial, personal or academic, research purpose only)
 */ 

#include "half.hpp"
#include "pim_runtime_api.h"
#include "test_utilities.h"
#include <algorithm>
#include <assert.h>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <stdio.h>
#include <stdlib.h>

#define IN_LENGTH 1024
#define BATCH_DIM 1

using half_float::half;

using namespace pim::mock;

template <class T>
void fill_uniform_random_values(void *data, uint32_t count, T start, T end) {
  std::random_device rd;
  std::mt19937 mt(rd());

  std::uniform_real_distribution<double> dist(start, end);

  for (int i = 0; i < count; i++)
    ((T *)data)[i] = dist(mt);
}

bool simple_pim_alloc_free() {
  PimBo pim_weight = {.mem_type = MEM_TYPE_PIM,
                      .size = IN_LENGTH * sizeof(half)};

  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  int ret = PimAllocMemory(&pim_weight);
  if (ret)
    return false;

  ret = PimFreeMemory(&pim_weight);
  if (ret)
    return false;

  PimDeinitialize();

  return true;
}

bool pim_repeat_allocate_free(void) {
  PimBo pim_weight = {.mem_type = MEM_TYPE_PIM,
                      .size = IN_LENGTH * sizeof(half)};

  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  int i = 0;
  while (i < 100) {
    int ret = PimAllocMemory(&pim_weight);
    if (ret)
      return false;
    ret = PimFreeMemory(&pim_weight);
    if (ret)
      return false;
    i++;
  }

  PimDeinitialize();

  return true;
}

bool pim_allocate_exceed_blocksize(void) {
  std::vector<PimBo> pimObjPtr;

  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  int ret;
  while (true) {
    PimBo pim_weight = {.mem_type = MEM_TYPE_PIM,
                        .size = IN_LENGTH * sizeof(half) * 1024 * 1024};
    ret = PimAllocMemory(&pim_weight);
    if (ret)
      break;
    pimObjPtr.push_back(pim_weight);
  }

  for (int i = 0; i < pimObjPtr.size(); i++)
    PimFreeMemory(&pimObjPtr[i]);

  PimDeinitialize();

  if (ret)
    return false;

  return true;
}

bool test_memcpy_bw_host_device() {
  int ret = 0;

  /* __PIM_API__ call : Initialize PimRuntime */
  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  /* __PIM_API__ call : Create PIM Buffer Object */
  PimBo *host_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);
  PimBo *device_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_DEVICE);
  PimBo *host_output =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);

  fill_uniform_random_values<half_float::half>(host_input->data, IN_LENGTH,
                                               (half_float::half)0.0,
                                               (half_float::half)0.5);
  PimCopyMemory(device_input, host_input, HOST_TO_DEVICE);
  PimCopyMemory(host_output, device_input, DEVICE_TO_HOST);

  ret = compare_half_relative((half_float::half *)host_input->data,
                              (half_float::half *)host_output->data, IN_LENGTH);
  if (ret != 0) {
    std::cout << "data is different" << std::endl;
    return false;
  }

  PimFreeMemory(host_input);
  PimFreeMemory(device_input);
  PimFreeMemory(host_output);

  PimDeinitialize();

  return true;
}

bool test_memcpy_bw_host_pim() {
  int ret = 0;

  /* __PIM_API__ call : Initialize PimRuntime */
  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  /* __PIM_API__ call : Create PIM Buffer Object */
  PimBo *host_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);
  PimBo *device_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_PIM);
  PimBo *host_output =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);

  fill_uniform_random_values<half_float::half>(host_input->data, IN_LENGTH,
                                               (half_float::half)0.0,
                                               (half_float::half)0.5);
  PimCopyMemory(device_input, host_input, HOST_TO_PIM);
  PimCopyMemory(host_output, device_input, PIM_TO_HOST);

  ret = compare_half_relative((half_float::half *)host_input->data,
                              (half_float::half *)host_output->data, IN_LENGTH);
  if (ret != 0) {
    std::cout << "data is different" << std::endl;
    return false;
  }

  PimFreeMemory(host_input);
  PimFreeMemory(device_input);
  PimFreeMemory(host_output);

  PimDeinitialize();

  return true;
}

bool test_memcpy_bw_device_pim() {
  int ret = 0;

  /* __PIM_API__ call : Initialize PimRuntime */
  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  /* __PIM_API__ call : Create PIM Buffer Object */
  PimBo *host_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);
  PimBo *device_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_DEVICE);
  PimBo *pim_input =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_PIM);
  PimBo *host_output =
      PimCreateBo(IN_LENGTH, 1, 1, BATCH_DIM, PIM_FP16, MEM_TYPE_HOST);

  fill_uniform_random_values<half_float::half>(host_input->data, IN_LENGTH,
                                               (half_float::half)0.0,
                                               (half_float::half)0.5);
  PimCopyMemory(device_input, host_input, HOST_TO_DEVICE);
  PimCopyMemory(pim_input, device_input, DEVICE_TO_PIM);
  PimCopyMemory(host_output, pim_input, PIM_TO_HOST);

  ret = compare_half_relative((half_float::half *)host_input->data,
                              (half_float::half *)host_output->data, IN_LENGTH);
  if (ret != 0) {
    std::cout << "data is different" << std::endl;
    return false;
  }

  PimFreeMemory(host_input);
  PimFreeMemory(device_input);
  PimFreeMemory(pim_input);
  PimFreeMemory(host_output);

  PimDeinitialize();

  return true;
}

TEST(UnitTest, PimMemCopyHostAndDeviceTest) {
  EXPECT_TRUE(test_memcpy_bw_host_device());
}
TEST(UnitTest, PimMemCopyHostAndPimTest) {
  EXPECT_TRUE(test_memcpy_bw_host_pim());
}
TEST(UnitTest, PimMemCopyDeviceAndPimTest) {
  EXPECT_TRUE(test_memcpy_bw_device_pim());
}
TEST(UnitTest, simplePimAllocFree) { EXPECT_TRUE(simple_pim_alloc_free()); }
TEST(UnitTest, PimRepeatAllocateFree) {
  EXPECT_TRUE(pim_repeat_allocate_free());
}
// The following test is unsupported because we do all allocation on the host
// and will run into std::bad_alloc after depleting the hosts RAM.
//TEST(UnitTest, PimAllocateExceedBlocksize) {
//  EXPECT_FALSE(pim_allocate_exceed_blocksize());
//}
