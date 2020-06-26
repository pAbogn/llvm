
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Autogenerated by gen-libclc-test.py

// RUN: %clang -emit-llvm -S -o - %s | FileCheck %s

#include <spirv/spirv_types.h>

// CHECK-NOT: declare {{.*}} @_Z
// CHECK-NOT: call {{[^ ]*}} bitcast
__attribute__((overloadable)) __clc_fp32_t
test___spirv_ocl_fast_normalize(__clc_fp32_t args_0) {
  return __spirv_ocl_fast_normalize(args_0);
}

__attribute__((overloadable)) __clc_vec2_fp32_t
test___spirv_ocl_fast_normalize(__clc_vec2_fp32_t args_0) {
  return __spirv_ocl_fast_normalize(args_0);
}

__attribute__((overloadable)) __clc_vec3_fp32_t
test___spirv_ocl_fast_normalize(__clc_vec3_fp32_t args_0) {
  return __spirv_ocl_fast_normalize(args_0);
}

__attribute__((overloadable)) __clc_vec4_fp32_t
test___spirv_ocl_fast_normalize(__clc_vec4_fp32_t args_0) {
  return __spirv_ocl_fast_normalize(args_0);
}
