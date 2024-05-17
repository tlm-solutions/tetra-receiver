#ifndef INCLUDE_PROMETHEUS_GAUGE_POPULATOR_H
#define INCLUDE_PROMETHEUS_GAUGE_POPULATOR_H

#include <gnuradio/block.h>
#include <gnuradio/blocks/api.h>

#include <prometheus/gauge.h>

namespace gr {
namespace prometheus {

/// This block takes a float as an input and writes in into a prometheus gauge
class prometheus_gauge_populator : virtual public block {
public:
  using sptr = boost::shared_ptr<prometheus_gauge_populator>;

  prometheus_gauge_populator(::prometheus::Gauge& gauge);

  static sptr make(::prometheus::Gauge& gauge);

  int general_work(int noutput_items, gr_vector_int& ninput_items, gr_vector_const_void_star& input_items,
                   gr_vector_void_star& output_items) override;

private:
  /// the prometheus gauge we are populating with this block
  ::prometheus::Gauge& gauge_;
};

} // namespace prometheus
} // namespace gr

#endif
