#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <optional>
#include <toml.hpp>
#include <variant>

#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

namespace config {

/// The sample rate of the TETRA Stream
[[maybe_unused]] static constexpr unsigned int kTetraSampleRate = 25000;

/// The default host where to send the TETRA data. Gnuradio breaks if this is a host and not an IP
[[maybe_unused]] const std::string kDefaultHost = "127.0.0.1";
[[maybe_unused]] constexpr unsigned int kDefaultPort = 42000;

template <typename T> class Range {
private:
  T min_ = 0;
  T max_ = 0;

public:
  /// A class takes two values of type T and stores the minimum value in min_,
  /// the maximum in max_ respectively.
  Range(const T lhs, const T rhs) noexcept {
    if (lhs < rhs) {
      this->min_ = lhs;
      this->max_ = rhs;
    } else {
      this->min_ = rhs;
      this->max_ = lhs;
    }
  };

  [[nodiscard]] auto lower_bound() const noexcept -> T { return min_; };
  [[nodiscard]] auto upper_bound() const noexcept -> T { return max_; };

  /// Check that the given Range is inside the bounds (inclusive) of this Range.
  [[nodiscard]] auto contains(const Range<T>& other) const noexcept -> bool {
    return !static_cast<bool>(other.min_ < min_ || other.max_ > max_);
  };
};

template <typename T> class SpectrumSlice {
public:
  /// the center frequency of this slice of EM spectrum
  T center_frequency_;
  /// the frequency Range of this slice of EM spectrum
  Range<T> frequency_range_;
  /// the sampling frequency for this slice of EM spectrum
  T sample_rate_;

  /// Get different properties for a slice of EM spectrum
  /// \param frequency the center frequency of the slice
  /// \param sample_rate the sample rate of this spectrum
  SpectrumSlice(const T center_frequency, const T sample_rate) noexcept
      : center_frequency_(center_frequency)
      , frequency_range_(center_frequency - sample_rate / 2, center_frequency + sample_rate / 2)
      , sample_rate_(sample_rate){};

  friend auto operator==(const SpectrumSlice<T>&, const SpectrumSlice<T>&) -> bool;
  friend auto operator!=(const SpectrumSlice<T>&, const SpectrumSlice<T>&) -> bool;
};

template <typename T> auto operator==(const SpectrumSlice<T>& lhs, const SpectrumSlice<T>& rhs) -> bool {
  return lhs.center_frequency_ == rhs.center_frequency_ && lhs.sample_rate_ == rhs.sample_rate_;
};

template <typename T> auto operator!=(const SpectrumSlice<T>& lhs, const SpectrumSlice<T>& rhs) -> bool {
  return !operator==<T>(lhs, rhs);
};

class Stream {
public:
  /// the slice of spectrum_ that is input to this block
  const SpectrumSlice<unsigned int> input_spectrum_;
  /// the slice of spectrum_ of the TETRA Stream
  const SpectrumSlice<unsigned int> spectrum_;
  /// the decimation_ of this block
  unsigned int decimation_ = 0;
  /// Optional field
  /// The host to which the samples of the Stream should be sent. This defaults
  /// to "locahost".
  const std::string host_{};
  /// Optional field
  /// The port to which the samples of the Stream should be sent. This defaults
  /// to 42000.
  const unsigned int port_ = 0;

  /// Describe the on which frequency a TETRA Stream should be extracted and
  /// where data should be sent to.
  /// \param input_spectrum the slice of spectrum that is input to this block
  /// \param spectrum the slice of spectrum of the TETRA Stream
  /// \param host the to send the data to
  /// \param port the port to send the data to
  Stream(const SpectrumSlice<unsigned int>& input_spectrum, const SpectrumSlice<unsigned int>& spectrum,
         std::string host, unsigned int port);
};

class Decimate {
public:
  /// the slice of spectrum that is input to this block
  const SpectrumSlice<unsigned int> input_spectrum_;
  /// the slice of spectrum after decimation_
  const SpectrumSlice<unsigned int> spectrum_;

  /// the decimation of this block
  unsigned int decimation_ = 0;

  /// The vector of streams the output of this Decimate block should be
  /// connected to.
  std::vector<Stream> streams_;

  /// Describe the decimation of the SDR Stream by the frequency where we want
  /// to extract a signal with a width of sample_rate
  /// \param input_spectrum the slice of spectrum that is input to this block
  /// \param spectrum the slice of spectrum after decimation
  /// \param streams the vector of streams the decimated signal should be sent
  /// to
  Decimate(const SpectrumSlice<unsigned int>& input_spectrum, const SpectrumSlice<unsigned int>& spectrum);
};

class TopLevel {
public:
  /// The spectrum of the SDR
  const SpectrumSlice<unsigned int> spectrum_;
  /// The device string for the SDR source block
  const std::string device_string_{};
  /// The RF gain setting of the SDR
  const unsigned int rf_gain_;
  /// The IF gain setting of the SDR
  const unsigned int if_gain_;
  /// The BB gain setting of the SDR
  const unsigned int bb_gain_;
  /// The vector of Streams which should be directly decoded from the input of
  /// the SDR.
  const std::vector<Stream> streams_{};
  /// The vector of decimators which should first Decimate a signal of the SDR
  /// and then sent it to the vector of streams inside them.
  const std::vector<Decimate> decimators_{};

  TopLevel(const SpectrumSlice<unsigned int>& spectrum, std::string device_string, unsigned int rf_gain,
           unsigned int if_gain, unsigned int bb_gain, const std::vector<Stream>& streams,
           const std::vector<Decimate>& decimators);
};

using decimate_or_stream = std::variant<Decimate, Stream>;

}; // namespace config

namespace toml {

static config::decimate_or_stream get_decimate_or_stream(const config::SpectrumSlice<unsigned int>& input_spectrum,
                                                         const value& v) {
  std::optional<unsigned int> sample_rate;

  const unsigned int frequency = find<unsigned int>(v, "Frequency");
  if (v.contains("SampleRate"))
    sample_rate = find<unsigned int>(v, "SampleRate");

  const std::string host = find_or(v, "Host", config::kDefaultHost);
  const unsigned int port = find_or(v, "Port", config::kDefaultPort);

  // If we have a sample rate specified this is a Decimate, otherwhise this is
  // a Stream.
  if (sample_rate.has_value()) {
    return config::Decimate(input_spectrum, config::SpectrumSlice<unsigned int>(frequency, *sample_rate));
  } else {
    return config::Stream(input_spectrum, config::SpectrumSlice<unsigned int>(frequency, config::kTetraSampleRate),
                          host, port);
  }
}

template <> struct from<config::TopLevel> {
  static auto from_toml(const value& v) -> config::TopLevel {
    const unsigned int center_frequency = find<unsigned int>(v, "CenterFrequency");
    const std::string device_string = find<std::string>(v, "DeviceString");
    const unsigned int sample_rate = find<unsigned int>(v, "SampleRate");
    const unsigned int rf_gain = find_or(v, "RFGain", 0);
    const unsigned int if_gain = find_or(v, "IFGain", 0);
    const unsigned int bb_gain = find_or(v, "BBGain", 0);

    config::SpectrumSlice<unsigned int> sdr_spectrum(center_frequency, sample_rate);

    std::vector<config::Stream> streams;
    std::vector<config::Decimate> decimators;

    // Iterate over all elements in the root table
    for (const auto& root_kv : v.as_table()) {
      const auto& table = root_kv.second;

      // Find table entries. These can be decimators or streams.
      if (!table.is_table())
        continue;

      const auto element = get_decimate_or_stream(sdr_spectrum, table);

      // Save the Stream
      if (std::holds_alternative<config::Stream>(element)) {
        const auto& stream_element = std::get<config::Stream>(element);
        streams.push_back(stream_element);

        continue;
      }

      // Found a decimator entry
      if (std::holds_alternative<config::Decimate>(element)) {
        auto decimate_element = std::get<config::Decimate>(element);

        // Find all subtables, that must be Stream entries and add them to the
        // decimator
        for (const auto& stream_pair : table.as_table()) {
          auto& stream_table = stream_pair.second;

          if (!stream_table.is_table())
            continue;

          const auto stream_element = get_decimate_or_stream(decimate_element.spectrum_, stream_table);

          if (!std::holds_alternative<config::Stream>(stream_element)) {
            throw std::invalid_argument("Did not find a Stream block under the Decimate block");
          }

          decimate_element.streams_.push_back(std::get<config::Stream>(stream_element));
        }

        decimators.push_back(decimate_element);

        continue;
      }

      throw std::invalid_argument("Did not handle a derived type of decimate_or_stream");
    }

    return config::TopLevel(sdr_spectrum, device_string, rf_gain, if_gain, bb_gain, streams, decimators);
  }
};

}; // namespace toml

#endif

#endif
