/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SPREAD_ALGORITHM_H
#define FS_SPREAD_ALGORITHM_H
#include "../stdafx.h"
#include "FireSpread.h"
namespace fs
{
using HorizontalAdjustment = std::function<MathSize(const Radians&)>;
HorizontalAdjustment horizontal_adjustment(const AspectSize slope_azimuth, const SlopeSize slope);
class SpreadAlgorithm
{
public:
  virtual ~SpreadAlgorithm() = default;
  [[nodiscard]] virtual OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    MathSize tfc,
    const Radians& head_raz,
    MathSize head_ros,
    MathSize back_ros,
    MathSize length_to_breadth
  ) const noexcept = 0;
};
class BaseSpreadAlgorithm : public SpreadAlgorithm
{
public:
  BaseSpreadAlgorithm(const MathSize max_angle, const MathSize cell_size, const MathSize min_ros)
    : max_angle_(max_angle), cell_size_(cell_size), min_ros_(min_ros)
  { }

protected:
  MathSize max_angle_;
  MathSize cell_size_;
  MathSize min_ros_;
};
class OriginalSpreadAlgorithm : public BaseSpreadAlgorithm
{
public:
  using BaseSpreadAlgorithm::BaseSpreadAlgorithm;
  [[nodiscard]] OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    MathSize tfc,
    const Radians& head_raz,
    MathSize head_ros,
    MathSize back_ros,
    MathSize length_to_breadth
  ) const noexcept override;
};
class WidestEllipseAlgorithm : public BaseSpreadAlgorithm
{
public:
  using BaseSpreadAlgorithm::BaseSpreadAlgorithm;
  [[nodiscard]] OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    MathSize tfc,
    const Radians& head_raz,
    MathSize head_ros,
    MathSize back_ros,
    MathSize length_to_breadth
  ) const noexcept override;
};
}
#endif
