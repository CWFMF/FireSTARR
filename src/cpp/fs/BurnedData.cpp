/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "BurnedData.h"
namespace fs
{
using fs::fuel::fuel_by_code;
BurnedData::BurnedData(const CellGrid& cells) noexcept
  : data_{from_grid(cells, [](const auto& v) { return fuel_by_code(v.fuelCode()); })},
    cell_size_{cells.cellSize()}, unburnable_(data_->count()), height_{cells.height()},
    width_{cells.width()}
{ }
BurnedData::BurnedData(const FuelGrid& fuel) noexcept
  : data_{from_grid(fuel, [](const auto& v) { return v; })}, cell_size_{fuel.cellSize()},
    unburnable_(data_->count()), height_{fuel.height()}, width_{fuel.width()}
{ }
bool BurnedData::at(const XYIdx& xy) const noexcept { return (*data_)[to_index(xy)]; }
void BurnedData::set(const XYIdx& xy) noexcept { data_->set(to_index(xy)); }
void BurnedData::clear() noexcept
{
  if (nullptr == data_)
  {
    data_ = std::make_unique<dtype>(false);
  }
  else
  {
    data_->reset();
  }
  unburnable_ = data_->count();
}
BurnedData::BurnedData(const BurnedData& rhs) noexcept
  : data_{nullptr == rhs.data_ ? nullptr : make_unique<dtype>(*rhs.data_)},
    cell_size_{rhs.cell_size_}, unburnable_{rhs.unburnable_}, height_{rhs.height_},
    width_{rhs.width_}
{ }
BurnedData::BurnedData(BurnedData&& rhs) noexcept
  : data_(nullptr == rhs.data_ ? nullptr : std::move(rhs.data_)), cell_size_{rhs.cell_size_},
    unburnable_{rhs.unburnable_}, height_{rhs.height_}, width_{rhs.width_}
{
  rhs.data_ = nullptr;
  rhs.unburnable_ = 0;
  rhs.height_ = 0;
  rhs.width_ = 0;
}
BurnedData& BurnedData::operator=(const BurnedData& rhs) noexcept
{
  cell_size_ = rhs.cell_size_;
  unburnable_ = rhs.unburnable_;
  height_ = rhs.height_;
  width_ = rhs.width_;
  if (nullptr == rhs.data_)
  {
    data_ = nullptr;
    return *this;
  }
  if (nullptr == data_)
  {
    data_ = std::make_unique<dtype>();
  }
  *data_ = *rhs.data_;
  return *this;
}
BurnedData& BurnedData::operator=(BurnedData&& rhs) noexcept
{
  cell_size_ = rhs.cell_size_;
  unburnable_ = rhs.unburnable_;
  height_ = rhs.height_;
  width_ = rhs.width_;
  if (nullptr == rhs.data_)
  {
    data_ = nullptr;
    return *this;
  }
  data_ = std::move(rhs.data_);
  rhs.data_ = nullptr;
  rhs.unburnable_ = 0;
  rhs.height_ = 0;
  rhs.width_ = 0;
  return *this;
}
uptr<BurnedData::dtype> BurnedData::from_grid(auto& grid, auto fct)
{
  // FIX: change to use grid iterator
  auto result = make_unique<dtype>(false);
  // make a template we can copy to reset things
  for (Idx y = 0; y < grid.height(); ++y)
  {
    for (Idx x = 0; x < grid.width(); ++x)
    {
      const XYIdx xy{x, y};
      // HACK: just mark outside edge as unburnable so we never need to check
      bool is_outer = 0 == y || 0 == x || (grid.height() - 1) == y || (grid.width() - 1) == x;
      (*result)[to_index(xy)] = is_outer || (nullptr == fct(grid.at(xy)));
    }
  }
  return result;
}
}
