//==----------- group_algorithm.hpp --- SYCL group algorithm----------------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include <CL/__spirv/spirv_ops.hpp>
#include <CL/__spirv/spirv_types.hpp>
#include <CL/__spirv/spirv_vars.hpp>
#include <CL/sycl/ONEAPI/atomic.hpp>
#include <CL/sycl/ONEAPI/functional.hpp>
#include <CL/sycl/ONEAPI/sub_group.hpp>
#include <CL/sycl/detail/spirv.hpp>
#include <CL/sycl/detail/type_traits.hpp>
#include <CL/sycl/group.hpp>
#include <CL/sycl/nd_item.hpp>

#ifndef __DISABLE_SYCL_ONEAPI_GROUP_ALGORITHMS__
__SYCL_INLINE_NAMESPACE(cl) {
namespace sycl {
namespace detail {

template <typename Group> size_t get_local_linear_range(Group g);
template <> inline size_t get_local_linear_range<group<1>>(group<1> g) {
  return g.get_local_range(0);
}
template <> inline size_t get_local_linear_range<group<2>>(group<2> g) {
  return g.get_local_range(0) * g.get_local_range(1);
}
template <> inline size_t get_local_linear_range<group<3>>(group<3> g) {
  return g.get_local_range(0) * g.get_local_range(1) * g.get_local_range(2);
}
template <>
inline size_t get_local_linear_range<ONEAPI::sub_group>(ONEAPI::sub_group g) {
  return g.get_local_range()[0];
}

template <typename Group>
typename Group::linear_id_type get_local_linear_id(Group g);

#ifdef __SYCL_DEVICE_ONLY__
#define __SYCL_GROUP_GET_LOCAL_LINEAR_ID(D)                                    \
  template <>                                                                  \
  group<D>::linear_id_type get_local_linear_id<group<D>>(group<D>) {           \
    nd_item<D> it = cl::sycl::detail::Builder::getNDItem<D>();                 \
    return it.get_local_linear_id();                                           \
  }
__SYCL_GROUP_GET_LOCAL_LINEAR_ID(1);
__SYCL_GROUP_GET_LOCAL_LINEAR_ID(2);
__SYCL_GROUP_GET_LOCAL_LINEAR_ID(3);
#undef __SYCL_GROUP_GET_LOCAL_LINEAR_ID
#endif // __SYCL_DEVICE_ONLY__

template <>
inline ONEAPI::sub_group::linear_id_type
get_local_linear_id<ONEAPI::sub_group>(ONEAPI::sub_group g) {
  return g.get_local_id()[0];
}

template <int Dimensions>
id<Dimensions> linear_id_to_id(range<Dimensions>, size_t linear_id);
template <> inline id<1> linear_id_to_id(range<1>, size_t linear_id) {
  return id<1>(linear_id);
}
template <> inline id<2> linear_id_to_id(range<2> r, size_t linear_id) {
  id<2> result;
  result[0] = linear_id / r[1];
  result[1] = linear_id % r[1];
  return result;
}
template <> inline id<3> linear_id_to_id(range<3> r, size_t linear_id) {
  id<3> result;
  result[0] = linear_id / (r[1] * r[2]);
  result[1] = (linear_id % (r[1] * r[2])) / r[2];
  result[2] = linear_id % r[2];
  return result;
}

template <typename T, class BinaryOperation> struct identity {};

template <typename T, typename V> struct identity<T, ONEAPI::plus<V>> {
  static constexpr T value = 0;
};

template <typename T, typename V> struct identity<T, ONEAPI::minimum<V>> {
  static constexpr T value = std::numeric_limits<T>::has_infinity
                                 ? std::numeric_limits<T>::infinity()
                                 : (std::numeric_limits<T>::max)();
};

template <typename T, typename V> struct identity<T, ONEAPI::maximum<V>> {
  static constexpr T value =
      std::numeric_limits<T>::has_infinity
          ? static_cast<T>(-std::numeric_limits<T>::infinity())
          : std::numeric_limits<T>::lowest();
};

template <typename T, typename V> struct identity<T, ONEAPI::multiplies<V>> {
  static constexpr T value = static_cast<T>(1);
};

template <typename T, typename V> struct identity<T, ONEAPI::bit_or<V>> {
  static constexpr T value = 0;
};

template <typename T, typename V> struct identity<T, ONEAPI::bit_xor<V>> {
  static constexpr T value = 0;
};

template <typename T, typename V> struct identity<T, ONEAPI::bit_and<V>> {
  static constexpr T value = ~static_cast<T>(0);
};

template <typename T>
using native_op_list =
    type_list<ONEAPI::plus<T>, ONEAPI::bit_or<T>, ONEAPI::bit_xor<T>,
              ONEAPI::bit_and<T>, ONEAPI::maximum<T>, ONEAPI::minimum<T>,
              ONEAPI::multiplies<T>>;

template <typename T, typename BinaryOperation> struct is_native_op {
  static constexpr bool value =
      is_contained<BinaryOperation, native_op_list<T>>::value ||
      is_contained<BinaryOperation, native_op_list<void>>::value;
};

template <typename Group, typename Ptr, class Function>
Function for_each(Group g, Ptr first, Ptr last, Function f) {
#ifdef __SYCL_DEVICE_ONLY__
  ptrdiff_t offset = sycl::detail::get_local_linear_id(g);
  ptrdiff_t stride = sycl::detail::get_local_linear_range(g);
  for (Ptr p = first + offset; p < last; p += stride) {
    f(*p);
  }
  return f;
#else
  (void)g;
  (void)first;
  (void)last;
  (void)f;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

} // namespace detail

namespace ONEAPI {

// EnableIf shorthands for algorithms that depend only on type
template <typename T>
using EnableIfIsScalarArithmetic = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_scalar_arithmetic<T>::value, T>;

template <typename T>
using EnableIfIsVectorArithmetic = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_vector_arithmetic<T>::value, T>;

template <typename Ptr, typename T>
using EnableIfIsPointer =
    cl::sycl::detail::enable_if_t<cl::sycl::detail::is_pointer<Ptr>::value, T>;

template <typename T>
using EnableIfIsTriviallyCopyable = cl::sycl::detail::enable_if_t<
    std::is_trivially_copyable<T>::value &&
        !cl::sycl::detail::is_vector_arithmetic<T>::value,
    T>;

// EnableIf shorthands for algorithms that depend on type and an operator
template <typename T, typename BinaryOperation>
using EnableIfIsScalarArithmeticNativeOp = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_scalar_arithmetic<T>::value &&
        cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

template <typename T, typename BinaryOperation>
using EnableIfIsVectorArithmeticNativeOp = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_vector_arithmetic<T>::value &&
        cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

// TODO: Lift TriviallyCopyable restriction eventually
template <typename T, typename BinaryOperation>
using EnableIfIsNonNativeOp = cl::sycl::detail::enable_if_t<
    (!cl::sycl::detail::is_scalar_arithmetic<T>::value &&
     !cl::sycl::detail::is_vector_arithmetic<T>::value &&
     std::is_trivially_copyable<T>::value) ||
        !cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

template <typename Group> bool all_of(Group, bool pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::spirv::GroupAll<Group>(pred);
#else
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class Predicate>
bool all_of(Group g, T x, Predicate pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  return all_of(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
EnableIfIsPointer<Ptr, bool> all_of(Group g, Ptr first, Ptr last,
                                    Predicate pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  bool partial = true;
  sycl::detail::for_each(
      g, first, last,
      [&](const typename Ptr::element_type &x) { partial &= pred(x); });
  return all_of(g, partial);
#else
  (void)g;
  (void)first;
  (void)last;
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group> bool any_of(Group, bool pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::spirv::GroupAny<Group>(pred);
#else
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class Predicate>
bool any_of(Group g, T x, Predicate pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  return any_of(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
EnableIfIsPointer<Ptr, bool> any_of(Group g, Ptr first, Ptr last,
                                    Predicate pred) {
#ifdef __SYCL_DEVICE_ONLY__
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  bool partial = false;
  sycl::detail::for_each(
      g, first, last,
      [&](const typename Ptr::element_type &x) { partial |= pred(x); });
  return any_of(g, partial);
#else
  (void)g;
  (void)first;
  (void)last;
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group> bool none_of(Group, bool pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::spirv::GroupAll<Group>(!pred);
#else
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class Predicate>
bool none_of(Group g, T x, Predicate pred) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  return none_of(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
EnableIfIsPointer<Ptr, bool> none_of(Group g, Ptr first, Ptr last,
                                     Predicate pred) {
#ifdef __SYCL_DEVICE_ONLY__
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  return !any_of(g, first, last, pred);
#else
  (void)g;
  (void)first;
  (void)last;
  (void)pred;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsTriviallyCopyable<T> broadcast(Group, T x,
                                         typename Group::id_type local_id) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::spirv::GroupBroadcast<Group>(x, local_id);
#else
  (void)x;
  (void)local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsVectorArithmetic<T> broadcast(Group g, T x,
                                        typename Group::id_type local_id) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s], local_id);
  }
  return result;
#else
  (void)g;
  (void)x;
  (void)local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsTriviallyCopyable<T>
broadcast(Group g, T x, typename Group::linear_id_type linear_local_id) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return broadcast(
      g, x,
      sycl::detail::linear_id_to_id(g.get_local_range(), linear_local_id));
#else
  (void)g;
  (void)x;
  (void)linear_local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsVectorArithmetic<T>
broadcast(Group g, T x, typename Group::linear_id_type linear_local_id) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s], linear_local_id);
  }
  return result;
#else
  (void)g;
  (void)x;
  (void)linear_local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsTriviallyCopyable<T> broadcast(Group g, T x) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  return broadcast(g, x, 0);
#else
  (void)g;
  (void)x;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
EnableIfIsVectorArithmetic<T> broadcast(Group g, T x) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s]);
  }
  return result;
#else
  (void)g;
  (void)x;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
reduce(Group, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(x, x)), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(x, x)), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::calc<T, __spv::GroupOperation::Reduce,
                            sycl::detail::spirv::group_scope<Group>::value>(
      typename sycl::detail::GroupOpTag<T>::type(), x, binary_op);
#else
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
reduce(Group g, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(x[0], x[0])),
                   typename T::element_type>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(x[0], x[0])), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = reduce(g, x[s], binary_op);
  }
  return result;
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsNonNativeOp<T, BinaryOperation> reduce(Group g, T x,
                                                 BinaryOperation op) {
  static_assert(sycl::detail::is_sub_group<Group>::value,
                "reduce algorithm with user-defined types and operators"
                "only supports ONEAPI::sub_group class.");
  T result = x;
  for (int mask = 1; mask < g.get_max_local_range()[0]; mask *= 2) {
    T tmp = g.shuffle_xor(result, id<1>(mask));
    if ((g.get_local_id()[0] ^ mask) < g.get_local_range()[0]) {
      result = op(result, tmp);
    }
  }
  return g.shuffle(result, 0);
}

template <typename Group, typename V, typename T, class BinaryOperation>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
reduce(Group g, V x, T init, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init, x)), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init, x)), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  return binary_op(init, reduce(g, x, binary_op));
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename V, typename T, class BinaryOperation>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
reduce(Group g, V x, T init, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init[0], x[0])),
                   typename T::element_type>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init[0], x[0])), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  T result = init;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = binary_op(init[s], reduce(g, x[s], binary_op));
  }
  return result;
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename V, typename T, class BinaryOperation>
EnableIfIsNonNativeOp<T, BinaryOperation> reduce(Group g, V x, T init,
                                                 BinaryOperation op) {
  static_assert(sycl::detail::is_sub_group<Group>::value,
                "reduce algorithm with user-defined types and operators"
                "only supports ONEAPI::sub_group class.");
  T result = x;
  for (int mask = 1; mask < g.get_max_local_range()[0]; mask *= 2) {
    T tmp = g.shuffle_xor(result, id<1>(mask));
    if ((g.get_local_id()[0] ^ mask) < g.get_local_range()[0]) {
      result = op(result, tmp);
    }
  }
  return g.shuffle(op(init, result), 0);
}

template <typename Group, typename Ptr, class BinaryOperation>
EnableIfIsPointer<Ptr, typename Ptr::element_type>
reduce(Group g, Ptr first, Ptr last, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(*first, *first)),
                   typename Ptr::element_type>::value ||
          (std::is_same<typename Ptr::element_type, half>::value &&
           std::is_same<decltype(binary_op(*first, *first)), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  typename Ptr::element_type partial =
      sycl::detail::identity<typename Ptr::element_type,
                             BinaryOperation>::value;
  sycl::detail::for_each(g, first, last,
                         [&](const typename Ptr::element_type &x) {
                           partial = binary_op(partial, x);
                         });
  return reduce(g, partial, binary_op);
#else
  (void)g;
  (void)last;
  (void)binary_op;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename Ptr, typename T, class BinaryOperation>
EnableIfIsPointer<Ptr, T> reduce(Group g, Ptr first, Ptr last, T init,
                                 BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init, *first)), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init, *first)), float>::value),
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  T partial = sycl::detail::identity<typename Ptr::element_type,
                                     BinaryOperation>::value;
  sycl::detail::for_each(g, first, last,
                         [&](const typename Ptr::element_type &x) {
                           partial = binary_op(partial, x);
                         });
  return reduce(g, partial, init, binary_op);
#else
  (void)g;
  (void)last;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
exclusive_scan(Group, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(std::is_same<decltype(binary_op(x, x)), T>::value ||
                    (std::is_same<T, half>::value &&
                     std::is_same<decltype(binary_op(x, x)), float>::value),
                "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::calc<T, __spv::GroupOperation::ExclusiveScan,
                            sycl::detail::spirv::group_scope<Group>::value>(
      typename sycl::detail::GroupOpTag<T>::type(), x, binary_op);
#else
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
exclusive_scan(Group g, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(x[0], x[0])),
                   typename T::element_type>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(x[0], x[0])), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = exclusive_scan(g, x[s], binary_op);
  }
  return result;
}

template <typename Group, typename V, typename T, class BinaryOperation>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
exclusive_scan(Group g, V x, T init, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init[0], x[0])),
                   typename T::element_type>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init[0], x[0])), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = exclusive_scan(g, x[s], init[s], binary_op);
  }
  return result;
}

template <typename Group, typename V, typename T, class BinaryOperation>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
exclusive_scan(Group g, V x, T init, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(std::is_same<decltype(binary_op(init, x)), T>::value ||
                    (std::is_same<T, half>::value &&
                     std::is_same<decltype(binary_op(init, x)), float>::value),
                "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  typename Group::linear_id_type local_linear_id =
      sycl::detail::get_local_linear_id(g);
  if (local_linear_id == 0) {
    x = binary_op(init, x);
  }
  T scan = exclusive_scan(g, x, binary_op);
  if (local_linear_id == 0) {
    scan = init;
  }
  return scan;
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename InPtr, typename OutPtr, typename T,
          class BinaryOperation>
EnableIfIsPointer<InPtr, OutPtr>
exclusive_scan(Group g, InPtr first, InPtr last, OutPtr result, T init,
               BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(*first, *first)), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(*first, *first)), float>::value),
      "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  ptrdiff_t offset = sycl::detail::get_local_linear_id(g);
  ptrdiff_t stride = sycl::detail::get_local_linear_range(g);
  ptrdiff_t N = last - first;
  auto roundup = [=](const ptrdiff_t &v,
                     const ptrdiff_t &divisor) -> ptrdiff_t {
    return ((v + divisor - 1) / divisor) * divisor;
  };
  typename InPtr::element_type x;
  typename OutPtr::element_type carry = init;
  for (ptrdiff_t chunk = 0; chunk < roundup(N, stride); chunk += stride) {
    ptrdiff_t i = chunk + offset;
    if (i < N) {
      x = first[i];
    }
    typename OutPtr::element_type out = exclusive_scan(g, x, carry, binary_op);
    if (i < N) {
      result[i] = out;
    }
    carry = broadcast(g, binary_op(out, x), stride - 1);
  }
  return result + N;
#else
  (void)g;
  (void)last;
  (void)result;
  (void)init;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation>
EnableIfIsPointer<InPtr, OutPtr> exclusive_scan(Group g, InPtr first,
                                                InPtr last, OutPtr result,
                                                BinaryOperation binary_op) {
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(*first, *first)),
                   typename OutPtr::element_type>::value ||
          (std::is_same<typename OutPtr::element_type, half>::value &&
           std::is_same<decltype(binary_op(*first, *first)), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  return exclusive_scan(g, first, last, result,
                        sycl::detail::identity<typename OutPtr::element_type,
                                               BinaryOperation>::value,
                        binary_op);
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
inclusive_scan(Group g, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(x[0], x[0])),
                   typename T::element_type>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(x[0], x[0])), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = inclusive_scan(g, x[s], binary_op);
  }
  return result;
}

template <typename Group, typename T, class BinaryOperation>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
inclusive_scan(Group, T x, BinaryOperation binary_op) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(std::is_same<decltype(binary_op(x, x)), T>::value ||
                    (std::is_same<T, half>::value &&
                     std::is_same<decltype(binary_op(x, x)), float>::value),
                "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::calc<T, __spv::GroupOperation::InclusiveScan,
                            sycl::detail::spirv::group_scope<Group>::value>(
      typename sycl::detail::GroupOpTag<T>::type(), x, binary_op);
#else
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename V, class BinaryOperation, typename T>
EnableIfIsScalarArithmeticNativeOp<T, BinaryOperation>
inclusive_scan(Group g, V x, BinaryOperation binary_op, T init) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(std::is_same<decltype(binary_op(init, x)), T>::value ||
                    (std::is_same<T, half>::value &&
                     std::is_same<decltype(binary_op(init, x)), float>::value),
                "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  if (sycl::detail::get_local_linear_id(g) == 0) {
    x = binary_op(init, x);
  }
  return inclusive_scan(g, x, binary_op);
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename V, class BinaryOperation, typename T>
EnableIfIsVectorArithmeticNativeOp<T, BinaryOperation>
inclusive_scan(Group g, V x, BinaryOperation binary_op, T init) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init[0], x[0])), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init[0], x[0])), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = inclusive_scan(g, x[s], binary_op, init[s]);
  }
  return result;
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation, typename T>
EnableIfIsPointer<InPtr, OutPtr>
inclusive_scan(Group g, InPtr first, InPtr last, OutPtr result,
               BinaryOperation binary_op, T init) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(init, *first)), T>::value ||
          (std::is_same<T, half>::value &&
           std::is_same<decltype(binary_op(init, *first)), float>::value),
      "Result type of binary_op must match scan accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  ptrdiff_t offset = sycl::detail::get_local_linear_id(g);
  ptrdiff_t stride = sycl::detail::get_local_linear_range(g);
  ptrdiff_t N = last - first;
  auto roundup = [=](const ptrdiff_t &v,
                     const ptrdiff_t &divisor) -> ptrdiff_t {
    return ((v + divisor - 1) / divisor) * divisor;
  };
  typename InPtr::element_type x;
  typename OutPtr::element_type carry = init;
  for (ptrdiff_t chunk = 0; chunk < roundup(N, stride); chunk += stride) {
    ptrdiff_t i = chunk + offset;
    if (i < N) {
      x = first[i];
    }
    typename OutPtr::element_type out = inclusive_scan(g, x, binary_op, carry);
    if (i < N) {
      result[i] = out;
    }
    carry = broadcast(g, out, stride - 1);
  }
  return result + N;
#else
  (void)g;
  (void)last;
  (void)result;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation>
EnableIfIsPointer<InPtr, OutPtr> inclusive_scan(Group g, InPtr first,
                                                InPtr last, OutPtr result,
                                                BinaryOperation binary_op) {
  // FIXME: Do not special-case for half precision
  static_assert(
      std::is_same<decltype(binary_op(*first, *first)),
                   typename OutPtr::element_type>::value ||
          (std::is_same<typename OutPtr::element_type, half>::value &&
           std::is_same<decltype(binary_op(*first, *first)), float>::value),
      "Result type of binary_op must match scan accumulation type.");
  return inclusive_scan(g, first, last, result, binary_op,
                        sycl::detail::identity<typename OutPtr::element_type,
                                               BinaryOperation>::value);
}

template <typename Group> bool leader(Group g) {
  static_assert(sycl::detail::is_generic_group<Group>::value,
                "Group algorithms only support the sycl::group and "
                "ONEAPI::sub_group class.");
#ifdef __SYCL_DEVICE_ONLY__
  typename Group::linear_id_type linear_id =
      sycl::detail::get_local_linear_id(g);
  return (linear_id == 0);
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

} // namespace ONEAPI
} // namespace sycl
} // __SYCL_INLINE_NAMESPACE(cl)
#endif // __DISABLE_SYCL_ONEAPI_GROUP_ALGORITHMS__
