/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_MONTH_H
#define FS_MONTH_H
#include <cstddef>
namespace fs
{
// months as array indexes
class Month
{
public:
  enum class Value
  {
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December
  };
  static Month from_index(const int value) { return {static_cast<Value>(value)}; }
  static Month from_ordinal(const int value) { return {static_cast<Value>(value - 1)}; }
  Month(const Value value) : value{value} { }
  int ordinal() const { return static_cast<int>(value) + 1; }
  size_t index() const { return static_cast<size_t>(value); }
  const char* name() const
  {
    static constexpr const char* NAMES[]{
      "January",
      "February",
      "March",
      "April",
      "May",
      "June",
      "July",
      "August",
      "September",
      "October",
      "November",
      "December"
    };
    return NAMES[index()];
  }

private:
  Value value;
};
}
#endif
