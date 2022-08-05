#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

class IPIN {
 public:
  IPIN() = default;
  IPIN(IPIN const&) = default;
  IPIN(IPIN&&) = default;
  IPIN& operator=(IPIN const&) = default;
  IPIN& operator=(IPIN&&) = default;
  virtual ~IPIN();

  virtual int use_pin() = 0;
};

// NOLINTNEXTLINE
IPIN::~IPIN() {}

template <typename PIN_MOSI, uint8_t MOSI>
class PIN : public IPIN {
 public:
  PIN() = default;
  PIN(PIN const&) = default;
  PIN(PIN&&) noexcept = default;
  PIN& operator=(PIN const&) = default;
  PIN& operator=(PIN&&) noexcept = default;
  ~PIN() override = default;

  int use_pin() override { return 1; }
};

class IIO {
 public:
  IIO() = default;
  IIO(IIO const&) = default;
  IIO(IIO&&) = default;
  IIO& operator=(IIO const&) = default;
  IIO& operator=(IIO&&) = default;
  virtual ~IIO();

  virtual int use_io() = 0;
};

// NOLINTNEXTLINE
IIO::~IIO() {}

class SPI : public IIO {
 public:
  using PinsPtrVector = std::vector<std::unique_ptr<IPIN>>;

  explicit SPI(PinsPtrVector&& pins) : mPins(std::move(pins)) {}
  SPI(SPI const&) = default;
  SPI(SPI&&) = default;
  SPI& operator=(SPI const&) = default;
  SPI& operator=(SPI&&) = default;
  ~SPI() override = default;

  int use_io() override;

 private:
  PinsPtrVector mPins;
};

int SPI::use_io() {
  int res = 0;
  for (auto& pin : mPins) {
    res += pin->use_pin();
  }
  return res;
}

class AdcDriver {
 public:
  using PinsPtrVector = std::vector<std::unique_ptr<IPIN>>;

  explicit AdcDriver(std::unique_ptr<IIO> interface, PinsPtrVector&& pins)
      : mIo(std::move(interface)), mPins(std::move(pins)) {}

  int use() {
    int res = mIo->use_io();
    for (auto& pin : mPins) {
      res += pin->use_pin();
    }
    return res;
  }

 private:
  std::unique_ptr<IIO> mIo;
  std::vector<std::unique_ptr<IPIN>> mPins;
};

class ADC {
 public:
  explicit ADC(std::unique_ptr<AdcDriver> driver) : mDriver(std::move(driver)) {}

  int use() { return mDriver->use(); }

 private:
  std::unique_ptr<AdcDriver> mDriver;
};

int main() {
  using PINMOSI = int;
  using PINRESET = int;
  static constexpr uint8_t value_mosi = 16;
  static constexpr uint8_t value_reset = 200;

  SPI::PinsPtrVector spi_pins;
  spi_pins.push_back(std::make_unique<PIN<PINMOSI, value_mosi>>());
  spi_pins.push_back(std::make_unique<PIN<PINRESET, value_reset>>());

  SPI::PinsPtrVector adc_pins;
  adc_pins.push_back(std::make_unique<PIN<PINMOSI, value_mosi>>());

  auto spi_interface = std::make_unique<SPI>(std::move(spi_pins));
  auto adc_driver =
      std::make_unique<AdcDriver>(std::move(spi_interface), std::move(adc_pins));
  auto adc = std::make_unique<ADC>(std::move(adc_driver));

  int res = adc->use();
  (void)res;
  return 0;
}
