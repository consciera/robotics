// live_display.cpp: CAS-R1, session 02
//
// READ-ONLY on-camera display. Shows a handful of wire slots, redrawn 10x a
// second, with as little text as possible so it stays legible at phone size
// after compression. Make it big with the terminal's zoom (Cmd + several times).
//
// No publisher. It cannot move anything. Ctrl+C to quit.
//
// Usage (on the Jetson):  ./live_display eth10 [slot ...]     default: 19 26 29 30

#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unistd.h>
#include <vector>

#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowState_;

static std::atomic<bool> g_stop{false};
static void on_sigint(int) { g_stop = true; }

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface> [slot ...]\n", argv[0]);
    return 1;
  }
  std::vector<int> slots;
  for (int i = 2; i < argc; i++) {
    int s = std::atoi(argv[i]);
    if (s >= 0 && s < 35) slots.push_back(s);
  }
  if (slots.empty()) slots = {19, 26, 29, 30};
  std::signal(SIGINT, on_sigint);

  ChannelFactory::Instance()->Init(0, argv[1]);
  std::mutex mu;
  LowState_ last;
  std::atomic<long> count{0};
  ChannelSubscriber<LowState_> sub("rt/lowstate");
  sub.InitChannel(
      [&](const void *msg) {
        std::lock_guard<std::mutex> lk(mu);
        last = *static_cast<const LowState_ *>(msg);
        count++;
      },
      1);

  const double R2D = 180.0 / M_PI;
  std::vector<double> start(slots.size(), NAN);
  std::printf("\033[?25l");  // hide cursor
  while (!g_stop) {
    LowState_ s;
    long n;
    {
      std::lock_guard<std::mutex> lk(mu);
      s = last;
      n = count;
    }
    std::printf("\033[H\033[J");  // home + clear
    if (n == 0) {
      std::printf(" waiting for data\n");
    } else {
      std::printf(" slot     deg    move\n");
      for (size_t k = 0; k < slots.size(); k++) {
        double q = s.motor_state()[slots[k]].q() * R2D;
        if (std::isnan(start[k])) start[k] = q;
        std::printf("  %2d  %7.1f  %+6.1f\n", slots[k], q, q - start[k]);
      }
    }
    std::fflush(stdout);
    usleep(100000);
  }
  std::printf("\033[?25h\n");  // show cursor again
  return 0;
}
