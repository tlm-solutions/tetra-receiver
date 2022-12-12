#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

#include <gnuradio/analog/feedforward_agc_cc.h>
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

#include <cxxopts.hpp>

int main(int argc, char **argv) {
  osmosdr::source::sptr src;
  std::vector<int> offsets;
  unsigned int samp_rate;
  unsigned int udp_start;

  try {
    cxxopts::Options options(
        "tetra-receiver",
        "Receive multiple TETRA streams at once and send the bits out via UDP");

    // clang-format off
    options.add_options()
      ("h,help", "Print usage")
      ("rf", "RF gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("if", "IF gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("bb", "BB gain", cxxopts::value<unsigned int>()->default_value("10"))
      ("device-string", "additional device arguments for osmosdr, see https://projects.osmocom.org/projects/gr-osmosdr/wiki/GrOsmoSDR", cxxopts::value<std::string>()->default_value(""))
      ("center-frequency", "Center frequency of the SDR", cxxopts::value<unsigned int>()->default_value("0"))
      ("offsets", "Offsets of the TETRA streams", cxxopts::value<std::vector<int>>())
      ("samp-rate", "Sample rate of the sdr", cxxopts::value<unsigned int>()->default_value("1000000"))
      ("udp-start", "Start UDP port. Each stream gets its own UDP port, starting at udp-start", cxxopts::value<unsigned int>()->default_value("42000"))
      ;
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    samp_rate = result["samp-rate"].as<unsigned int>();

    std::string ver = gr::version();
    std::string cCompiler = gr::c_compiler();
    std::string cxxCompiler = gr::cxx_compiler();
    std::string compilerFlags = gr::compiler_flags();
    std::string prefs = gr::prefs::singleton()->to_string();

    std::cout << "GNU Radio Version: " << ver
              << "\n\n C Compiler: " << cCompiler
              << "\n\n CXX Compiler: " << cxxCompiler << "\n\n Prefs: " << prefs
              << "\n\n Compiler Flags: " << compilerFlags << "\n\n";

    // setup osmosdr source
    src = osmosdr::source::make(result["device-string"].as<std::string>());
    src->set_block_alias("src");

    src->set_sample_rate(samp_rate);
    src->set_center_freq(result["center-frequency"].as<unsigned int>());
    src->set_gain_mode(false, 0);
    src->set_gain(result["rf"].as<unsigned int>(), "RF", 0);
    src->set_gain(result["if"].as<unsigned int>(), "IF", 0);
    src->set_gain(result["bb"].as<unsigned int>(), "BB", 0);
    src->set_bandwidth(samp_rate / 2, 0);

    // create the decoding blocks for each tetra stream
    offsets = result["offsets"].as<std::vector<int>>();
    udp_start = result["udp-start"].as<unsigned int>();
  } catch (std::exception &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  auto tb = gr::make_top_block("fg");

  auto it = offsets.begin();
  for (; it != offsets.end(); ++it) {
    auto offset = *it;
    auto udp_port = udp_start + std::distance(offsets.begin(), it);
    auto decimation = 32.0f;
    auto channel_rate = 36000;
    auto sps = 2;
    auto nfilts = 32;
    auto constellation = gr::digital::constellation_dqpsk::make();
    constellation->gen_soft_dec_lut(8);

    auto xlat_taps = gr::filter::firdes::complex_band_pass(1, samp_rate, -12500,
                                                           12500, 12500 * 0.2);
    auto rrc_taps = gr::filter::firdes::root_raised_cosine(
        nfilts, nfilts, 1.0 / static_cast<float>(sps), 0.35, 11 * sps * nfilts);

    auto xlat = gr::filter::freq_xlating_fir_filter_ccc::make(
        decimation, xlat_taps, offset, samp_rate);
    auto mmse_resampler_cc = gr::filter::mmse_resampler_cc::make(
        0, static_cast<float>(samp_rate) / (static_cast<float>(decimation) *
                                            static_cast<float>(channel_rate)));
    auto agc = gr::analog::feedforward_agc_cc::make(8, 1);
    auto digital_fll_band_edge_cc =
        gr::digital::fll_band_edge_cc::make(sps, 0.35, 45, M_PI / 100.0f);
    auto digital_pfb_clock_sync_xxx = gr::digital::pfb_clock_sync_ccf::make(
        sps, 2 * M_PI / 100.0f, rrc_taps, nfilts, nfilts / 2.0, 1.5, sps);
    auto digital_cma_equalizer_cc =
        gr::digital::cma_equalizer_cc::make(15, 1, 10e-3, sps);
    auto diff_phasor_cc = gr::digital::diff_phasor_cc::make();
    auto digital_constellation_decoder_cb =
        gr::digital::constellation_decoder_cb::make(constellation);
    auto digital_map_bb =
        gr::digital::map_bb::make(constellation->pre_diff_code());
    auto blocks_unpack_k_bits_bb =
        gr::blocks::unpack_k_bits_bb::make(constellation->bits_per_symbol());
    auto blocks_udp_sink = gr::blocks::udp_sink::make(sizeof(char), "127.0.0.1",
                                                      udp_port, 1472, false);

    try {
      tb->connect(src, 0, xlat, 0);
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
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error creating gnuradio blocks for offset number "
                << std::distance(offsets.begin(), it) << " with value "
                << offset << "\n"
                << e.what();
      return EXIT_FAILURE;
    }
  }

  tb->start();

  tb->wait();

  return EXIT_SUCCESS;
}
