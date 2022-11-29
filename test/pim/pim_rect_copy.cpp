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

#include "half.hpp"
#include "pim_runtime_api.h"
#include "test_utilities.h"
#include <gtest/gtest.h>
#include <iostream>

using half_float::half;
using namespace half_float::literal;

using namespace pim::mock;

TEST(HIPIntegrationTest, PimCopyRect3D) {
  // Copy a 2x2x2 cube between a 4x4x3 buffer on the host and a 4x4x3 buffer on
  // the device.

  half host[] = {1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h,
                 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h,
                 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h, 42.0_h, 42.0_h, 1.0_h,
                 1.0_h, 42.0_h, 42.0_h, 1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h,
                 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h, 42.0_h, 42.0_h, 1.0_h,
                 1.0_h, 42.0_h, 42.0_h, 1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h};

  half device[] = {1.0_h,  1.0_h,  1.0_h, 1.0_h, 42.0_h, 42.0_h, 1.0_h, 1.0_h,
                   42.0_h, 42.0_h, 1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h,
                   1.0_h,  1.0_h,  1.0_h, 1.0_h, 42.0_h, 42.0_h, 1.0_h, 1.0_h,
                   42.0_h, 42.0_h, 1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h,
                   1.0_h,  1.0_h,  1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h,
                   1.0_h,  1.0_h,  1.0_h, 1.0_h, 1.0_h,  1.0_h,  1.0_h, 1.0_h};
  
  half deviceCheck[48];
  for(size_t i=0; i < 48; ++i){
    deviceCheck[i] = 1.0_h;
  }

  auto *deviceMem = PimCreateBo(4, 4, 3, 1, PIM_FP16, MEM_TYPE_PIM);

  // Initialize the device buffer with ones before performing rectangular copy.
  PimCopyMemory(deviceMem->data, deviceCheck, sizeof(deviceCheck), HOST_TO_PIM);

  PimCopy3D copyH2D;
  // Source information.
  copyH2D.src_x_in_bytes = 2; // Offset of 1 FP16 element = 2 bytes.
  copyH2D.src_y = 1;
  copyH2D.src_z = 1;
  copyH2D.src_mem_type = MEM_TYPE_HOST;
  copyH2D.src_ptr = host;
  copyH2D.src_pitch = 8; // 4 FP16 elements per row = 8 bytes.
  copyH2D.src_height = 4;
  copyH2D.src_bo = nullptr;
  // Destination information.
  copyH2D.dst_x_in_bytes = 0;
  copyH2D.dst_y = 1;
  copyH2D.dst_z = 0;
  copyH2D.dst_mem_type = MEM_TYPE_PIM;
  copyH2D.dst_ptr = nullptr;
  copyH2D.dst_pitch = 0; // Ignored
  copyH2D.dst_height = 0; // Ignored
  copyH2D.dst_bo = deviceMem;
  // Slice information
  copyH2D.width_in_bytes = 4; // Two FP16 elements = 4 bytes.
  copyH2D.height = 2;
  copyH2D.depth = 2;

  PimCopyMemoryRect(&copyH2D);

  // Copy the complete content of the device buffer to verify its content.
  PimCopyMemory(deviceCheck, deviceMem->data, sizeof(deviceCheck), PIM_TO_HOST);

  ASSERT_FALSE(compare_half_relative(device, deviceCheck, 48));
  half hostCheck[48];
  for (size_t i = 0; i < 48; ++i) {
    hostCheck[i] = 1.0_h;
  }

  PimCopy3D copyD2H;
  // Source information.
  copyD2H.src_x_in_bytes = 0;
  copyD2H.src_y = 1;
  copyD2H.src_z = 0;
  copyD2H.src_mem_type = MEM_TYPE_PIM;
  copyD2H.src_ptr = nullptr;
  copyD2H.src_pitch = 0;  // Ignored
  copyD2H.src_height = 0; // Ignored
  copyD2H.src_bo = deviceMem;
  // Destination information.
  copyD2H.dst_x_in_bytes = 2; // Offset of 1 FP16 element = 2 bytes.
  copyD2H.dst_y = 1;
  copyD2H.dst_z = 1;
  copyD2H.dst_mem_type = MEM_TYPE_HOST;
  copyD2H.dst_ptr = hostCheck;
  copyD2H.dst_pitch = 8; // 4 FP16 elements per row = 8 bytes.
  copyD2H.dst_height = 4;
  copyD2H.dst_bo = nullptr;
  // Slice information
  copyD2H.width_in_bytes = 4; // Two FP16 elements = 4 bytes.
  copyD2H.height = 2;
  copyD2H.depth = 2;

  PimCopyMemoryRect(&copyD2H);

  ASSERT_FALSE(compare_half_relative(host, hostCheck, 48));
}
