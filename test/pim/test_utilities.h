/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is a property of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "half.hpp"
#include "pim_data_types.h"
#include "stdio.h"
#include <iostream>
#include <vector>

//
// The functions in this file have been extracted from the code base
// of Samsung's PIMLibrary to be used with the tests here.
// DO NOT DISTRIBUTE THIS FILE!
//

inline int load_data(const char *filename, char *data, size_t size) {
  FILE *fp = fopen(filename, "r");

  if (fp == nullptr) {
    printf("fopen error : %s\n", filename);
    return -1;
  }

  for (int i = 0; i < size; i++) {
    fscanf(fp, "%c", &data[i]);
  }
  fclose(fp);

  return 0;
}

inline int compare_half_Ulps_and_absoulte(half_float::half data_a,
                                          half_float::half data_b,
                                          int allow_bit_cnt,
                                          float absTolerance = 0.001) {
  uint16_t Ai = *((uint16_t *)&data_a);
  uint16_t Bi = *((uint16_t *)&data_b);

  float diff = fabs((float)data_a - (float)data_b);
  int maxUlpsDiff = 1 << allow_bit_cnt;

  if (diff <= absTolerance) {
    return true;
  }

  if ((Ai & (1 << 15)) != (Bi & (1 << 15))) {
    if (Ai == Bi)
      return true;
    return false;
  }

  // Find the difference in ULPs.
  int ulpsDiff = abs(Ai - Bi);

  if (ulpsDiff <= maxUlpsDiff)
    return true;

  return false;
}

inline int compare_half_relative(half_float::half *data_a,
                                 half_float::half *data_b, int size,
                                 float absTolerance = 0.0001) {
  int pass_cnt = 0;
  int warning_cnt = 0;
  int fail_cnt = 0;
  int ret = 0;
  std::vector<int> fail_idx;
  std::vector<float> fail_data_pim;
  std::vector<float> fail_data_goldeny;

  float max_diff = 0.0;
  int pass_bit_cnt = 4;
  int warn_bit_cnt = 8;

  for (int i = 0; i < size; i++) {
    if (compare_half_Ulps_and_absoulte(data_a[i], data_b[i], pass_bit_cnt)) {
      // std::cout << "c data_a : " << (float)data_a[i] << " data_b : "
      // <<(float)data_b[i]  << std::endl;
      pass_cnt++;
    } else if (compare_half_Ulps_and_absoulte(data_a[i], data_b[i],
                                              warn_bit_cnt, absTolerance)) {
      // std::cout << "w data_a : " << (float)data_a[i] << " data_b : "
      // <<(float)data_b[i]  << std::endl;
      warning_cnt++;
    } else {
      if (std::abs(float(data_a[i]) - float(data_b[i])) > max_diff) {
        max_diff = std::abs(float(data_a[i]) - float(data_b[i]));
      }
      std::cout << "@ index " << i << ": f data_a : " << (float)data_a[i]
                << " data_b : " << (float)data_b[i] << std::endl;

      fail_idx.push_back(pass_cnt + warning_cnt + fail_cnt);
      fail_data_pim.push_back((float)data_a[i]);
      fail_data_goldeny.push_back((float)data_b[i]);

      fail_cnt++;
      ret = 1;
    }
  }

  int quasi_cnt = pass_cnt + warning_cnt;
  if (ret) {
    printf("relative - pass_cnt : %d, warning_cnt : %d, fail_cnt : %d, pass "
           "ratio : %f, max diff : %f\n",
           pass_cnt, warning_cnt, fail_cnt,
           ((float)quasi_cnt /
            ((float)fail_cnt + (float)warning_cnt + (float)pass_cnt) * 100),
           max_diff);
#ifdef DEBUG_PIM
    for (int i = 0; i < fail_idx.size(); i++) {
      std::cout << fail_idx[i] << " pim : " << fail_data_pim[i]
                << " golden :" << fail_data_goldeny[i] << std::endl;
    }
#endif
  }

  return ret;
}
