/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Weather.h"
#include "Log.h"
namespace tbd::wx
{
const Temperature Temperature::Zero = Temperature(0);
const RelativeHumidity RelativeHumidity::Zero = RelativeHumidity(0);
const Direction Direction::Zero = Direction(0, false);
const Speed Speed::Zero = Speed(0);
const Wind Wind::Zero = Wind(Direction::Zero, Speed::Zero);
const Precipitation Precipitation::Zero = Precipitation(0);
const Temperature Temperature::Invalid = Temperature(-1);
const RelativeHumidity RelativeHumidity::Invalid = RelativeHumidity(-1);
const Direction Direction::Invalid = Direction(-1, false);
const Speed Speed::Invalid = Speed(-1);
const Wind Wind::Invalid = Wind(Direction::Invalid, Speed::Invalid);
const Precipitation Precipitation::Invalid = Precipitation(-1);
}
