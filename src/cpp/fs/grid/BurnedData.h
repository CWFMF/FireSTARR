/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_BURNED_DATA_H
#define FS_BURNED_DATA_H
#include "../stdafx.h"
#include "../types/Location.h"
#include "ConstantGrid.h"
namespace fs
{
class BurnedData
{
public:
  BurnedData(const CellGrid& cells) noexcept;
  BurnedData(const FuelGrid& fuel) noexcept;
  bool at(const XYIdx& h) const noexcept;
  void set(const XYIdx& h) noexcept;
  void clear() noexcept;
  BurnedData() noexcept = default;
  BurnedData(const BurnedData& rhs) noexcept;
  BurnedData(BurnedData&& rhs) noexcept;
  BurnedData& operator=(const BurnedData& rhs) noexcept;
  BurnedData& operator=(BurnedData&& rhs) noexcept;
  [[nodiscard]] constexpr Idx height() const noexcept { return height_; }
  [[nodiscard]] constexpr Idx width() const noexcept { return width_; }
  /**
   * \brief Calculate area for cells that have a value (ha)
   * \return Area for cells that have a value (ha)
   */
  [[nodiscard]] MathSize fireSize() const noexcept
  {
    // we know that every cell is a key, so we convert that to hectares
    const MathSize per_width = (cell_size_ / 100.0);
    // size of fire is number of bits set * cell size
    return static_cast<MathSize>(data_->count() - unburnable_) * per_width * per_width;
  }

private:
  // std::bitset doesn't heap allocate
  using dtype = std::bitset<static_cast<size_t>(MAX_HEIGHT) * MAX_WIDTH>;
  uptr<dtype> data_{nullptr};
  static uptr<dtype> from_grid(auto& grid, auto fct);
  MathSize cell_size_{-1};
  size_t unburnable_{0};
  Idx height_{};
  Idx width_{};
};
}
#endif
