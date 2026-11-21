# R1: hardware & SDK findings

These notes are cumulative across sessions: each finding names the session it came from. Per-session summaries are in `../sessions/<NN>-<name>/README.md`.

Running list. Edits are additive and dated.
**Observed** = seen on this robot. **Inferred** = reasoned, not seen.
**Operator-reported** = from the operator, not observed during a logged session.

## Session 01 (2026-10-17)

### Getting connected
- The laptop needs its **own cable into the backpack's Ethernet port**. No
  DHCP on the robot subnet: set a static address (vendor-documented subnet)
  with no gateway. **Observed.**
- macOS `networksetup -setmanual` **needs `sudo`**. Without it nothing is
  saved, and no error is obvious. **Observed.**
- The Jetson's robot-subnet interface is **`eth10`**, not `eth0`. **Observed.**

### Onboard computer
- **The Jetson's clock is unset.** Anything timestamped on the robot reads
  1970. **Observed.**
- **Two SDK copies on the image.** Only `~/unitree_sdk2` has an
  `example/r1/` directory. `~/unitree_sdk2-main` has none. **Observed.**
- The login shell prompts `ros:foxy(1) noetic(2)` and waits for input. **Observed.**
- 242 pending OS updates, deliberately not applied.

### SDK
- The R1 low-level example imports its messages from **`unitree/idl/hg/`**
  (the shared humanoid set, as on G1/H1-2) and its motion switcher from
  **`unitree/robot/b2/`** (a quadruped's namespace). The G1 example does the
  same. **Observed** (source).
- `rt/lowstate` arrives at ~1 kHz. The example commands at 500 Hz. **Observed.**
- Joint numbering: see `joint-index-map.md`.
- `Crc32Core` is defined in the example file, not the SDK. **Observed.**

### Motion service and the remote
- A service named **`ai`** (form `0`) held the control channel when we
  connected (one boot observed). `ReleaseMode` released it on the first try.
  **Observed.**
- The example never re-enables the service. **Observed** (source).
- While the service was released, pressing damping on the remote produced no
  visible change. **Inconclusive.** The joints were already limp, so a
  working damping button would look the same. That the remote is disabled
  until reboot is **inferred, not confirmed.** That a reboot restores it is
  **not yet observed.**

### Commanding one joint (left wrist, wire slot 19)
- **Kp 10, +5° → stopped 1.9° short** at the hold (+3.1° reached), barely
  returned, and **wasn't visible** to the operator. **Kp 30, +10° → 0.7°
  short** (+9.3° reached), returned within 0.2°, and was **seen**. Both from
  the CSVs in `sessions/01-first-contact/data/`. **Observed.**
- Run 1 printed `end: completed`, even though the joint never reached its
  target or returned. **The program's "completed" means "ran to the end
  with no abort", not "reached the target".** Run 2's terminal output wasn't
  captured, so whether it also printed `completed` is unconfirmed. No abort
  fired in its CSV.
- Static friction: break-away ~0.3 Nm in one direction, ~0.4–0.5 Nm in the
  other. Cause of the asymmetry unknown. **Observed** (magnitudes), cause
  **unknown**.

### Posture
- Supine with the backpack on reads pitch ≈ -61°, roll ≈ 12°. The backpack
  props the torso up. **Observed.**

### From the Unitree Explore app (operator-reported, not part of session 01)
- The app's AI features require internet and are unavailable in AP mode.
- Demo teaching requires balanced standing, and the app recommends a safety hoist.

## Open questions
- Does releasing the service really disable the remote, and does a reboot
  restore it? Check `CheckMode` before and after a button press, and after a reboot.
- Is ~1 kHz lowstate one publisher or two?
- Units of `tick`. If ms, the boot time conflicts with the operator's account.
- Wrist gearbox backlash? (Hand check not done.)

## Session 05 (2026-11-21)

### Wi-Fi (untethered)
- **The backpack USB ports reach the Jetson.** A stick plugged there shows in
  the Jetson's `lsusb`. **Observed.**
- Kernel **5.10.104-tegra**. In-kernel MediaTek USB Wi-Fi drivers include
  `mt7601u` and `mt76x2u`. There's no `mt7921u` (that arrived in Linux 5.18), so
  Wi-Fi 6E sticks like the Netgear A8000 won't work without a kernel change.
  **Observed** (modules), **inferred** (A8000, not plugged in: too thick
  for the port).
- **MT7601U stick (148f:7601): plug and play.** The driver loads by itself,
  `wlan0` appears, and NetworkManager (active) manages it. 2.4 GHz only.
  **Observed.**
- `nmcli dev wifi connect` brings up Wi-Fi without touching `eth10`. A Wi-Fi
  default route is added with a lower metric than eth10's static one.
  **Observed.**
- **The voice service still works over `eth10`** (internal) when the laptop
  talks to the Jetson over Wi-Fi with the cable unplugged. **Observed.**

### Camera
- **No camera device on the Jetson** (`/dev/video*` absent, nothing on USB).
  **Observed.**
- **The R1 answers `unitree::robot::go2::VideoClient`** (service
  `"videohub"`, `GetImageSample`, API 1001). It returns a **1280×720 JPEG**
  (~300 KB), wide-angle, from the head camera, in about 1 s. **Observed.**
  This isn't documented for the R1; the SDK has no R1 video client. Same
  pattern as audio (session 03: the R1 answers the G1 voice client).
  Probe: `sessions/05-wireless/src/probe_video.cpp`.
- Turning the head by hand rotates the camera view. **Observed.** So the
  camera is in the turning part of the head.

### Joint readings
- Head yaw (slot 30) read 114.5° at rest early in the session, then
  **exactly 0.00°** (zero spread, motor live) after a hand turn. It read 64°
  after another turn, then exactly 0.00° again after the next one. Exactly
  zero twice is **unexplained**.
- A full read-only snapshot of IMU and all live joints takes ~0.6 s on the
  Jetson (`sessions/05-wireless/src/senses.cpp`).
