#pragma once
namespace rotary {
// Nominal speed values follow the Crumar Mojo61 manual's vintage-cabinet
// measurements. Time constants below are our first-order model tuning,
// not the manuals' ambiguously defined total ramp durations.
namespace tuning {
constexpr float hornSlowHz=.77f;
constexpr float hornFastHz=6.9f;
constexpr float drumSlowHz=.72f;
constexpr float drumFastHz=6.4f;
constexpr float hornAccelerationTau=.6f;
constexpr float hornDecelerationTau=.8f;
constexpr float drumAccelerationTau=2.f;
constexpr float drumDecelerationTau=1.8f;
}
}
