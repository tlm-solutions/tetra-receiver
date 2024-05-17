#include <gnuradio/io_signature.h>

#include "prometheus_gauge_populator.h"

namespace gr {
namespace prometheus {

prometheus_gauge_populator::sptr prometheus_gauge_populator::make(::prometheus::Gauge& gauge) {
  return gnuradio::get_initial_sptr(new prometheus_gauge_populator(gauge));
}

prometheus_gauge_populator::prometheus_gauge_populator(::prometheus::Gauge& gauge)
    : block(
          /*name=*/"prometheus_gauge_populator",
          /*input_signature=*/
          io_signature::make(/*min_streams=*/1, /*max_streams=*/1, /*sizeof_stream_items=*/sizeof(float)),
          /*output_signature=*/io_signature::make(/*min_streams=*/0, /*max_streams=*/0, /*sizeof_stream_items=*/0))
    , gauge_(gauge) {}

int prometheus_gauge_populator::general_work(const int, gr_vector_int& ninput_items,
                                             gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  const int input_count = ninput_items[0];
  const auto* in = (const float*)input_items[0];
  assert(input_count > 0 && "general_work called with zero input items");

  GR_LOG_DEBUG(d_logger, "Called gauge populator with " + std::to_string(input_count) + " items");

  // Consume one and write it to the gauge.
  const auto item = *in;
  consume_each(1);

  gauge_.Set(item);

  // We do not produce any items.
  return 0;
}

} // namespace prometheus
} // namespace gr
