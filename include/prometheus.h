#ifndef FUNNEL_PROMETHEUS_HPP
#define FUNNEL_PROMETHEUS_HPP

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <memory>

class PrometheusExporter {
private:
  std::shared_ptr<prometheus::Registry> registry_;
  std::unique_ptr<prometheus::Exposer> exposer_;

public:
  PrometheusExporter(const std::string& host) noexcept;
  ~PrometheusExporter() noexcept = default;

  auto signal_strength() noexcept -> prometheus::Family<prometheus::Gauge>&;
};

#endif // FUNNEL_PROMETHEUS_HPP
