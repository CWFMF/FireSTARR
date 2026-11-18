/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_MOISTURE_H
#define FS_MOISTURE_H
#include "FireWeatherIndices.h"
namespace fs
{
MathSize ffmc_effect(const Ffmc ffmc) noexcept;
MathSize ffmc_to_moisture(const MathSize ffmc) noexcept;
MathSize ffmc_to_moisture(const Ffmc& ffmc) noexcept;
Ffmc moisture_to_ffmc(const MathSize m) noexcept;
Ffmc ffmc_from_moisture(const MathSize m) noexcept;
}
#endif
