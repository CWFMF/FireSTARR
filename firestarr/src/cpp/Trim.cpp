/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Trim.h"
// from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
namespace fs::util
{
// trim from start (in place)
void trim_left(string* s)
{
  s->erase(s->begin(), find_if(s->begin(), s->end(), [](const int ch) noexcept { return !isspace(ch); }));
}
// trim from end (in place)
void trim_right(string* s)
{
  s->erase(find_if(s->rbegin(), s->rend(), [](const int ch) noexcept { return !isspace(ch); }).base(), s->end());
}
// trim from both ends (in place)
void trim(string* s)
{
  trim_left(s);
  trim_right(s);
}
// trim from start (copying)
string trim_left_copy(string s)
{
  trim_left(&s);
  return s;
}
// trim from end (copying)
string trim_right_copy(string s)
{
  trim_right(&s);
  return s;
}
// trim from both ends (copying)
string trim_copy(string s)
{
  trim(&s);
  return s;
}
}
