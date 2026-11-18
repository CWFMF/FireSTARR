/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FWI_WEATHER_H
#define FS_FWI_WEATHER_H
#include "../unstable.h"
#include "FireWeatherIndices.h"
namespace fs
{
/**
 * \brief A Weather value with calculated FWI indices.
 */
struct FwiWeatherImpl : public Weather
{
  static consteval FwiWeatherImpl Zero() { return {}; }
  static consteval FwiWeatherImpl Invalid()
  {
    return {
      Weather::Invalid(),
      Ffmc::Invalid(),
      Dmc::Invalid(),
      Dc::Invalid(),
      Isi::Invalid(),
      Bui::Invalid(),
      Fwi::Invalid()
    };
  }
  /**
   * \brief Fine Fuel Moisture Code
   */
  Ffmc ffmc{};
  /**
   * \brief Duff Moisture Code
   */
  Dmc dmc{};
  /**
   * \brief Drought Code
   */
  Dc dc{};
  /**
   * \brief Initial Spread Index
   */
  Isi isi{};
  /**
   * \brief Build-up Index
   */
  Bui bui{};
  /**
   * \brief Fire Weather Index
   */
  Fwi fwi{};
  constexpr FwiWeatherImpl() noexcept = default;
  constexpr FwiWeatherImpl(
    const Weather wx,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    Isi isi = Isi::Invalid(),
    Bui bui = Bui::Invalid(),
    Fwi fwi = Fwi::Invalid()
  ) noexcept
    : Weather(wx), ffmc{ffmc}, dmc{dmc}, dc{dc},
      isi{Isi::Invalid() == isi ? Isi{wind.speed, ffmc} : isi},
      bui{Bui::Invalid() == bui ? Bui{dmc, dc} : bui},
      fwi{Fwi::Invalid() == fwi ? Fwi{this->isi, this->bui} : fwi}
  { }
  constexpr FwiWeatherImpl(
    const Temperature temp,
    const RelativeHumidity rh,
    const Wind wind,
    const Precipitation prec,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    const Isi isi,
    const Bui bui,
    const Fwi fwi
  ) noexcept
    : FwiWeatherImpl{Weather{temp, rh, wind, prec}, ffmc, dmc, dc, isi, bui, fwi}
  { }
  constexpr FwiWeatherImpl(
    const FwiWeatherImpl& yesterday,
    const int month,
    const MathSize latitude,
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    Ffmc ffmc = Ffmc::Invalid(),
    Dmc dmc = Dmc::Invalid(),
    Dc dc = Dc::Invalid(),
    Isi isi = Isi::Invalid(),
    Bui bui = Bui::Invalid(),
    Fwi fwi = Fwi::Invalid()
  ) noexcept
    : FwiWeatherImpl(
        {.temperature = temp, .rh = rh, .wind = wind, .prec = prec},
        (Ffmc::Invalid() == ffmc) ? Ffmc{temp, rh, wind.speed, prec, yesterday.ffmc} : ffmc,
        (Dmc::Invalid() == dmc) ? Dmc{temp, rh, prec, yesterday.dmc, month, latitude} : dmc,
        (Dc::Invalid() == dc) ? Dc{temp, prec, yesterday.dc, month, latitude} : dc,
        isi,
        bui,
        fwi
      )
  { }
  auto operator<=>(const FwiWeatherImpl& rhs) const = default;
  /**
   * \brief Moisture content (%) based on Ffmc
   * \return Moisture content (%) based on Ffmc
   */
  [[nodiscard]] MathSize mcFfmcPct() const;
  /**
   * \brief Moisture content (%) based on Dmc
   * \return Moisture content (%) based on Dmc
   */
  [[nodiscard]] MathSize mcDmcPct() const;
  /**
   * \brief Moisture content (ratio) based on Ffmc
   * \return Moisture content (ratio) based on Ffmc
   */
  [[nodiscard]] MathSize mcFfmc() const;
  /**
   * \brief Moisture content (ratio) based on Dmc
   * \return Moisture content (ratio) based on Dmc
   */
  [[nodiscard]] MathSize mcDmc() const;
  /**
   * \brief Ffmc effect used for spread
   * \return Ffmc effect used for spread
   */
  [[nodiscard]] MathSize ffmcEffect() const;
};
class FwiWeather
{
private:
  static mutex mutex_;
  ptr<const FwiWeatherImpl> lookup(const FwiWeatherImpl& wx) noexcept
  {
    // keep unique FwiWeatherImpl and then just do pointer comparison for equality
    scoped_lock lock(mutex_);
    static set<FwiWeatherImpl> fwi_values{};
    static const FwiWeatherImpl empty{};
    if (empty == wx)
    {
      return nullptr;
    }
    auto e = fwi_values.emplace(wx);
    return &(*e.first);
  }

public:
  // static FwiWeather Zero() { return FwiWeather(FwiWeatherImpl::Zero()); }
  // static FwiWeather Invalid() { return FwiWeather(FwiWeatherImpl::Invalid()); }
  const Ffmc& ffmc() const noexcept { return impl_->ffmc; }
  const Dmc& dmc() const noexcept { return impl_->dmc; }
  const Dc& dc() const noexcept { return impl_->dc; }
  const Isi& isi() const noexcept { return impl_->isi; }
  const Bui& bui() const noexcept { return impl_->bui; }
  const Fwi& fwi() const noexcept { return impl_->fwi; }
  const Temperature& temperature() const noexcept { return impl_->temperature; };
  const RelativeHumidity& rh() const noexcept { return impl_->rh; };
  const Wind& wind() const noexcept { return impl_->wind; };
  const Precipitation& prec() const noexcept { return impl_->prec; };
  constexpr FwiWeather() noexcept = default;
  FwiWeather(const FwiWeatherImpl& wx) noexcept : impl_{FwiWeather::lookup(wx)} { }
  FwiWeather(const FwiWeather& rhs) noexcept : impl_{rhs.impl_} { }
  FwiWeather(FwiWeather&& rhs) noexcept : impl_{rhs.impl_} { }
  FwiWeather& operator=(const FwiWeather& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }
  FwiWeather& operator=(FwiWeather&& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }
  FwiWeather(
    const Weather wx,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    const Isi isi = Isi::Invalid(),
    const Bui bui = Bui::Invalid(),
    const Fwi fwi = Fwi::Invalid()
  ) noexcept
    : FwiWeather{FwiWeatherImpl{Weather(wx), ffmc, dmc, dc, isi, bui, fwi}}
  { }
  FwiWeather(
    const Temperature temp,
    const RelativeHumidity rh,
    const Wind wind,
    const Precipitation prec,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    // Isi isi = Isi::Invalid(),
    // Bui bui = Bui::Invalid(),
    // Fwi fwi = Fwi::Invalid()
    const Isi isi,
    const Bui bui,
    const Fwi fwi
  ) noexcept
    : FwiWeather{Weather{temp, rh, wind, prec}, ffmc, dmc, dc, isi, bui, fwi}
  { }
  FwiWeather(
    const FwiWeather& yesterday,
    const int month,
    const MathSize latitude,
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    Ffmc ffmc = Ffmc::Invalid(),
    Dmc dmc = Dmc::Invalid(),
    Dc dc = Dc::Invalid(),
    Isi isi = Isi::Invalid(),
    Bui bui = Bui::Invalid(),
    Fwi fwi = Fwi::Invalid()
  ) noexcept
    : FwiWeather{FwiWeatherImpl{
        *yesterday.impl_,
        month,
        latitude,
        temp,
        rh,
        wind,
        prec,
        ffmc,
        dmc,
        dc,
        isi,
        bui,
        fwi
      }}
  { }
  auto operator<=>(const FwiWeather& rhs) const { return *impl_ <=> *rhs.impl_; }
  auto operator==(const FwiWeather& rhs) const { return impl_ == rhs.impl_; }
  [[nodiscard]] MathSize mcFfmcPct() const { return impl_->mcFfmcPct(); }
  [[nodiscard]] MathSize mcDmcPct() const { return impl_->mcDmcPct(); }
  [[nodiscard]] MathSize mcFfmc() const { return impl_->mcFfmc(); }
  [[nodiscard]] MathSize mcDmc() const { return impl_->mcDmc(); }
  [[nodiscard]] MathSize ffmcEffect() const { return impl_->ffmcEffect(); }
  [[nodiscard]] bool isNull() const { return nullptr == impl_; }

private:
  ptr<const FwiWeatherImpl> impl_{nullptr};
};
}
#endif
