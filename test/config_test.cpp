#include <gtest/gtest.h>

#include "config.h"

using namespace toml::literals::toml_literals;

TEST(config, Range_order) {
  config::Range<unsigned int> ordered_range(10, 12);
  EXPECT_EQ(ordered_range.lower_bound(), 10);
  EXPECT_EQ(ordered_range.upper_bound(), 12);

  config::Range<unsigned int> unordered_range(12, 10);
  EXPECT_EQ(unordered_range.lower_bound(), 10);
  EXPECT_EQ(unordered_range.upper_bound(), 12);
}

TEST(config, Range_contains) {
  config::Range<unsigned int> ref_range(10, 13);

  // a range contains itself
  EXPECT_TRUE(ref_range.contains(ref_range));
  // it contains a sub range
  EXPECT_TRUE(ref_range.contains(config::Range<unsigned int>(11, 12)));
  // it contains a sub range also with same lower bound
  EXPECT_TRUE(ref_range.contains(config::Range<unsigned int>(10, 12)));
  // it contains a sub range also with same upper bound
  EXPECT_TRUE(ref_range.contains(config::Range<unsigned int>(11, 13)));
  // it does not when the the upper bound is bigger
  EXPECT_FALSE(ref_range.contains(config::Range<unsigned int>(11, 14)));
  // it does not when the the lower bound is smaller
  EXPECT_FALSE(ref_range.contains(config::Range<unsigned int>(9, 12)));
  // compare range is bigger, but includes ref range
  EXPECT_FALSE(ref_range.contains(config::Range<unsigned int>(9, 14)));
  // compare range is outside of ref range
  EXPECT_FALSE(ref_range.contains(config::Range<unsigned int>(1, 2)));
  // compare range is outside of ref range
  EXPECT_FALSE(ref_range.contains(config::Range<unsigned int>(14, 15)));
}

TEST(config, SpectrumSlice_range) {
  config::SpectrumSlice<unsigned int> slice(/*center_frequency=*/1000, /*sample_rate=*/200);
  EXPECT_EQ(slice.center_frequency_, 1000);
  EXPECT_EQ(slice.frequency_range_.lower_bound(), 900);
  EXPECT_EQ(slice.frequency_range_.upper_bound(), 1100);
  EXPECT_EQ(slice.sample_rate_, 200);
}

TEST(config, TopLevel_decimate_not_divisible) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 1000000
		
		[DecimateA]
		Frequency = 4250000
		SampleRate = 500001
	)"_toml;

  // Input sample rate is not divisible by Decimate block sample rate.
  EXPECT_THROW(toml::get<config::TopLevel>(config_object), std::invalid_argument);
}

TEST(config, TopLevel_stream_not_divisible) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 60000
		
		[Stream0]
		Frequency = 4000000
	)"_toml;

  // Input sample rate is not divisible by Stream block sample rate.
  // 60000 is not divisible by default 25000 TETRA stream sample rate
  EXPECT_THROW(toml::get<config::TopLevel>(config_object), std::invalid_argument);
}

TEST(config, TopLevel_decimate_under_decimate) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 60000
		
		[DecmiateA]
		Frequency = 4250000
		SampleRate = 500000

		[DecmiateA.DecimateB]
		Frequency = 4250000
		SampleRate = 250000
	)"_toml;

  // Did not find a Stream block under the Decimate block
  EXPECT_THROW(toml::get<config::TopLevel>(config_object), std::invalid_argument);
}

TEST(config, TopLevel_prometheus_default) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 60000
		
		[Prometheus]
	)"_toml;

  const config::TopLevel t = toml::get<config::TopLevel>(config_object);

  // prometheus is set
  EXPECT_TRUE(t.prometheus_);

  EXPECT_EQ(t.prometheus_->host_, config::kDefaultPrometheusHost);
  EXPECT_EQ(t.prometheus_->port_, config::kDefaultPrometheusPort);
}

TEST(config, TopLevel_prometheus_set) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 60000
		
		[Prometheus]
		Host = "127.0.0.2"
		Port = 4200
	)"_toml;

  const config::TopLevel t = toml::get<config::TopLevel>(config_object);

  // prometheus is set
  EXPECT_TRUE(t.prometheus_);

  EXPECT_EQ(t.prometheus_->host_, "127.0.0.2");
  EXPECT_EQ(t.prometheus_->port_, 4200);
}

TEST(config, TopLevel_valid_parser) {
  const toml::value config_object = u8R"(
TEST(config, TopLevel_valid_parser) {
  const toml::value config_object = u8R"(
		CenterFrequency = 4000000
		DeviceString = "device_string_abc"
		SampleRate = 1000000
		IFGain = 14
		
		[DecimateA]
		Frequency = 4250000
		SampleRate = 500000
		
		[DecimateA.Stream0]
		Frequency = 4250010
		Host = "127.0.0.1"
		Port = 4100
		
		[DecimateA.Stream1]
		Frequency = 4250100
		
		[Stream2]
		Frequency = 4000100
		Host = "127.0.0.2"
		Port = 4200
	)"_toml;

  const config::TopLevel t = toml::get<config::TopLevel>(config_object);

  EXPECT_EQ(t.spectrum_.center_frequency_, 4000000);
  EXPECT_EQ(t.spectrum_.sample_rate_, 1000000);
  EXPECT_EQ(t.device_string_, "device_string_abc");
  EXPECT_EQ(t.rf_gain_, 0);
  EXPECT_EQ(t.if_gain_, 14);
  EXPECT_EQ(t.bb_gain_, 0);

  // prometheus is not set
  EXPECT_FALSE(t.prometheus_);

  EXPECT_EQ(t.decimators_.size(), 1);
  const auto& decimate_a = t.decimators_[0];

  EXPECT_EQ(decimate_a.spectrum_.center_frequency_, 4250000);
  EXPECT_EQ(decimate_a.spectrum_.sample_rate_, 500000);
  EXPECT_EQ(decimate_a.decimation_, 2);

  EXPECT_EQ(decimate_a.streams_.size(), 2);
  const auto& stream_0 = decimate_a.streams_[1];
  const auto& stream_1 = decimate_a.streams_[0];

  EXPECT_EQ(stream_0.spectrum_.center_frequency_, 4250010);
  EXPECT_EQ(stream_0.spectrum_.sample_rate_, config::kTetraSampleRate);
  EXPECT_EQ(stream_0.host_, "127.0.0.1");
  EXPECT_EQ(stream_0.port_, 4100);
  EXPECT_EQ(stream_0.decimation_, 20);

  EXPECT_EQ(stream_1.spectrum_.center_frequency_, 4250100);
  EXPECT_EQ(stream_1.spectrum_.sample_rate_, config::kTetraSampleRate);
  EXPECT_EQ(stream_1.host_, config::kDefaultHost);
  EXPECT_EQ(stream_1.port_, config::kDefaultPort);
  EXPECT_EQ(stream_1.decimation_, 20);

  EXPECT_EQ(t.streams_.size(), 1);
  const auto& stream_2 = t.streams_[0];

  EXPECT_EQ(stream_2.spectrum_.center_frequency_, 4000100);
  EXPECT_EQ(stream_2.spectrum_.sample_rate_, config::kTetraSampleRate);
  EXPECT_EQ(stream_2.host_, "127.0.0.2");
  EXPECT_EQ(stream_2.port_, 4200);
  EXPECT_EQ(stream_2.decimation_, 40);
}
