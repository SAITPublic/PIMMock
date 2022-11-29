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
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PIM
#define NUM_ITER (100)
#else
#define NUM_ITER (1)
#endif

using half_float::half;

using namespace pim::mock;

int pim_copy_up_to_256KB(bool block, uint32_t input_len) {
  int ret = 0;

  /* __PIM_API__ call : Initialize PimRuntime */
  PimInitialize(RT_TYPE_HIP, PIM_FP16);

  PimDesc *pim_desc = PimCreateDesc(1, 1, 1, input_len, PIM_FP16);

  /* __PIM_API__ call : Create PIM Buffer Object */
  PimBo *host_input = PimCreateBo(pim_desc, MEM_TYPE_HOST);
  PimBo *host_output = PimCreateBo(pim_desc, MEM_TYPE_HOST);
  PimBo *golden_output = PimCreateBo(pim_desc, MEM_TYPE_HOST);
  PimBo *pim_input = PimCreateBo(pim_desc, MEM_TYPE_PIM);
  PimBo *device_output = PimCreateBo(pim_desc, MEM_TYPE_PIM);

  /* Initialize the input, output data */
  std::string test_vector_data = TEST_VECTORS_DATA;

  std::string input = test_vector_data + "load/relu/input_256KB.dat";
  std::string output = test_vector_data + "load/relu/input_256KB.dat";
  std::string output_dump = test_vector_data + "dump/relu/output_256KB.dat";

  ret |= load_data(input.c_str(), (char *)host_input->data, host_input->size);
  ret |= load_data(output.c_str(), (char *)golden_output->data,
                   golden_output->size);

  /* __PIM_API__ call : Preload weight data on PIM memory */
  PimCopyMemory(pim_input, host_input, HOST_TO_PIM);
  for (int i = 0; i < NUM_ITER; i++) {
    /* __PIM_API__ call : Execute PIM kernel */
    PimCopyMemory(device_output, pim_input, PIM_TO_PIM);
    if (!block)
      PimSynchronize();

    PimCopyMemory(host_output, device_output, PIM_TO_HOST);

    ret |= compare_half_relative((half *)golden_output->data,
                                 (half *)host_output->data, input_len);
  }
  //    dump_data(output_dump.c_str(), (char*)host_output->data,
  //    host_output->size);

  /* __PIM_API__ call : Free memory */
  PimDestroyBo(host_input);
  PimDestroyBo(host_output);
  PimDestroyBo(golden_output);
  PimDestroyBo(device_output);
  PimDestroyBo(pim_input);

  /* __PIM_API__ call : Deinitialize PimRuntime */
  PimDeinitialize();

  return ret;
}

TEST(HIPIntegrationTest, PimCopy1Sync) {
  EXPECT_TRUE(pim_copy_up_to_256KB(true, 128 * 1024) == 0);
}
