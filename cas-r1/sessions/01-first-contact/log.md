# Session 01 — first SDK contact · 2026-10-17

Hardware: Unitree R1 EDU (26 DoF, ~29 kg, 123 cm). Onboard Jetson Orin NX on the
robot's internal subnet. Backpack installed, ethernet connected.
Posture: **supine on a mat, all session.** No hoist in the room.
Physical stop: damping mode on the controller.

Goals, in order: (1) reach the Jetson over the robot subnet; (2) read joint
states; (3) send one value to one joint and see it move.

Prediction for this session: _none on file in `predictions/` at session start._

Times are local (PDT). Entries are written as we go and are append-only.

---

## Running log

**14:19 — Start. Checked the laptop's network state.**
- Only Wi-Fi is up, on the home LAN. Both wired Ethernet adapters show
  `inactive`, so the laptop has no wired link at all yet.
- "Ethernet connected" so far means the backpack ↔ robot side. Laptop ↔ robot
  is not cabled, or the cable/adapter has no link.
- Next: get a wired link from the laptop onto the robot subnet.

**14:2x: Operator confirms the cable is seated at both ends and the robot is powered on.**
- Robot subnet and Jetson address come from the operator and haven't been
  checked on the wire yet. Addresses are left out of the log (repo rule).
- Plan, one command at a time: (1) find which interface has link,
  (2) give it a static address on the robot subnet, (3) ping the Jetson.
- At 14:19 both wired adapters showed no link. Step 1 checks whether that changed.

**14:26: Step 1 result: still no wired link.**
- Both Ethernet adapters show up on the laptop but read `inactive`. Only
  Wi-Fi is active. Same as 14:19, even though the cable is reported seated
  at both ends and the robot is powered.
- Inactive means no carrier (no electrical link). It isn't an addressing
  problem, so changing IP settings won't fix it. Stopped before step 2.
- Operator first pasted the command instead of its output; Claude ran it
  directly (read-only).

**14:2x: Root cause of "no link" is probably found: the laptop isn't cabled to the robot.**
- Operator asked whether the laptop has to be physically connected. That
  suggests "ethernet connected at both ends" meant backpack ↔ robot, not
  laptop ↔ robot. Explains the `inactive` readings at 14:19 and 14:26.
- Unconfirmed: whether the R1 offers a usable wireless path to the Jetson
  out of the box. Going wired for first contact.

**14:38: Wired link is up. Step 1 done.**
- Operator cabled the laptop to the backpack's Ethernet port via a USB
  gigabit adapter. A new interface appeared, `active`, negotiated at
  1000baseT full-duplex. That's a clean physical link.
- It self-assigned a link-local (169.254/16) address. That means nothing on
  the robot network is handing out addresses, as expected, so a static
  address is needed.
- This adapter is first in the macOS service order. The static config gets
  no router/gateway, so internet stays on Wi-Fi.
- Time lost to the cabling misunderstanding: ~19 min (14:19 → 14:38).

**14:4x: Step 2 didn't take the first time.**
- Operator ran `networksetup -setmanual` on the adapter (no gateway). The
  interface still shows the self-assigned link-local address, not the
  static one.
- Not yet known: whether the setting wasn't saved, or was saved but not
  applied to the live interface. Checking the saved config next.
- Saved config check: the adapter is still set to DHCP, so the static
  setting was never saved. It isn't a case of saved-but-not-applied.
  Likely cause (a guess): `networksetup` needed admin rights and failed
  without any visible error. Retrying with `sudo`.

**14:5x: Step 2 done. The static address took on the second try, with `sudo`.**
- The interface now has the static address on the robot subnet, /24 mask,
  link active.
- Finding: without `sudo`, `networksetup -setmanual` saved nothing and the
  operator didn't notice an error. With `sudo` it applied immediately.
  Goes in notes as a gotcha.
- Next: ping the Jetson at the address the operator supplied (not yet
  verified for the R1).

**14:5x: Step 3 done. Something answers at the expected Jetson address.**
- 4/4 replies, 0% loss, avg ~0.9 ms round trip. TTL 64 is consistent with
  a Linux host.
- Caveat: a ping only shows *a* device answers at that address. It doesn't
  confirm the device is the Jetson. Identity gets confirmed by logging in.
- Laptop ↔ robot network: done. ~35 min from start, ~19 of them on the
  cabling misunderstanding and a few on the sudo failure.

**15:0x: SSH login to the onboard computer worked.**
- The guessed default username/password (carried over from Unitree's other
  robots) worked on the first try. Security to-do for later: change the
  default password. Not today.
- Banner: Ubuntu 20.04.5 LTS, kernel `5.10.x-tegra`, aarch64. A tegra
  kernel means NVIDIA Jetson, so this matches a Jetson Orin NX on
  JetPack 5-era software. Identity is effectively confirmed. Will
  double-check with `hostname`.
- Finding: "Last login" shows a date of **1 Jan 1970**. The robot's clock
  is not set (no network time on an isolated subnet, and the clock likely
  doesn't keep time across power-off). Any timestamps the robot writes in
  its own logs today are unreliable. Use laptop time for this log.
- Finding: that previous login came **from the same laptop address we
  chose**. Someone (probably at the factory) used it before, which supports
  it being the vendor's intended PC address on this subnet.
- The login shell stopped at a ROS version prompt (ROS 1 noetic vs ROS 2
  foxy) and is waiting for input. Choosing foxy, because Unitree's SDK2 and
  ROS 2 bridge both use the same DDS middleware. This is a guess for the R1.
- ssh warned about no post-quantum key exchange (the robot runs an older
  OpenSSH). Not a concern on a direct cable.
- Chose foxy (1). Shell prompt came up. Hostname is the generic default.
  Not changing it.
- Banner says 242 pending OS updates. **Deliberately not applying them.**
  Updating the vendor image on the robot computer could break Unitree's
  services, and it's out of scope for today.
- **14:45: Hardware model confirmed: the board reports "NVIDIA Orin NX
  Developer Kit". GOAL 1 DONE: on the robot subnet and logged into the Jetson.**
  ~26 min since session start.
  (Unitree ships it with the dev-kit board identifier; that's normal for a carrier board.)

---

## Goal 2: read joint states (read-only)

Rule: nothing gets published to any command topic. Subscribe/read only.

> **Correction, added 14:46:** Claude estimated the times on the
> entries marked "14:5x" (×2) and "15:0x" instead of reading the clock.
> They're wrong. All of them actually happened between ~14:40 and 14:45.
> Same for the "~35 min" in the ping entry: network setup actually took
> ~22–25 min. From here on, every entry is stamped from the laptop clock.

**14:47: Looked at the Jetson home directory (`ls ~`).**
- Present: `unitree_sdk2` **and** a separate `unitree_sdk2-main` (plus its
  .zip), `cyclonedds_ws`, `unitree`. Two SDK copies means we need to find
  out which one, if either, is actually built.
- Present: drivers for three different dexterous hands (brainco, inspire,
  linker) and one named `h1_inspire_service` (**H1**, on an **R1**).
  Guess: Unitree ships one factory image shared across models, and these
  are leftovers. Not confirmed.
- Present: a stray `a.out` (an unnamed compiled program) and a zip with a
  hash for a name. Origin unknown. **Not running either.**
- `nomachine.sh` suggests remote desktop is preinstalled. Not using it.

**14:47: Both SDK copies have the same layout and both have a `build/` folder.**
- So both have been compiled at some point. Still unknown: whether they're
  the same version, and whether either has R1-specific examples.

**14:48: Example folders compared. Only `unitree_sdk2` has an `r1/` folder.**
- `unitree_sdk2-main` has every other model (a2, b2, b2w, g1, go2, go2w, h1)
  but **no r1**. Guess: `-main` is an older download from before R1
  support was added, and `unitree_sdk2` is the one shipped for this robot.
- Decision: use `~/unitree_sdk2` only. Ignore `-main`.

**14:48: `example/r1/` has two folders: `high_level` and `low_level`.**
- Joint states come from the low-level side (per-motor data). High-level
  is usually the motion service (posture/locomotion). **Not opening
  high_level today**, given the stay-supine rule.

**14:49: The only R1 low-level example is `r1_ankle_swing_example.cpp` (+ `gamepad.hpp`).**
- No read-only example ships for the R1. The one example **sends motion
  commands** (it swings the ankles).
- Guess, based on the G1 version of this example: it first drives *all*
  joints to a zero pose, then swings the ankles. That's whole-body motion,
  the opposite of "smallest motion, least consequential joint".
  **Will not run it.** Unconfirmed until the source is read.
- Plan: read the example's source (selected lines only) to learn the R1's
  topic names, message types and motor count, then write our own
  subscribe-only reader in `cas-r1/src/`.

**14:50: Read the ankle-swing example (grep only, selected lines). What it tells us:**
- **Message family: `hg`** (`unitree/idl/hg/LowState_`, `LowCmd_`,
  `IMUState_`). This is the same family the G1 and H1-2 use, so the R1
  reuses the existing humanoid message format rather than a new one.
- **Topics:** state = `rt/lowstate`, command = `rt/lowcmd`, plus a second
  IMU in the torso on `rt/secondary_imu`.
- **Init:** `ChannelFactory::Instance()->Init(0, networkInterface)` means
  DDS domain 0, bound to a named network interface.
- **Motion switcher:** creates a `b2::MotionSwitcherClient`. The namespace
  is B2 (a quadruped) inside an R1 example. The G1 example does the same,
  so this is vendor code reuse, not an R1 error. Its job is to check for
  or release Unitree's built-in motion service before sending low-level
  commands. **Relevant for goal 3, not for reading.**
- Reads `low_state.mode_machine()` and prints it as "R1 type". Our reader
  should print it too.
- Joint index names are an `enum R1JointIndex` at line 100. Reading that
  next so the numbers map to names.

**14:52: Joint index map from `enum R1JointIndex` (vendor example).**
- 0–5 left leg (hip pitch/roll/yaw, knee, ankle pitch/roll), 6–11 right
  leg, 12–13 waist (roll, yaw; no waist pitch), 14–18 left arm (shoulder
  pitch/roll/yaw, elbow, wrist roll), 20–23 right arm, 24–25 head
  (pitch, yaw). Ankles also have A/B aliases on the same indices, which
  suggests a parallel-linkage ankle like the G1.
- **⚠ Vendor-code anomaly: `RightShoulderPitch = 29`.** By the pattern it
  should be **19**. Index 19 is otherwise unused, and 29 is outside the
  0–25 range. With 29 in place the count is still 26, matching the 26 DoF,
  so 19 is the gap. Probably a typo in the example, but **not confirmed**.
  The live data decides it: if slot 19 has values and 29 is empty, it's a
  typo. Consequence if it is a typo: anything using this enum to command
  the right shoulder pitch would write to the wrong slot. **Right shoulder
  pitch is off-limits for goal 3 regardless.**

**14:52: Wrote `cas-r1/src/read_lowstate.cpp` (not yet compiled or run).**
- Subscribe-only: no publisher, no motion switcher. Prints rate, robot
  type, IMU orientation, and all 26 joints, plus slot 29 and any other
  spare slot with data. Then exits.
- Built-in sanity check: lying on its back, the IMU pitch should read
  roughly ±90°. If it reads ~0°, the data doesn't match reality.
- Needs the Jetson's network interface name. Getting that next.

**14:53: The Jetson's robot-subnet interface is named `eth10`, not `eth0`.**
- This is the interface name to pass to the SDK. It's an unusual name.
  Anyone copying G1 instructions that say `eth0` would get no data
  without any visible error.
- `docker0` exists (down), so Docker is installed on the Jetson. Not
  using it.

**14:54: The file copy went wrong. Wrong terminal, and pasted text got run as commands.**
- The copy command ran in the **robot's** shell, not on the laptop. Pasted
  terminal text, including a prompt and earlier output, also went into the
  robot shell. bash tried to run it: `docker0`, `unitree@ubuntu:~$`,
  `The` → "command not found". An apostrophe in the pasted text opened a
  quote (`>` continuation lines) that swallowed the rest.
- Nothing harmful ran. Every line either failed as "command not found" or
  ended up inside the quote. **The file was almost certainly not copied**
  (that laptop path doesn't exist on the Jetson). Unconfirmed.
- Side effect: the Jetson saved its own host key (ECDSA) to its
  known_hosts. Harmless.
- **Lesson / gotcha:** on a robot shell, pasting output back in is a
  hazard, because any line that starts with a real command word would
  run. Rule from now on: robot terminal gets typed/copied *commands only*.
  Output goes to Claude, never back into a shell. Tell the two terminals
  apart by prompt: `unitree@ubuntu` = robot, `<laptop user>@…` = laptop.

**14:55: Copy done, from the laptop tab this time.** `read_lowstate.cpp` copied to the Jetson home dir (3288 bytes, 100%).
- Next: compile it on the Jetson against `~/unitree_sdk2`. First checking
  where the SDK keeps its libraries, because the layout is assumed from
  the public repo, not seen.

**14:55: SDK layout on the Jetson matches the public repo.** Static `libunitree_sdk2.a`
(aarch64), shared CycloneDDS libs (`libddsc`, `libddscxx`), headers under
`thirdparty/include/{dds,ddsc,ddscxx}`. Next: compile with g++ directly.
No CMake project, and nothing added to the vendor SDK tree.

**14:56: `read_lowstate` compiled on the Jetson on the first try. No errors, no warnings.**
- (A stray Ctrl+C after the prompt came back. Harmless.)
- Next: run it, bound to `eth10`. Still subscribe-only, robot still supine.

**14:57: FIRST JOINT STATES READ BACK. `read_lowstate eth10` ran on the first try. GOAL 2 data in hand.**
~38 min from session start. Nothing was published; robot untouched, supine.

Top line: `1053 Hz  mode_machine=1  mode_pr=0  tick=1158180`
- **Rate ~1 kHz.** Claude predicted "~500 Hz, like the G1". That was wrong.
  Unverified alternative: two sources each publishing ~500 Hz on the same
  topic would also count as ~1 kHz. To check later.
- `mode_machine=1` (what the example calls "R1 type"). `mode_pr=0` means
  ankles are in pitch/roll mode.
- **tick puzzle:** if tick is milliseconds (assumed, as on G1), the robot
  has been up 19 min 18 s, so it booted ~14:38. But the operator said it
  was already on at ~14:26, and 14:38 is exactly when the laptop was
  cabled to the backpack. Either it restarted around then, or tick isn't
  ms. **Open question for the operator.**

IMU: `roll 12.3  pitch -60.6  yaw -9.3` (deg)
- Expected ~±90° pitch for flat on its back. Read **-60.6°**, about 30° short.
  Best guess: the backpack sits under its back and props the torso up
  ~30°, and the 12° roll means it's tipped slightly to one side. **Needs a
  visual check.** If true, "supine" here means "propped on the backpack",
  which is less stable than flat.

Motors: every live slot reports `mode 1`, near-zero velocity, |tau| ≤ 0.09 Nm,
temps 32–43 °C. Limp and cool, as expected at rest.

**⚠ MAIN FINDING: the vendor example's joint enum does NOT match the live data.**
- Enum slots with **no data**: 14 (L shoulder pitch), 20 (R shoulder roll),
  21 (R shoulder yaw).
- Slots **outside the enum with live data**: 19, 26, 29, 30.
- 26 live slots total = 26 DoF ✓, but in a different layout:
  live = 0–13, 15–19, 22–26, 29–30.
- Hypothesis (pattern match, NOT verified): this is the **G1's 29-slot
  layout** with the R1's missing joints left empty. G1 uses 12–14 waist (R1
  has no waist pitch, so 14 is empty), 15–21 left arm (R1 has 5-DoF arms,
  so 15–19 are live and 20–21, the G1 wrist pitch/yaw, are empty), 22–28
  right arm (22–26 live, 27–28 empty), and head is added at 29–30. Fits
  every live/empty slot. The labels within each group still aren't
  verified.
- Consequence: **the joint names in our own table are wrong from row 14
  on** (they came from the enum). Index 29's "typo" from earlier now looks
  like a symptom of a bigger mismatch. Vendor code and observed behaviour
  disagree. **No joint can be commanded by name until its slot is
  identified physically.**
- Slot 30 reads 110.0° with tau exactly 0.00. Odd. Possibly a head joint
  with a different zero offset. Unverified.

**14:58: Plan to find the real joint slots without sending any command.**
- Wrote `cas-r1/src/watch_lowstate.cpp` (not yet compiled). Read-only: it
  takes a baseline, watches for 15 s, and reports which slots moved >3°.
  The operator moves ONE small joint by hand while it runs.
- First target: one wrist (forearm twist). It's the leading candidate for
  goal 3's first command because it's low mass, carries no gravity load,
  and has nothing to hit. We must know its true slot first.
- Waiting on the operator: visual check of the posture (torso propped on
  the backpack?), the restart question, and that the controller is in
  damping before anyone touches the robot.

**14:59: `watch_lowstate.cpp` copied to the Jetson (2404 bytes, 100%).** Next: compile.

**14:59: `watch_lowstate` compiled cleanly.** Holding the hand test until the operator
answers: posture/stability, restart at ~14:38, controller in damping.
(Asked twice; not yet answered.)

**15:01: Operator answers.** (1) Yes, the torso is propped on the backpack, and steady.
(2) The controller is now in **damping mode**. (3) Restart at ~14:38: unknown,
left open.
- Hand test approved: LEFT wrist, twist by hand while `watch_lowstate` runs.
- **Claude's prediction, written before the test:** the vendor enum says left
  wrist roll = slot **18**. The G1-layout hypothesis says slot **19**.
  Claude predicts **19**.

**15:03: Operator asked to skip the hand test and move the joint by command.** Claude
recommended against it for now, for two reasons:
1. The true slot for the wrist is unverified (enum vs live data mismatch).
   A command to "slot 18" or "19" would move a joint we can't name.
2. **Stop-mechanism concern (guess from G1 behaviour, unverified on R1):**
   low-level commands need Unitree's built-in motion service released
   (the example's MotionSwitcherClient). On the G1, once that service is
   released, **the controller's damping button no longer stops the robot**.
   The stop becomes "kill our program", and the motor holds or goes limp
   depending on the last command. If the R1 behaves the same, the
   operator's physical stop doesn't exist during goal 3. That has to be
   settled before the first command.

**15:04: Hand test, LEFT wrist twisted ~¼ turn and back. Result:**
```
idx  start  min    max   range
15    26.1  19.3   26.1    6.8
17     9.3   9.3   49.0   39.7
18    31.0  30.6   38.1    7.5
19    21.1 -68.6   21.1   89.7
```
- **Slot 19 = left wrist (twist).** It moved 89.7°, matching a quarter turn
  (90°). **Claude's prediction (19) was right. The vendor enum (18) is wrong
  for this robot.** First direct confirmation that the example's index
  map doesn't match the wire.
- Slot 17 also moved ~40°. The forearm wasn't held fully still, so the
  twist leaked into another arm joint. Which joint 17 is stays
  ambiguous (enum says elbow; G1 layout says shoulder yaw). Slots 15 and
  18 moved ~7° each, from the same leak.
- Slot 19 ended back at its start value (max = start), so the wrist was
  returned.
- Status: goal 3 target = slot 19 (left wrist twist). Before the first
  command: read how the example releases/restores the motion service, and
  agree on the stop.

**15:05: Read the example's motion-switcher and gain lines. ⚠ Possible correction to Claude's earlier claim.**
- Line 303: `motor_cmd().at(joint_idx_in_idl[i])`. The example does
  **not** write to wire slots by enum number. It goes through a remapping
  array, `joint_idx_in_idl`. So `enum R1JointIndex` is probably a
  *logical* index (0–25), mapped to the real wire slots, which would match
  the G1-style layout we saw.
- If so, **"the vendor enum is wrong" was Claude's misreading, not a vendor
  bug.** The mismatch came from using the enum as wire slots in
  `read_lowstate.cpp`. Not confirmed until `joint_idx_in_idl` is read.
  (Claude's slot-19 prediction still holds. It came from the wire
  layout, not the enum.) The `RightShoulderPitch = 29` value is still
  unexplained.
- Motion service: the example loops `CheckMode` → `ReleaseMode` until no
  service is active (lines 194–195). **No call re-enables it** (no
  `SelectMode` in the file). Guess: after it runs, the built-in controller
  stays off until a reboot (or the app/remote re-selects a mode), so the
  remote's damping stop may stay dead after our program exits too.
  Unverified on R1.
- Gains: per-joint `Kp` (stiffness) table at line 67, `Kd` ("Damping for
  all R1 Joints") at line 77. Reading both next.

**15:06: CONFIRMED: the enum is a logical index and the vendor code is right. Claude's earlier "vendor enum is wrong" was a misreading.**
- `joint_idx_in_idl` maps logical → wire: 0–13 → 0–13, then logical
  14–18 (left arm) → wire **15–19**. So LeftWristRoll (logical 18) → wire
  **19**, matching the hand test exactly. (Right arm/head entries weren't
  shown; presumably 22–26 and 29–30.)
- The real lesson: `read_lowstate.cpp` used enum numbers as wire slots.
  That bug was ours. Its labels are wrong from row 14 on. To fix: apply
  the same mapping.
- `RightShoulderPitch = 29` in the enum is still most likely a typo (19).
  Harmless as far as we can see, since indexing goes through the table.
- Quirks: `joint_idx_in_idl` is declared `std::array<float, …>` but used as
  an index (it works via implicit conversion). The motor-error printout
  (line 232) only reports leg motors.
- `R1_NUM_MOTOR = 26`.
- **Gains Unitree uses:** arms Kp 100,100,100,100, **wrist 50**; arms Kd 2.
  (Legs 200/3, waist 300/5.) Our first command will be well under the
  wrist's 50/2.

**15:07: Operator approved the goal-3 plan,** including releasing Unitree's motion
service (and likely losing the remote's damping button until a restart).
- Plan: release service → our program holds all other joints in soft
  damping → slot 19 (left wrist) +5° over 2 s, hold 2 s, back over 2 s,
  Kp 10 / Kd 1 (Unitree uses 50/2) → damping → exit.
- Stops: Ctrl+C (sends damping before exiting), a hand on the wrist, the
  power switch.
- Next: read how the example builds the command message (incl. checksum).

**15:08: How the example builds a command (`LowCommandWriter`, lines ~291–309):**
- Sets `mode_pr` and `mode_machine` (copied from lowstate) on the message.
- For each of the 26 joints, at wire slot `joint_idx_in_idl[i]`: `mode=1`
  (enable), `tau`, `q`, `dq`, `kp`, `kd`. Unmapped wire slots stay zeroed.
- **Checksum:** `crc = Crc32Core((uint32_t*)&msg, (sizeof(msg)>>2) - 1)`,
  computed over the raw in-memory message. Then publishes to `rt/lowcmd`.
- Still needed before writing our program: where `Crc32Core` is defined
  (copy it exactly, not from memory) and how often the example sends.

**15:08: `Crc32Core` is defined in the example itself (line 134). The example sends commands every **2 ms (500 Hz)**.**
- That's half the ~1 kHz rate we measured for lowstate.
- The example also **checks the CRC on incoming lowstate** (line 222).
  Our reader didn't. Noted.
- Next: print the CRC function and the release loop (lines 134–152,
  192–200) to copy them exactly.

**15:10: Wrote `cas-r1/src/move_one_joint.cpp` (10057 bytes). Not compiled or run yet.**
- All parameters hard-coded: slot 19, +5°, Kp 10 / Kd 1, every other live
  slot kp 0 / kd 1 (damping). 2 s out, 2 s hold, 2 s back, 0.5 s damping,
  500 Hz. CRC function copied verbatim from the example. The live-slot
  list comes from today's 14:57 readout, not the enum.
- Safety gates: `--dry-run` mode (reads state, prints the plan, sends
  nothing). Pre-flight aborts if any slot has no data or the wrist is
  already moving. Requires typing `GO`. Release retries capped at 3
  (the example loops forever). Start angle re-read after the release so
  the first command matches the actual position. Runtime aborts: tracking
  error >15°, wrist speed >3 rad/s, lowstate stale >0.1 s, Ctrl+C. Every
  exit path sends 0.5 s of damping. Writes a CSV of target vs measured.
- Does NOT restore the motion service. Robot restart needed afterwards.

**15:11: `move_one_joint.cpp` copied to the Jetson (10 KB, 100%).** Next: compile.

**15:12: `move_one_joint` compiled cleanly on the Jetson, first try.** Next: `--dry-run`
(sends nothing).

**15:13: Dry run OK. Nothing sent.** `slot 19 now -68.4 deg -> target -63.4 deg`, mode_machine=1, mode_pr=0.
- The wrist is at **-68.4°, not ~21°.** It stayed where the hand test left
  it (that test's minimum was -68.6°).
- **Correction to the hand-test entry:** "slot 19 ended back at its start
  value (max = start)" was wrong. max = start only means it never went
  *above* start. The wrist was twisted to about -68° and left there. Damping
  holds whatever position it's left in.
- Direction check: +5° takes it from -68.4 to -63.4, i.e. **toward** the
  middle of the range seen (-68.6…+38.1), away from the far end. The
  joint's hard limit isn't known.

**15:14: ★ FIRST COMMAND SENT. The joint moved. GOAL 3 DONE.** ~55 min from session start.
```
motion service before release: form='0' name='ai'
t=0.0s  target -68.4  measured -68.4
t=2.0s  target -63.4  measured -65.3
t=4.0s  target -63.4  measured -65.3
t=6.0s  target -68.4  measured -65.7
end: completed. slot 19 at -65.6 deg (started -68.4)
```
- Release worked on the first try. The service that was running reported
  **name `ai`, form `0`**. Meaning unknown: probably Unitree's default
  motion/"AI" mode.
- **Out:** commanded +5.0°, got **+3.1°** (stopped 1.9° short). Then it held
  still through the 2 s hold. Claude predicted it would fall short of the
  full 5° at Kp 10 because of friction. Right.
- **Back: it barely returned.** Commanded back to -68.4°, measured -65.7°
  (moved 0.4° of 3.1°), and it ended at -65.6°. So the motion was
  **asymmetric**: it moved readily one way and hardly at all the other way,
  with similar force available (error ~1.9° out vs ~2.7° back, × Kp 10 ≈
  0.3–0.5 Nm).
  Guess (unverified): with the hand twisted ~90° from neutral, the hand's
  weight pulls the wrist toward the middle. That helped going out and
  resisted coming back. Friction from the hand resting on the mat is the
  other candidate. Either way, **Kp 10 is near the threshold where this
  joint moves at all.**
- No abort triggered. All exits went through 0.5 s of damping. CSV of
  target vs measured saved on the Jetson as `~/move_one_joint_log.csv`
  (still to be copied into the repo).
- The robot's motion service is now released. Restart needed.

**15:15: Run CSV copied into the repo: `log/2026-10-17-move_one_joint.csv`** (3250 rows, 0–6.5 s; every planned 2 ms step written).
- **The wrist didn't move for the first second.** It first moved (>0.3°) at
  t=1.02 s, when the target was already 2.6° ahead, with ~0.32 Nm of
  estimated torque. That's **stick-slip**: it sticks, then breaks free.
  Peak speed 0.59 rad/s at break-away (well under the 3 rad/s abort).
- On hold it stuck at 1.9° short with ~0.27–0.29 Nm applied.
- On the return, torque reached **-0.48 Nm** (t=6.0) without breaking free.
  So break-away **out ≈ 0.3 Nm, back > 0.48 Nm**. The asymmetry is real and
  measured, not just eyeballed. Cause still a guess (hand weight at ~90°
  twist, or mat friction).
- Takeaway for next session: at Kp 10 this joint's static friction
  (~0.3–0.5 Nm) eats ~2–3° of any small command. To track small moves,
  either raise Kp (Unitree uses 50) or command from a neutral wrist pose
  and compare.

**15:16: ⚠ DISAGREEMENT: the operator saw NO wrist movement. The sensor reports +3.1° out (and stick-slip).**
- **Retracting the 15:14 "GOAL 3 DONE".** The goal was "send one value to
  one joint and *see it move*". What we actually have: the command was
  accepted (release worked, 500 Hz stream, no aborts) and the joint
  *sensor* reported motion, but nothing visible was observed. **Goal 3:
  NOT met.** Claude's "the wrist moved" (15:14) was stated from the data
  without checking it against what the operator saw.
- Candidate explanations (all unverified):
  1. **Too small to see.** 3° of hand twist moves the edge of the hand only
     a couple of millimetres. Most likely.
  2. **Gearbox backlash / play.** The sensor may sit on the motor side of
     the gearbox, so the motor turned through slack without the output
     (the hand) turning. That would also fit the stick-slip trace.
  3. Watching at the wrong moment: nothing happened for the first ~1 s.
- Not candidate: wrong joint. The hand test showed slot 19 tracks the
  hand, in the hand→sensor direction.

**15:17: Operator declined the hand play check and asked to run the command again. Run 2 prepared.**
- Changed only: +5° → **+10°**, Kp 10 → **30** (still below Unitree's 50),
  Kd stays 1. CSV is now written to `move_one_joint_run2.csv`. All safety
  gates unchanged. From ~-65.6° this goes toward ~-55.6°, still toward the
  middle of the range.
- Peak commanded torque if the wrist didn't move at all: 30 × 0.175 rad ≈
  5 Nm. Friction measured in run 1 was ~0.3–0.5 Nm, so it should break
  free early and track.
- Backlash vs. too-small question: left open (the hand check was skipped).
  Run 2 partly answers it. If 10° is visible, "too small" is the likely
  explanation for run 1.

**15:18: Run-2 build copied and compiled cleanly.** Next: dry run.

**15:18: Run-2 dry run OK.** `slot 19 now -65.6 -> target -55.6 (+10.0), Kp 30 Kd 1`. Nothing sent.
- The wrist hasn't moved since run 1 ended (-65.6°). lowstate is still
  flowing with the motion service released.
- **Claude's prediction, written before run 2:** at Kp 30, friction
  (~0.5 Nm) leaves only ~1° of error, so it gets within ~1–2° of +10° out
  **and** comes most of the way back this time. Visible to the operator.

**15:19: ★ Run 2: the operator SAW the wrist move: "very gentle/minor". GOAL 3 MET (visible).**
~60 min from session start.
- Terminal output and run-2 CSV not yet in hand. The prediction (≈+10° out,
  most of the way back) gets scored once they are.
- Run 1 (5°, Kp 10) → sensor +3.1°, not visible. Run 2 (10°, Kp 30) →
  visible but minor.

**15:20: Run-2 CSV copied (`log/2026-10-17-move_one_joint_run2.csv`, 3250 rows, 0–6.5 s). Analysis:**
```
t=0.0 target -65.6  q -65.6
t=2.0 target -55.6  q -56.3   err +0.7
t=4.0 target -55.6  q -56.2   err +0.7
t=6.0 target -65.6  q -65.4   err -0.2
peak +9.3° of +10°, end net +0.2°, max |dq| 0.67 rad/s
```
- **Prediction scored: RIGHT.** It got within 0.7° of +10° out (predicted
  1–2°) and came back to within 0.2° (predicted "most of the way").
- Started moving at t=0.45 s (run 1: 1.02 s). Stick-slip is still there
  but much shorter.
- The asymmetry persists but is overcome: holding torque ~0.29 Nm out, peak
  ~0.38 Nm on the way back. Consistent with run 1's break-away estimates
  (~0.3 out, >0.48 back at lower speed). Cause still unknown.
- Operator's observation matches the data this time: visible, small
  (~9° of hand twist).
- Terminal output for run 2 wasn't pasted. The CSV is the record.

**15:21: Remote damping test (motion service still released): operator pressed damping. "Nothing happened."**
- Consistent with the guess (remote has no service to talk to), but
  **inconclusive**: our program left every joint in soft damping already,
  so a working damping button would also look like "nothing happened".
  To really tell them apart, query the motion service state
  (`CheckMode`) before and after pressing, or test from a pose where
  damping visibly changes something. Logged as **unverified** for next
  session.

**16:36: Session close.**
- Operator asked how to run `read_lowstate` again. Told them to read rows by
  index, because its labels are wrong from row 14 on. Power-off state at
  close: not confirmed in this log.
- Before the first commit: the laptop's local username appeared once in
  this log (the "tell the terminals apart" entry) and was **redacted** to
  `<laptop user>`. That's the only non-additive edit to this log. This
  file was renamed from `2026-10-17-session-01.md` to `01-first-contact.md`.
  The CSVs keep their names.

### Summary
- Goal 1 (reach the Jetson): done ~14:45. ~19 min lost because the laptop
  wasn't cabled, plus one silent `networksetup` failure without `sudo`.
- Goal 2 (read joint states): done 14:57. 26 live slots at ~1 kHz. Joint
  labels were wrong in our own reader (enum used as wire slots); the
  vendor's remapping table explains it.
- Goal 3 (one joint, seen to move): run 1 (5°, Kp 10) moved the sensor
  3.1°, **not visible**. Run 2 (10°, Kp 30) reached 9.3°, **seen by the
  operator**, returned within 0.2°.
- Claude errors on record: estimated timestamps (corrected), "vendor enum
  is wrong" (retracted), "wrist returned" after the hand test (wrong),
  "goal 3 done" after run 1 (retracted).

**16:50 (post-session, additive): Files reorganized into `cas-r1/sessions/01-first-contact/`.**
This log is now `log.md` there. CSVs → `data/run1-wrist-5deg-kp10.csv`, `data/run2-wrist-10deg-kp30.csv`.
Programs → `src/`, with `move_one_joint.cpp` split into `_run1` (rebuilt by reversing the 4
documented edits; 10057 bytes, matching the logged copy) and `_run2`. Paths written
earlier in this log refer to the old locations.
