/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TRIM_H
#define FS_TRIM_H
#include "../stdafx.h"
namespace fs
{
// remove leading and trailing spaces
[[nodiscard]] inline string trim_copy(string s)
{
  auto j = static_cast<int>(s.size() - 1);
  while (j > 0 && ' ' == s.at(static_cast<size_t>(j)))
  {
    --j;
  }
  if (0 >= j)
  {
    // string is empty or all whitespace
    return "";
  }
  int i = 0;
  while (i < j && ' ' == s.at(static_cast<size_t>(i)))
  {
    ++i;
  }
  return s.substr(static_cast<size_t>(i), static_cast<size_t>(j - i + 1));
}
}
#endif
