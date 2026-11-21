// senses.cpp: CAS-R1, session 05
//
// READ-ONLY. Listens to rt/lowstate for about half a second and prints one
// line of key=value pairs for converse.py to turn into a readout for Cassian:
//   imu roll/pitch (deg), accelerometer (m/s^2), lowstate rate,
//   every live joint slot (0-13, 15-19, 22-26, 29-30), in degrees,
//   hottest motor temperature.
//
// No publisher, no mode calls. It cannot move anything.
//
// Usage (on the Jetson):  ./senses eth10

#include <atomic>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <unistd.h>

#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowState_;

static const double R2D = 180.0 / M_PI;
static const int kSlots[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                             13, 15, 16, 17, 18, 19, 22, 23, 24, 25, 26, 29, 30};

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface>\n", argv[0]);
    return 1;
  }
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

  for (int i = 0; i < 30 && count == 0; i++) usleep(100000);
  if (count == 0) {
    std::printf("ok=0 reason=no_lowstate\n");
    return 2;
  }
  long c0 = count;
  usleep(500000);
  long rate = (count - c0) * 2;

  std::lock_guard<std::mutex> lk(mu);
  const auto &imu = last.imu_state();
  std::printf("ok=1 hz=%ld roll=%.1f pitch=%.1f ax=%.2f ay=%.2f az=%.2f", rate,
              imu.rpy()[0] * R2D, imu.rpy()[1] * R2D, imu.accelerometer()[0],
              imu.accelerometer()[1], imu.accelerometer()[2]);
  for (int s : kSlots) std::printf(" q%d=%.1f", s, last.motor_state()[s].q() * R2D);
  int tmax = 0;
  for (int i = 0; i < 35; i++) {
    int t = last.motor_state()[i].temperature()[0];
    if (t > tmax) tmax = t;
  }
  std::printf(" tmax=%d\n", tmax);
  return 0;
}
