// baseline_lowstate.cpp: CAS-R1, session 02
//
// READ-ONLY baseline. Prints (to stdout, meant to be redirected to a file):
//   - the operator's posture note, verbatim, at capture time
//   - the Jetson's own clock (known to be unset in session 01)
//   - who holds the control channel (MotionSwitcherClient::CheckMode, a query)
//   - lowstate rate, mode_machine, mode_pr, tick, IMU orientation
//   - every wire slot 0-34: live?, position (mean and spread over 0.5 s),
//     velocity, torque, temperature, motor mode
//   - a summary: live slots, and the status of slots 19, 26, 29, 30
//
// No publisher. Never calls ReleaseMode or SelectMode. It cannot move anything.
//
// Usage (on the Jetson):  ./baseline_lowstate eth10 "posture note"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/b2/motion_switcher/motion_switcher_client.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowState_;

static const double R2D = 180.0 / M_PI;
static const int kN = 35;
static const int kWatch[4] = {19, 26, 29, 30};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <network_interface> \"posture note\"\n", argv[0]);
    return 1;
  }
  ChannelFactory::Instance()->Init(0, argv[1]);

  std::mutex mu;
  LowState_ last;
  std::atomic<long> count{0};
  // Running sums for a 0.5 s average, filled only while `sampling` is true.
  std::atomic<bool> sampling{false};
  long n = 0;
  double sum[kN] = {0}, lo[kN], hi[kN];
  for (int i = 0; i < kN; i++) lo[i] = 1e9, hi[i] = -1e9;

  ChannelSubscriber<LowState_> sub("rt/lowstate");
  sub.InitChannel(
      [&](const void *msg) {
        const auto &s = *static_cast<const LowState_ *>(msg);
        std::lock_guard<std::mutex> lk(mu);
        last = s;
        count++;
        if (sampling) {
          for (int i = 0; i < kN; i++) {
            double q = s.motor_state()[i].q();
            sum[i] += q;
            if (q < lo[i]) lo[i] = q;
            if (q > hi[i]) hi[i] = q;
          }
          n++;
        }
      },
      1);

  for (int i = 0; i < 50 && count == 0; i++) usleep(100000);

  std::printf("# CAS-R1 baseline (read-only)\n");
  std::printf("# posture note (operator, at capture time): %s\n", argv[2]);

  std::time_t now = std::time(nullptr);
  char buf[64];
  std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
  std::printf("# jetson clock: %s (epoch %ld)\n", buf, long(now));

  // Who holds the control channel. Query only.
  {
    auto msc = std::make_shared<b2::MotionSwitcherClient>();
    msc->SetTimeout(3.0f);
    msc->Init();
    std::string form, name;
    int32_t rc = msc->CheckMode(form, name);
    if (rc == 0)
      std::printf("# control channel: form='%s' name='%s'%s\n", form.c_str(), name.c_str(),
                  name.empty() ? " (empty = nothing holds it, i.e. released)" : "");
    else
      std::printf("# control channel: CheckMode query FAILED, code %d\n", rc);
  }

  if (count == 0) {
    std::printf("# NO rt/lowstate messages in 5 s on '%s'. No baseline.\n", argv[1]);
    return 2;
  }

  long c0 = count;
  sleep(1);
  long rate = count - c0;

  sampling = true;
  usleep(500000);
  sampling = false;

  std::lock_guard<std::mutex> lk(mu);
  const auto &s = last;
  const auto &rpy = s.imu_state().rpy();
  std::printf("# lowstate: %ld Hz  mode_machine=%u  mode_pr=%u  tick=%u  samples_averaged=%ld\n",
              rate, unsigned(s.mode_machine()), unsigned(s.mode_pr()), unsigned(s.tick()), n);
  std::printf("# imu rpy deg: roll %.1f  pitch %.1f  yaw %.1f\n", rpy[0] * R2D, rpy[1] * R2D,
              rpy[2] * R2D);
  std::printf("idx,live,q_deg_mean,q_deg_spread,dq_rad_s,tau_nm,temp_c,mode\n");

  bool live[kN];
  std::string live_list;
  int nlive = 0;
  for (int i = 0; i < kN; i++) {
    const auto &m = s.motor_state()[i];
    live[i] = m.q() != 0 || m.dq() != 0 || m.tau_est() != 0 || m.temperature()[0] != 0 ||
              m.mode() != 0;
    double mean = n ? sum[i] / n : m.q();
    double spread = n ? (hi[i] - lo[i]) : 0;
    std::printf("%d,%d,%.2f,%.2f,%.3f,%.3f,%d,%u\n", i, live[i] ? 1 : 0, mean * R2D,
                spread * R2D, m.dq(), m.tau_est(), m.temperature()[0], unsigned(m.mode()));
    if (live[i]) {
      live_list += (nlive ? "," : "") + std::to_string(i);
      nlive++;
    }
  }
  std::printf("# live slots (%d): %s\n", nlive, live_list.c_str());
  std::printf("# watched slots:");
  for (int w : kWatch) std::printf(" %d=%s", w, live[w] ? "LIVE" : "no-data");
  std::printf("\n");
  return 0;
}
