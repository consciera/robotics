// watch_lowstate.cpp: CAS-R1, session 01
//
// READ-ONLY. Takes a baseline from rt/lowstate, watches for N seconds, then
// reports which motor slots moved and by how much. Used to find which slot a
// joint really is: a person moves one joint by hand while this runs.
// This file has no publisher and cannot command a motor.
//
// Usage (on the Jetson):  ./watch_lowstate <network_interface> [seconds=15]

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/hg/LowState_.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowState_;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface> [seconds]\n", argv[0]);
    return 1;
  }
  int secs = argc > 2 ? std::atoi(argv[2]) : 15;
  ChannelFactory::Instance()->Init(0, argv[1]);

  const double deg = 180.0 / M_PI;
  std::mutex mu;
  bool have_base = false;
  float base[35] = {0}, lo[35] = {0}, hi[35] = {0};

  ChannelSubscriber<LowState_> sub("rt/lowstate");
  sub.InitChannel(
      [&](const void* msg) {
        const auto& s = *static_cast<const LowState_*>(msg);
        std::lock_guard<std::mutex> lk(mu);
        for (int i = 0; i < 35; i++) {
          float q = s.motor_state()[i].q();
          if (!have_base) base[i] = lo[i] = hi[i] = q;
          if (q < lo[i]) lo[i] = q;
          if (q > hi[i]) hi[i] = q;
        }
        have_base = true;
      },
      1);

  for (int i = 0; i < 50 && !have_base; i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if (!have_base) {
    std::printf("No rt/lowstate messages in 5 s on '%s'.\n", argv[1]);
    return 2;
  }

  std::printf("Baseline taken. MOVE THE JOINT NOW. Watching %d s...\n", secs);
  std::fflush(stdout);
  std::this_thread::sleep_for(std::chrono::seconds(secs));

  std::lock_guard<std::mutex> lk(mu);
  std::printf("\nSlots that moved more than 3 deg:\n");
  std::printf("idx   start(deg)   min(deg)   max(deg)   range\n");
  int n = 0;
  for (int i = 0; i < 35; i++) {
    double range = (hi[i] - lo[i]) * deg;
    if (range < 3.0) continue;
    std::printf("%2d   %9.1f  %9.1f  %9.1f  %6.1f\n", i, base[i] * deg, lo[i] * deg,
                hi[i] * deg, range);
    n++;
  }
  if (n == 0) std::printf("(none)\n");
  return 0;
}
