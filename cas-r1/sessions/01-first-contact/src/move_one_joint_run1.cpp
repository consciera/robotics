// move_one_joint.cpp: CAS-R1, session 01, goal 3
//
// The first command we send to the robot. It moves ONE joint (wire slot 19,
// the left wrist twist, identified by hand test) by +5 degrees, holds it,
// returns it, and leaves every joint in soft damping.
//
// Everything that matters is a hard-coded constant below, so nothing can be
// mistyped on camera.
//
//   ./move_one_joint eth10 --dry-run   reads state, prints the plan, sends nothing
//   ./move_one_joint eth10             asks you to type GO, then runs
//
// Stops: Ctrl+C (sends damping, then exits), a hand on the wrist (Kp is low),
// the power switch.
//
// NOTE: running for real releases Unitree's built-in motion service. While it
// is released, the remote's damping button probably does nothing. This program
// does not turn the service back on; restart the robot afterwards.

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

#include <unitree/idl/hg/LowCmd_.hpp>
#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/b2/motion_switcher/motion_switcher_client.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

using namespace unitree::robot;
using unitree_hg::msg::dds_::LowCmd_;
using unitree_hg::msg::dds_::LowState_;

// ---- The whole experiment, in constants ------------------------------------
static const int    kSlot      = 19;     // wire slot: left wrist twist (hand test)
static const double kDeltaDeg  = 5.0;    // how far to move
static const double kKp        = 10.0;   // stiffness for slot 19 (Unitree uses 50)
static const double kKd        = 1.0;    // damping for slot 19 (Unitree uses 2)
static const double kDampKd    = 1.0;    // damping for every other joint (kp = 0)
static const double kRampS     = 2.0;    // seconds to move out, and to move back
static const double kHoldS     = 2.0;    // seconds to hold at +5 deg
static const double kSettleS   = 0.5;    // seconds of pure damping at the end
static const double kDt        = 0.002;  // 500 Hz, same as Unitree's example
// Abort limits (go to damping and exit if crossed)
static const double kMaxErrDeg = 15.0;   // target vs measured, slot 19
static const double kMaxVel    = 3.0;    // rad/s, slot 19
static const double kStaleS    = 0.1;    // no new lowstate for this long
// The 26 wire slots that carry live data on this robot (read 14:57 today).
static const int kLive[26] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                              13, 15, 16, 17, 18, 19, 22, 23, 24, 25, 26, 29, 30};
// -----------------------------------------------------------------------------

// Copied verbatim from example/r1/low_level/r1_ankle_swing_example.cpp:134
inline uint32_t Crc32Core(uint32_t *ptr, uint32_t len) {
  uint32_t xbit = 0;
  uint32_t data = 0;
  uint32_t CRC32 = 0xFFFFFFFF;
  const uint32_t dwPolynomial = 0x04c11db7;
  for (uint32_t i = 0; i < len; i++) {
    xbit = 1 << 31;
    data = ptr[i];
    for (uint32_t bits = 0; bits < 32; bits++) {
      if (CRC32 & 0x80000000) {
        CRC32 <<= 1;
        CRC32 ^= dwPolynomial;
      } else
        CRC32 <<= 1;
      if (data & xbit) CRC32 ^= dwPolynomial;
      xbit >>= 1;
    }
  }
  return CRC32;
}

static std::atomic<bool> g_stop{false};
static void on_sigint(int) { g_stop = true; }

static std::mutex g_mu;
static LowState_ g_state;
static std::atomic<long> g_count{0};
static std::chrono::steady_clock::time_point g_last_rx;

static const double D2R = M_PI / 180.0, R2D = 180.0 / M_PI;

// Build one command. kp_slot = 0 means "damping everywhere".
static LowCmd_ make_cmd(uint8_t mode_pr, uint8_t mode_machine, double q_slot, double kp_slot) {
  LowCmd_ c;
  c.mode_pr() = mode_pr;
  c.mode_machine() = mode_machine;
  for (int s : kLive) {
    auto &m = c.motor_cmd().at(s);
    m.mode() = 1;
    m.q() = 0;
    m.dq() = 0;
    m.tau() = 0;
    m.kp() = 0;
    m.kd() = kDampKd;
  }
  auto &w = c.motor_cmd().at(kSlot);
  w.q() = q_slot;
  w.kp() = kp_slot;
  w.kd() = kKd;
  return c;
}

// Checksum on the exact object being sent, as the example does (line 307).
static void send(ChannelPublisher<LowCmd_> &pub, LowCmd_ c) {
  c.crc() = Crc32Core((uint32_t *)&c, (sizeof(c) >> 2) - 1);
  pub.Write(c);
}

// Smooth 0 -> 1 over [0, 1] (cosine ramp: no jump in speed at either end).
static double ease(double x) {
  if (x <= 0) return 0;
  if (x >= 1) return 1;
  return 0.5 - 0.5 * std::cos(M_PI * x);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface> [--dry-run]\n", argv[0]);
    return 1;
  }
  const bool dry = argc > 2 && std::strcmp(argv[2], "--dry-run") == 0;
  std::signal(SIGINT, on_sigint);

  ChannelFactory::Instance()->Init(0, argv[1]);
  ChannelSubscriber<LowState_> sub("rt/lowstate");
  sub.InitChannel(
      [](const void *msg) {
        std::lock_guard<std::mutex> lk(g_mu);
        g_state = *static_cast<const LowState_ *>(msg);
        g_last_rx = std::chrono::steady_clock::now();
        g_count++;
      },
      1);

  for (int i = 0; i < 50 && g_count == 0; i++) usleep(100000);
  if (g_count == 0) {
    std::printf("ABORT: no rt/lowstate in 5 s on '%s'. Nothing sent.\n", argv[1]);
    return 2;
  }

  auto snapshot = [] {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_state;
  };

  // ---- Pre-flight checks (read-only) ----
  LowState_ s = snapshot();
  for (int slot : kLive) {
    if (s.motor_state()[slot].temperature()[0] == 0) {
      std::printf("ABORT: slot %d reports no data. Nothing sent.\n", slot);
      return 3;
    }
  }
  double q0 = s.motor_state()[kSlot].q();
  double dq0 = s.motor_state()[kSlot].dq();
  if (std::fabs(dq0) > 0.2) {
    std::printf("ABORT: slot %d is moving (%.2f rad/s). Hands off, retry. Nothing sent.\n",
                kSlot, dq0);
    return 3;
  }
  std::printf("slot %d now %.1f deg -> target %.1f deg (+%.1f), Kp %.0f Kd %.0f\n", kSlot,
              q0 * R2D, (q0 + kDeltaDeg * D2R) * R2D, kDeltaDeg, kKp, kKd);
  std::printf("plan: %.0fs out, %.0fs hold, %.0fs back, then damping. mode_machine=%u mode_pr=%u\n",
              kRampS, kHoldS, kRampS, unsigned(s.mode_machine()), unsigned(s.mode_pr()));

  if (dry) {
    std::printf("DRY RUN: nothing released, nothing sent.\n");
    return 0;
  }

  std::printf("Type GO and press Enter to release the motion service and move. Anything else aborts: ");
  std::fflush(stdout);
  std::string answer;
  std::getline(std::cin, answer);
  if (answer != "GO" || g_stop) {
    std::printf("Aborted. Nothing sent.\n");
    return 0;
  }

  // ---- Release Unitree's motion service (same calls as the example) ----
  auto msc = std::make_shared<b2::MotionSwitcherClient>();
  msc->SetTimeout(5.0f);
  msc->Init();
  std::string form, name;
  msc->CheckMode(form, name);
  std::printf("motion service before release: form='%s' name='%s'\n", form.c_str(), name.c_str());
  for (int tries = 0; !name.empty(); tries++) {
    if (tries == 3 || g_stop) {
      std::printf("ABORT: motion service still active after %d tries. Nothing sent.\n", tries);
      return 4;
    }
    if (msc->ReleaseMode()) std::printf("ReleaseMode failed, retrying\n");
    sleep(2);
    msc->CheckMode(form, name);
  }
  std::printf("motion service released.\n");

  ChannelPublisher<LowCmd_> pub("rt/lowcmd");
  pub.InitChannel();

  // Re-read the start angle AFTER release, so the first command matches where
  // the wrist actually is now (no jump).
  s = snapshot();
  q0 = s.motor_state()[kSlot].q();
  const uint8_t mpr = s.mode_pr(), mm = s.mode_machine();

  std::ofstream csv("move_one_joint_log.csv");
  csv << "t,target_deg,q_deg,dq,tau_est\n";

  const double t_out = kRampS, t_hold = t_out + kHoldS, t_back = t_hold + kRampS;
  const double t_end = t_back + kSettleS;
  const char *reason = "completed";
  int next_print = 0;
  const double print_at[] = {0.0, t_out, t_hold, t_back};

  auto t_start = std::chrono::steady_clock::now();
  auto next = t_start;
  for (long k = 0;; k++) {
    double t = k * kDt;
    if (t >= t_end) break;
    if (g_stop) { reason = "Ctrl+C"; break; }

    double frac = t < t_out ? ease(t / kRampS)
                : t < t_hold ? 1.0
                : t < t_back ? 1.0 - ease((t - t_hold) / kRampS)
                : 0.0;
    double target = q0 + frac * kDeltaDeg * D2R;
    double kp = t < t_back ? kKp : 0.0;  // last 0.5 s: damping only

    LowState_ now;
    std::chrono::steady_clock::time_point last_rx;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      now = g_state;
      last_rx = g_last_rx;
    }
    double q = now.motor_state()[kSlot].q(), dq = now.motor_state()[kSlot].dq();
    double age = std::chrono::duration<double>(std::chrono::steady_clock::now() - last_rx).count();
    if (age > kStaleS) { reason = "lowstate went stale"; break; }
    if (std::fabs(q - target) * R2D > kMaxErrDeg) { reason = "tracking error too large"; break; }
    if (std::fabs(dq) > kMaxVel) { reason = "slot moving too fast"; break; }

    send(pub, make_cmd(mpr, mm, target, kp));
    csv << t << "," << target * R2D << "," << q * R2D << "," << dq << ","
        << now.motor_state()[kSlot].tau_est() << "\n";

    if (next_print < 4 && t >= print_at[next_print]) {
      std::printf("t=%.1fs  target %6.1f deg  measured %6.1f deg\n", t, target * R2D, q * R2D);
      next_print++;
    }
    next += std::chrono::microseconds(2000);
    std::this_thread::sleep_until(next);
  }

  // Always finish with damping everywhere, for 0.5 s.
  for (int i = 0; i < 250; i++) {
    send(pub, make_cmd(mpr, mm, 0.0, 0.0));
    usleep(2000);
  }
  double q_end = snapshot().motor_state()[kSlot].q();
  std::printf("end: %s. slot %d at %.1f deg (started %.1f). All joints in damping.\n", reason,
              kSlot, q_end * R2D, q0 * R2D);
  std::printf("Motion service is still released: restart the robot to get the remote back.\n");
  return 0;
}
