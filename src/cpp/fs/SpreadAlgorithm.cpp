/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SpreadAlgorithm.h"
#include "FireSpread.h"
#include "FuelType.h"
#include "unstable.h"
#include "Util.h"
namespace fs
{
using fs::fuel::fire_intensity;
using fs::fuel::ROSOffset;
HorizontalAdjustment horizontal_adjustment(const AspectSize slope_azimuth, const SlopeSize slope)
{
  // do everything we can to avoid calling trig functions unnecessarily
  constexpr auto no_correction = [](const Radians&) noexcept { return 1.0; };
  if (0 == slope)
  {
    // do check once and make function just return 1.0 if no slope
    return no_correction;
  }
  const auto b_semi = cos(atan(slope / 100.0));
  const Radians slope_radians{Degrees{slope_azimuth}};
  const auto do_correction = [=](const Radians& theta) noexcept {
    // never gets called if isInvalid() so don't check
    // figure out how far the ground distance is in map distance horizontally
    const auto angle_unrotated = theta - slope_radians;
    // if (to_degrees(angle_unrotated) == 270 || to_degrees(angle_unrotated) == 90)
    // {
    //   // CHECK: if we're going directly across the slope then horizontal distance is same as
    //   spread
    //   // distance
    //   return 1.0;
    // }
    const auto tan_u = tan(angle_unrotated);
    const auto y = b_semi / sqrt(b_semi * tan_u * (b_semi * tan_u) + 1.0);
    const auto x = y * tan_u;
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  return do_correction;
}
[[nodiscard]] OffsetSet OriginalSpreadAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor,
  MathSize tfc,
  const Radians& head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth
) const noexcept
{
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize ros) {
    if (ros < min_ros_)
    {
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = static_cast<IntensitySize>(fire_intensity(tfc, ros));
    // spreading, so figure out offset from current point
    offsets.push_back(ROSOffset{
      intensity,
      ros,
      direction.asDegrees(),
      Offset{
        static_cast<XYSize>(ros_cell * sin(direction)),
        static_cast<XYSize>(ros_cell * cos(direction))
      }
    });
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    return offsets;
  }
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [=](const Radians& theta) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    const auto sin_t_sq = 1.0 - cos_t_sq;
    // // or could be: 1.0 = cos^2 + sin^2
    // const auto sin_t = sin(theta);
    // const auto sin_t_sq = sin_t * sin_t;
    // or could be: 1.0 = cos^2 + sin^2
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&, head_raz](const Radians& angle_radians, const MathSize ros_flat) {
    if (ros_flat < min_ros_)
    {
      return false;
    }
    auto direction = (Radians{angle_radians} + head_raz).fix();
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = (head_raz - Radians{angle_radians}).fix();
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&](const Radians& angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = add_offset(head_raz, head_ros);
  MathSize i = max_angle_;
  while (added && i < 90)
  {
    added = add_offsets_calc_ros(Radians::from_degrees(i));
    i += max_angle_;
  }
  if (added)
  {
    added = add_offsets(Radians::D_090(), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    i = 90 + max_angle_;
    while (added && i < 180)
    {
      added = add_offsets_calc_ros(Radians::from_degrees(i));
      i += max_angle_;
    }
    if (added)
    {
      // only use back ros if every other angle is spreading since this should be lowest
      //  180
      if (back_ros >= min_ros_)
      {
        const auto direction{Radians{head_raz}.to_heading().fix()};
        static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
      }
    }
  }
  return offsets;
}
[[nodiscard]] OffsetSet WidestEllipseAlgorithm::calculate_offsets(
  const HorizontalAdjustment correction_factor,
  const MathSize tfc,
  const Radians& head_raz,
  const MathSize head_ros,
  const MathSize back_ros,
  const MathSize length_to_breadth
) const noexcept
{
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize ros) {
#ifdef DEBUG_POINTS
    const auto s0 = offsets.size();
#endif
    if (ros < min_ros_)
    {
      // #endif
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = static_cast<IntensitySize>(fire_intensity(tfc, ros));
    // spreading, so figure out offset from current point
    offsets.push_back(ROSOffset{
      intensity,
      ros,
      Direction{direction},
      Offset{
        static_cast<XYSize>(ros_cell * sin(direction)),
        static_cast<XYSize>(ros_cell * cos(direction))
      }
    });
#ifdef DEBUG_POINTS
    const auto s1 = offsets.size();
    logging::check_equal(s0 + 1, s1, "offsets.size()");
    logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    // #endif
    return offsets;
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [=](const Radians& theta) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    // or could be: 1.0 = cos^2 + sin^2
    const auto sin_t = sin(theta);
    const auto sin_t_sq = sin_t * sin_t;
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&, head_raz](const Radians& angle_radians, const MathSize ros_flat) {
    if (ros_flat < min_ros_)
    {
      return false;
    }
    auto direction = (Radians{angle_radians} + head_raz).fix();
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = (head_raz - Radians{angle_radians}).fix();
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&](const Radians& angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = true;
#define STEP_X 0.2
#define STEP_MAX Radians::from_degrees(max_angle_)
  MathSize step_x = STEP_X / pow(length_to_breadth, 0.5);
  Radians theta{0.0};
  Radians angle{0.0};
  Radians last_theta{0.0};
  MathSize cur_x = 1.0;
  // widest point should be at origin, which is 'c' away from origin
  MathSize widest = atan2(flank_ros, c);
  // size_t num_angles = 0;
  MathSize widest_x = cos(widest);
  Radians step_max{STEP_MAX / pow(length_to_breadth, 0.5)};
  while (added && cur_x > (STEP_MAX / 4.0).value)
  {
    // ++num_angles;
    theta = min(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    if (theta > (STEP_MAX / 2.0))
    {
      step_max = STEP_MAX;
    }
    cur_x -= step_x;
    if (cur_x > widest_x && abs(cur_x - widest_x) < step_x)
    {
      cur_x = widest_x;
    }
  }
  if (added)
  {
    angle = ellipse_angle(length_to_breadth, Radians{(Radians::D_090() + theta) / 2.0});
    added = add_offsets_calc_ros(angle);
    // always just do one between the last angle and 90
    theta = Radians::D_090();
    // ++num_angles;
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets(Radians::D_090(), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    cur_x = cos(theta);
    last_theta = theta;
  }
  cur_x -= (step_x / 2.0);
  step_x *= length_to_breadth;
  Radians max_angle{Radians::D_180() - (length_to_breadth * step_max)};
  MathSize min_x = cos(max_angle);
  while (added && cur_x >= min_x)
  {
    // ++num_angles;
    theta = max(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    if (angle > max_angle)
    {
      break;
    }
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    cur_x -= step_x;
  }
  if (added)
  {
    // only use back ros if every other angle is spreading since this should be lowest
    //  180
    if (back_ros >= min_ros_)
    {
      const auto direction{Radians{head_raz}.to_heading().fix()};
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
#ifdef DEBUG_POINTS
  if (head_ros >= min_ros_)
  {
    logging::check_fatal(offsets.empty(), "Empty when ros of {:f} >= {:f}", head_ros, min_ros_);
  }
#endif
  return offsets;
}
}
