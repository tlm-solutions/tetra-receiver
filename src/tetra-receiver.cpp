#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <cxxopts.hpp>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/udp_sink.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/constants.h>
#include <gnuradio/digital/cma_equalizer_cc.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/constellation_decoder_cb.h>
#include <gnuradio/digital/diff_phasor_cc.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/digital/pfb_clock_sync_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#include <gnuradio/filter/mmse_resampler_cc.h>
#include <gnuradio/logger.h>
#include <gnuradio/prefs.h>
#include <gnuradio/sys_paths.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>

#include "config.h"
#include "prometheus.h"

static auto print_gnuradio_diagnostics() -> void {
  const auto ver = gr::version();
  const auto c_compiler = gr::c_compiler();
  const auto cxx_compiler = gr::cxx_compiler();
  const auto compiler_flags = gr::compiler_flags();
  const auto prefs = gr::prefs::singleton()->to_string();

  std::cout << "GNU Radio Version: " << ver << "\n\n C Compiler: " << c_compiler
            << "\n\n CXX Compiler: " << cxx_compiler << "\n\n Prefs: " << prefs
            << "\n\n Compiler Flags: " << compiler_flags << "\n\n";
}

class GnuradioTopBlock {
private:
  static auto from_config(const config::Stream& stream, gr::top_block_sptr tb, gr::basic_block_sptr input) -> void {
    auto decimation = stream.decimation_;
    auto offset = static_cast<int>(stream.spectrum_.center_frequency_) -
                  static_cast<int>(stream.input_spectrum_.center_frequency_);
    auto sample_rate = stream.input_spectrum_.sample_rate_;

    float half_sample_rate = stream.spectrum_.sample_rate_ / 2;
    auto xlat_taps = gr::filter::firdes::complex_band_pass(1, stream.input_spectrum_.sample_rate_, -half_sample_rate,
                                                           half_sample_rate, half_sample_rate * 0.2);
    auto xlat = gr::filter::freq_xlating_fir_filter_ccc::make(decimation, xlat_taps, offset,
                                                              stream.input_spectrum_.sample_rate_);

    auto channel_rate = 36000;
    auto sps = 2;
    auto nfilts = 32;
    auto constellation = gr::digital::constellation_dqpsk::make();
    constellation->gen_soft_dec_lut(8);

    auto rrc_taps =
        gr::filter::firdes::root_raised_cosine(nfilts, nfilts, 1.0 / static_cast<float>(sps), 0.35, 11 * sps * nfilts);

    auto mmse_resampler_cc = gr::filter::mmse_resampler_cc::make(
        0, static_cast<float>(sample_rate) / (static_cast<float>(decimation) * static_cast<float>(channel_rate)));
    auto agc = gr::analog::feedforward_agc_cc::make(8, 1);
    auto digital_fll_band_edge_cc = gr::digital::fll_band_edge_cc::make(sps, 0.35, 45, M_PI / 100.0f);
    auto digital_pfb_clock_sync_xxx =
        gr::digital::pfb_clock_sync_ccf::make(sps, 2 * M_PI / 100.0f, rrc_taps, nfilts, nfilts / 2.0, 1.5, sps);
    auto digital_cma_equalizer_cc = gr::digital::cma_equalizer_cc::make(15, 1, 10e-3, sps);
    auto diff_phasor_cc = gr::digital::diff_phasor_cc::make();
    auto digital_constellation_decoder_cb = gr::digital::constellation_decoder_cb::make(constellation);
    auto digital_map_bb = gr::digital::map_bb::make(constellation->pre_diff_code());
    auto blocks_unpack_k_bits_bb = gr::blocks::unpack_k_bits_bb::make(constellation->bits_per_symbol());
    auto blocks_udp_sink = gr::blocks::udp_sink::make(sizeof(char), stream.host_, stream.port_, 1472, false);

    tb->connect(input, 0, xlat, 0);
    tb->connect(xlat, 0, mmse_resampler_cc, 0);
    tb->connect(mmse_resampler_cc, 0, agc, 0);
    tb->connect(agc, 0, digital_fll_band_edge_cc, 0);
    tb->connect(digital_fll_band_edge_cc, 0, digital_pfb_clock_sync_xxx, 0);
    tb->connect(digital_pfb_clock_sync_xxx, 0, digital_cma_equalizer_cc, 0);
    tb->connect(digital_cma_equalizer_cc, 0, diff_phasor_cc, 0);
    tb->connect(diff_phasor_cc, 0, digital_constellation_decoder_cb, 0);
    tb->connect(digital_constellation_decoder_cb, 0, digital_map_bb, 0);
    tb->connect(digital_map_bb, 0, blocks_unpack_k_bits_bb, 0);
    tb->connect(blocks_unpack_k_bits_bb, 0, blocks_udp_sink, 0);
  };

  static auto from_config(const config::Decimate& decimate, gr::top_block_sptr tb, gr::basic_block_sptr input) -> void {
    float half_sample_rate = decimate.spectrum_.sample_rate_ / 2;
    auto offset = static_cast<int>(decimate.spectrum_.center_frequency_) -
                  static_cast<int>(decimate.input_spectrum_.center_frequency_);
    auto xlat_taps = gr::filter::firdes::complex_band_pass(1, decimate.input_spectrum_.sample_rate_, -half_sample_rate,
                                                           half_sample_rate, half_sample_rate * 0.2);
    auto xlat = gr::filter::freq_xlating_fir_filter_ccc::make(decimate.decimation_, xlat_taps, offset,
                                                              decimate.input_spectrum_.sample_rate_);

    tb->connect(input, 0, xlat, 0);

    for (auto const& stream : decimate.streams_) {
      from_config(stream, tb, xlat);
    }

    // add a null sink to have at least one connected
    auto null_sink = gr::blocks::null_sink::make(/*sizeof_stream_item=*/sizeof(gr_complex));
    tb->connect(xlat, 0, null_sink, 0);
  };

public:
  static auto from_config(const config::TopLevel& top) -> gr::top_block_sptr {
    auto tb = gr::make_top_block("fg");

    // setup osmosdr source
    auto src = osmosdr::source::make(top.device_string_);
    src->set_block_alias("src");

    src->set_sample_rate(top.spectrum_.sample_rate_);
    src->set_center_freq(top.spectrum_.center_frequency_);
    src->set_gain_mode(false, 0);
    src->set_gain(top.rf_gain_, "RF", 0);
    src->set_gain(top.if_gain_, "IF", 0);
    src->set_gain(top.bb_gain_, "BB", 0);
    src->set_bandwidth(top.spectrum_.sample_rate_ / 2, 0);

    for (auto const& decimate : top.decimators_) {
      from_config(decimate, tb, src);
    }
    for (auto const& stream : top.streams_) {
      from_config(stream, tb, src);
    }

    // add a null sink to have at least one connected
    auto null_sink = gr::blocks::null_sink::make(/*sizeof_stream_item=*/sizeof(gr_complex));
    tb->connect(src, 0, null_sink, 0);

    return tb;
  }
};

auto main(int argc, char** argv) -> int {
  try {
    cxxopts::Options options("tetra-receiver", "Receive multiple TETRA streams at once and send the bits out via UDP");

    // clang-format off
    options.add_options()
      ("h,help", "Print usage")
      ("config-file", "Instead of these options read the config from a config file", cxxopts::value<std::string>()->default_value(""))
      ("rf", "RF gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("if", "IF gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("bb", "BB gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("device-string", "additional device arguments for osmosdr, see https://projects.osmocom.org/projects/gr-osmosdr/wiki/GrOsmoSDR", cxxopts::value<std::string>()->default_value(""))
      ("center-frequency", "Center frequency of the SDR", cxxopts::value<unsigned int>()->default_value("0"))
      ("offsets", "offsets of the TETRA streams", cxxopts::value<std::vector<int>>())
      ("samp-rate", "Sample rate of the sdr", cxxopts::value<unsigned int>()->default_value("1000000"))
      ("udp-start", "Start UDP port. Each stream gets its own UDP port, starting at udp-start", cxxopts::value<unsigned int>()->default_value("27000"))
      ("prometheus-host", "Address of prometheus server for exporting metrics.", cxxopts::value<std::string>()->default_value("127.0.0.1:26100"))
      ;
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return EXIT_SUCCESS;
    }

    gr::top_block_sptr tb = 0;
    std::shared_ptr<PrometheusExporter> exporter = nullptr;

    // Read from config file instead
    if (result.count("config-file")) {
      auto data = toml::parse(result["config-file"].as<std::string>());
      auto top = toml::get<config::TopLevel>(data);

      tb = GnuradioTopBlock::from_config(top);
      std::string prometheus_addr = top.prometheus_host_ + ":" + std::to_string(top.prometheus_port_);
      exporter = std::make_shared<PrometheusExporter>(prometheus_addr);
    } else {
      const auto sample_rate = result["samp-rate"].as<unsigned int>();
      const auto& device_string = result["device-string"].as<std::string>();
      const auto center_frequency = result["center-frequency"].as<unsigned int>();
      const auto rf_gain = result["rf"].as<unsigned int>();
      const auto if_gain = result["if"].as<unsigned int>();
      const auto bb_gain = result["bb"].as<unsigned int>();
      const auto& offsets = result["offsets"].as<std::vector<int>>();
      const auto udp_start = result["udp-start"].as<unsigned int>();
      const auto prometheus_host = result["prometheus-host"].as<std::string>();
      const auto prometheus_port = result["prometheus-port"].as<unsigned short>();

      std::vector<config::Stream> streams;
      const auto input_spectrum = config::SpectrumSlice(center_frequency, sample_rate);

      // create the decoding blocks for each tetra Stream
      for (auto offsets_it = offsets.begin(); offsets_it != offsets.end(); ++offsets_it) {
        auto stream_frequency = center_frequency + *offsets_it;
        auto udp_port = udp_start + std::distance(offsets.begin(), offsets_it);

        const auto tetra_spectrum = config::SpectrumSlice(stream_frequency, config::kTetraSampleRate);

        streams.emplace_back(config::Stream(input_spectrum, tetra_spectrum, config::kDefaultHost, udp_port));
      }

      config::TopLevel top(
          input_spectrum, 
          device_string, 
          rf_gain, 
          if_gain, 
          bb_gain, 
          prometheus_host, 
          prometheus_port, 
          /*streams=*/streams,
          /*decimators=*/{}
        );

      tb = GnuradioTopBlock::from_config(top);
      std::string prometheus_addr = top.prometheus_host_ + ":" + std::to_string(top.prometheus_port_);
      exporter = std::make_shared<PrometheusExporter>(prometheus_addr);
    }
    auto& signal_strength = exporter->signal_strength();

    signal_strength.Add({{"signal_strength", 0}}).SetToCurrentTime();
    signal_strength.Add({{"signal_strength", 0}}).Set(25.123);

    // print the gnuradio debugging information
    print_gnuradio_diagnostics();

    tb->start();
    tb->wait();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
