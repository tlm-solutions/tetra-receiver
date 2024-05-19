#include <gtest/gtest.h>
#include <gmock/gmock.h>

//#include <gnuradio/io_signature.h>
#include <prometheus/gauge.h>

#include "prometheus.h"
#include "prometheus_gauge_populator.h"

class PrometheusGaugePopulatorMock : public gr::prometheus::PrometheusGaugePopulator {
public:
  using sptr = boost::shared_ptr<PrometheusGaugePopulatorMock>;

  explicit PrometheusGaugePopulatorMock(::prometheus::Gauge& gauge)
      : PrometheusGaugePopulator(gauge) {}

  static auto make(::prometheus::Gauge& gauge) -> sptr {
    return gnuradio::get_initial_sptr(new PrometheusGaugePopulatorMock(gauge));
  };

  MOCK_METHOD(void, consume_each, (int));
};

TEST(config /*unused*/, CreatingPrometheusServer/*unused*/) {

  const std::string host = "127.0.0.1:25123";
  PrometheusExporter exporter(host);

  auto& signal_strength = exporter.signal_strength();
  auto labels = signal_strength.GetConstantLabels();
  auto name = signal_strength.GetName();

  ASSERT_EQ(name, "signal_strength");

  auto& stream_signal_strength = signal_strength.Add(
      {{"frequency", std::to_string(2.0)}, {"name", "test"}});

  testing::NiceMock<PrometheusGaugePopulatorMock> populator{/*gauge=*/stream_signal_strength};
  EXPECT_CALL(populator, consume_each);

  int noutput_items = 1;
  gr_vector_int ninput_items;
  ninput_items.resize(1);
  ninput_items[0] = 1;
  gr_vector_const_void_star input_items;
  float vec[] = {60.0};
  input_items.resize(1);
  input_items[0] = &vec;
  gr_vector_void_star output_items;

  populator.general_work(noutput_items, ninput_items, input_items, output_items);

  std::cout << name << std::endl;
  for (auto x : labels) {
    std::cout << x.first << "/" << x.second << std::endl;
  }

  /*EXPECT_EQ(slice.center_frequency_, 1000);
  EXPECT_EQ(slice.frequency_range_.lower_bound(), 900);
  EXPECT_EQ(slice.frequency_range_.upper_bound(), 1100);
  EXPECT_EQ(slice.sample_rate_, 200);*/
}
