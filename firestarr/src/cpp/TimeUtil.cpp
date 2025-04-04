/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "TimeUtil.h"
namespace tbd::util
{
void fix_tm(tm* t)
{
  const time_t t_t = mktime(t);
  t = localtime(&t_t);
}
}
