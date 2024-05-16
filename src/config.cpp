#include "config.h"

namespace config {

Stream::Stream(const SpectrumSlice<unsigned int>& input_spectrum, const SpectrumSlice<unsigned int>& spectrum,
               std::string host, unsigned int port)
    : input_spectrum_(input_spectrum)
    , spectrum_(spectrum)
    , host_(std::move(host))
    , port_(port) {
  // check that this Stream is valid
  if (!input_spectrum.frequency_range_.contains(spectrum.frequency_range_)) {
    throw std::invalid_argument("Frequency Range of the Streams in not "
                                "inside the frequency Range of the input.");
  }

  const auto& input_sample_rate = input_spectrum.sample_rate_;
  const auto& sample_rate = spectrum.sample_rate_;
  decimation_ = input_sample_rate / sample_rate;
  auto remainder = input_sample_rate % sample_rate;
  if (remainder != 0) {
    throw std::invalid_argument("Input sample rate is not divisible by Stream block sample rate.");
  }
}

Decimate::Decimate(const SpectrumSlice<unsigned int>& input_spectrum, const SpectrumSlice<unsigned int>& spectrum)
    : input_spectrum_(input_spectrum)
    , spectrum_(spectrum) {
  // check that this Stream is valid
  if (!input_spectrum.frequency_range_.contains(spectrum.frequency_range_)) {
    throw std::invalid_argument("Decimator frequency Range is not inside the one of the SDR");
  }

  const auto& input_sample_rate = input_spectrum.sample_rate_;
  const auto& sample_rate = spectrum.sample_rate_;
  decimation_ = input_sample_rate / sample_rate;
  auto remainder = input_sample_rate % sample_rate;

  if (remainder != 0) {
    throw std::invalid_argument("Input sample rate is not divisible by Decimate block sample rate.");
  }

  for (const auto& stream : streams_) {
    if (stream.input_spectrum_ != spectrum) {
      throw std::invalid_argument("The output of Decimate does not match to the input of Stream.");
    }
  }
}

TopLevel::TopLevel(const SpectrumSlice<unsigned int>& spectrum, std::string device_string, const unsigned int rf_gain,
                   const unsigned int if_gain, const unsigned int bb_gain, std::string prometheus_host, unsigned short prometheus_port,
                   const std::vector<Stream>& streams, const std::vector<Decimate>& decimators)
    : spectrum_(spectrum)
    , device_string_(std::move(device_string))
    , rf_gain_(rf_gain)
    , if_gain_(if_gain)
    , bb_gain_(bb_gain)
    , prometheus_host_(std::move(prometheus_host))
    , prometheus_port_(prometheus_port)
    , streams_(streams)
    , decimators_(decimators) {
  for (const auto& stream : streams) {
    if (operator!=<unsigned int>(stream.input_spectrum_, spectrum)) {
      throw std::invalid_argument("The output of Decimate does not match to the input of Stream.");
    }
  }
  for (const auto& decimator : decimators) {
    if (operator!=<unsigned int>(decimator.input_spectrum_, spectrum)) {
      throw std::invalid_argument("The output of Decimate does not match to the input of Stream.");
    }
  }
}

} // namespace config
