/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Startup.h"
namespace tbd::wx
{
Startup::Startup(string station,
                 const tm& generated,
                 const topo::Point& point,
                 const double distance_from,
                 const Ffmc& ffmc,
                 const Dmc& dmc,
                 const Dc& dc,
                 const Precipitation& apcp_prev,
                 const bool overridden) noexcept
  : station_(std::move(station)),
    generated_(generated),
    point_(point),
    distance_from_(distance_from),
    ffmc_(ffmc),
    dmc_(dmc),
    dc_(dc),
    apcp_prev_(apcp_prev),
    is_overridden_(overridden)
{
}
}
