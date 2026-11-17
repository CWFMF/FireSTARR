/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_MERGEITERATOR_H
#define FS_MERGEITERATOR_H
#include "stdafx.h"
namespace fs
{
// mangled version of std::transform_reduce() that calls .begin() and .end()
template <typename _ForwardIteratorSource, class _Tp, class _BinaryOperation, class _UnaryOperation>
_Tp do_transform_reduce(
  _ForwardIteratorSource&& container,
  _Tp __init,
  _BinaryOperation __binary_op,
  _UnaryOperation __unary_op
)
{
  // to help compiler determine type
  using _ForwardIterator = decltype(container.begin());
  return std::transform_reduce(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    static_cast<_ForwardIterator>(container.begin()),
    static_cast<_ForwardIterator>(container.end()),
    __init,
    __binary_op,
    __unary_op
  );
}
template <typename M, class F>
const M merge_maps_generic(const M& lhs, const M& rhs, F f)
{
  using value_type = typename M::value_type;
  using key_type = typename M::key_type;
  using mapped_type = typename M::mapped_type;
  std::function<mapped_type(const mapped_type&, const mapped_type&)> fct_merge = f;
  M out{};
  auto it_lhs = lhs.begin();
  auto it_rhs = rhs.begin();
  auto end_lhs = lhs.end();
  auto end_rhs = rhs.end();
  while (true)
  {
    if (end_lhs == it_lhs)
    {
      while (end_rhs != it_rhs)
      {
        out.emplace(*it_rhs);
        ++it_rhs;
      }
      break;
    }
    if (end_rhs == it_rhs)
    {
      while (end_lhs != it_lhs)
      {
        out.emplace(*it_lhs);
        ++it_lhs;
      }
      break;
    }
    // at end of neither so pick lower value
    const value_type& pair0 = *it_lhs;
    const value_type& pair1 = *it_rhs;
    const key_type& k0 = pair0.first;
    const key_type& k1 = pair1.first;
    const mapped_type& m0 = pair0.second;
    const mapped_type& m1 = pair1.second;
    if (k0 < k1)
    {
      out.emplace(pair0);
      ++it_lhs;
    }
    else if (k0 > k1)
    {
      out.emplace(pair1);
      ++it_rhs;
    }
    else
    {
      assert(k0 == k1);
      const mapped_type merged = fct_merge(m0, m1);
      out.emplace(pair<const key_type, const mapped_type>(k0, merged));
      ++it_lhs;
      ++it_rhs;
    }
  }
  return out;
}
}
#endif
