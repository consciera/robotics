// read_lowstate.cpp: CAS-R1, session 01
//
// READ-ONLY. Subscribes to rt/lowstate, prints one snapshot of every motor
// slot, then exits. This file has no publisher and no motion-switcher
// client. It cannot command a motor.
//
// Usage (on the Jetson):  ./read_lowstate <network_interface>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <thread>

#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/hg/LowState_.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowState_;

// Names from enum R1JointIndex in the vendor example
// (example/r1/low_level/r1_ankle_swing_example.cpp).
// The vendor enum puts RightShoulderPitch at 29, not 19. We label both and
// print every slot, so the data shows which one is live.
static const char* kName[35] = {
    "L hip pitch", "L hip roll", "L hip yaw", "L knee", "L ankle pitch", "L ankle roll",
    "R hip pitch", "R hip roll", "R hip yaw", "R knee", "R ankle pitch", "R ankle roll",
    "waist roll", "waist yaw",
    "L shoulder pitch", "L shoulder roll", "L shoulder yaw", "L elbow", "L wrist roll",
    "(19: unnamed)",
    "R shoulder roll", "R shoulder yaw", "R elbow", "R wrist roll",
    "head pitch", "head yaw",
    "-", "-", "-", "(29: R sh. pitch?)", "-", "-", "-", "-", "-"};

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface>\n", argv[0]);
    return 1;
  }
  ChannelFactory::Instance()->Init(0, argv[1]);

  std::mutex mu;
  LowState_ last;
  std::atomic<int> count{0};

  ChannelSubscriber<LowState_> sub("rt/lowstate");
  sub.InitChannel(
      [&](const void* msg) {
        std::lock_guard<std::mutex> lk(mu);
        last = *static_cast<const LowState_*>(msg);
        count++;
      },
      1);

  // Wait up to 5 s for the first message.
  for (int i = 0; i < 50 && count == 0; i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if (count == 0) {
    std::printf("No rt/lowstate messages in 5 s on '%s'.\n", argv[1]);
    return 2;
  }

  // Count messages over 1 s to get the publish rate.
  int c0 = count;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int rate = count - c0;

  LowState_ s;
  {
    std::lock_guard<std::mutex> lk(mu);
    s = last;
  }

  const double deg = 180.0 / M_PI;
  const auto& rpy = s.imu_state().rpy();
  std::printf("rt/lowstate  %d Hz   mode_machine=%u  mode_pr=%u  tick=%u\n", rate,
              unsigned(s.mode_machine()), unsigned(s.mode_pr()), unsigned(s.tick()));
  std::printf("IMU rpy (deg): roll %.1f  pitch %.1f  yaw %.1f\n\n", rpy[0] * deg,
              rpy[1] * deg, rpy[2] * deg);
  std::printf("idx  joint                  q(deg)   dq(r/s)  tau(Nm)  temp  mode\n");

  for (int i = 0; i < 35; i++) {
    const auto& m = s.motor_state()[i];
    bool live = m.q() != 0 || m.dq() != 0 || m.tau_est() != 0 || m.temperature()[0] != 0 ||
                m.mode() != 0;
    if (i > 25 && !live && i != 29) continue;  // hide empty spare slots, keep 29 visible
    std::printf("%2d   %-20s %8.1f  %7.2f  %7.2f  %4d  %4u%s\n", i, kName[i], m.q() * deg,
                m.dq(), m.tau_est(), m.temperature()[0], unsigned(m.mode()),
                live ? "" : "   <- no data");
  }
  return 0;
}
