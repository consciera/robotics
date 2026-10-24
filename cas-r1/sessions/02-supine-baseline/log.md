# Session 02: supine baseline · 2026-10-24

Hardware: Unitree R1 EDU, same unit as session 01.
**Posture this session: SUPINE, no hoist (see the correction at 11:53 below; the brief said "upright and suspended").** Session 01
was supine on a mat, propped ~30° on the backpack (IMU pitch ≈ -61°). It
was not "horizontal on a table". Corrected from the session brief; see the
session 01 log at 14:57 and 15:01.

Goals, in order: (1) confirm the physical link; (2) check the Jetson clock,
the network mode, and what holds the control channel; (3) capture a
baseline of every live index before anything moves; (4) set up the live
on-camera display.

Prediction for this session: _none in `prediction.md` at session start
(11:50)._

Times are local (PDT), from the laptop clock. Entries are append-only.

---

## Running log

**11:50: Session 02 started. Session folder created.**

**11:50: Step 1: physical link, checked from the laptop *before* anything else.**
- USB Ethernet adapter: `active`, 1000baseT full-duplex.
- The static address from session 01 **persisted** across the week. The
  saved config is still Manual, so no re-setup was needed.
- Link light ≠ traffic, so the Jetson was pinged: 4/4 replies, avg 0.64 ms,
  0% loss. **Traffic is actually flowing.** The 19-minute failure from
  session 01 didn't recur.

**11:51: Things flagged before going further:**
- **Posture changed: supine → upright, suspended.** Session 01's whole
  safety case was "supine, so damping is a safe stop". Upright, damping
  is safe only if the hoist carries the full weight. Asked the operator
  to confirm the hoist setup. The IMU in the baseline will independently
  confirm "upright" (pitch should read near 0°, not -61°).
- The baseline is read-only. Nothing is released (no `ReleaseMode`) this
  session unless explicitly decided. Releasing the `ai` service while
  upright could drop the limbs under gravity.
- "AI features unavailable in AP mode" in the notes is **operator-reported**
  (Explore app), not observed. This session tries to observe the network
  mode directly.

**11:52: Wrote `src/baseline_lowstate.cpp` (not yet compiled).**
- Read-only: no publisher, no ReleaseMode/SelectMode. One run covers
  step 2 (Jetson clock, control-channel owner via `CheckMode`, a query)
  and step 3 (every slot 0–34: live flag, position averaged over 0.5 s +
  spread, velocity, torque, temp, mode; IMU; summary of 19/26/29/30).
- The posture note is a required argument, so it's written into the
  file **at capture time**.
- No joint names in the output, only indices, because the session 01
  labels were wrong. Names live in `notes/joint-index-map.md`.
- Plan: run it over ssh *from the laptop*, redirecting into the repo, so
  the filename timestamp comes from the laptop clock (the Jetson's reads 1970).
- Connection method reused from session 01: wired link, static address,
  `scp`/`ssh` with password, `eth10`, the g++ line from the session 01 log.

**11:53: Shell hygiene applied on the laptop (`PS1='$ '`, robot address in `$R1`, scrollback cleared). `baseline_lowstate.cpp` copied (4756 bytes, matches local).**
- As predicted: the scp password prompt still prints `user@<address>`. The
  shell variable doesn't hide it. Workaround: do logins before recording,
  then clear. Hoist confirmation still pending from the operator.

**11:53: ⚠ CORRECTION from the operator: the robot is SUPINE. There is NO hoist yet (it arrives Wednesday).**
- The session brief's "upright and suspended" was wrong. Every
  posture-related entry above that assumed it (the 11:51 flag, the IMU
  expectation "near 0°") is superseded. Expect pitch ≈ -61° as in session 01.
- Safety envelope reverts to session 01: supine, controller damping is
  the stop. No standing or balance, nothing that lifts it.
- Upside for the baseline: same posture as session 01, so differences
  from session 01 readings are **not** explained by posture (unless it's
  propped differently on the backpack; the IMU will tell).
- Session folder renamed `02-suspended-baseline` → `02-supine-baseline`
  (not yet committed). The copy of `baseline_lowstate.cpp` on the Jetson
  is unaffected.

**11:54: ssh into the Jetson OK. ROS prompt → 1.** ⚠ Surprise in the login banner:
- "Last login: **Sun Oct 18 06:17:51 2026**". Last week the same line read
  1970. So the Jetson's clock has a **real 2026 date** at least as of that login.
- That login matches session 01's last ssh/scp (the run-2 CSV copy,
  ~15:17–15:20 PDT Oct 17) **if the Jetson's timezone is UTC+8**:
  15:17 PDT = 22:17 UTC = 06:17 next day at UTC+8. Guess: the clock got set
  *during* session 01, and the Jetson uses a China-time zone. Unverified.
- How it got set, with no internet on the robot subnet, is unknown. Possibly
  the robot's other board or the app via Wi-Fi. `date` next.

**11:55: Step 2a, Jetson clock: `Sun 25 Oct 2026 02:55:13 AM CST`.**
- Laptop at the same moment (to within the copy/paste delay of a few seconds):
  11:55 PDT = 02:55 at UTC+8. **The Jetson clock is CORRECT now**, and its
  timezone is CST = China Standard Time (UTC+8). That confirms the 11:54
  guess.
- **Changed since session 01** (1970 then). It kept correct time across a
  week, including the power-off at the end of session 01. So it either
  has a working RTC once set, or it syncs at boot from somewhere. Which
  one is unknown.
- Robot-side timestamps are now usable but in UTC+8. Convert with −15 h
  to PDT.
- Operator's prompt now `$ ` on the robot too.

**11:56: Step 2b, network mode. Ran `ip -br link` + a list of wireless interfaces on the Jetson.**
- Interfaces: `lo`, `dummy0` (down), `eth10` (up), `docker0` (down).
  **No wireless interface on the Jetson at all.**
- So the robot's Wi-Fi (and its AP vs client mode) **is not visible from
  the Jetson**. Inferred: it's handled by another board in the robot.
  Network mode is **not determined** this session. To check it from the app
  side, or find which board runs Wi-Fi.
- **⚠ Claude error (on camera):** `ip -br link` prints each interface's
  **MAC address**, which counts as identifying. Claude chose that command
  without filtering it. The MACs are on screen/in scrollback for that take.
  They are **not** recorded in this log. Operator to cut that part from the
  footage and clear scrollback. Future commands filter the output first.

**11:56: `baseline_lowstate` compiled cleanly on the first try**, including the new
`CheckMode` query. (The scrollback with the MACs is still on screen.
Reminded the operator to clear it.)

**11:57: Step 3: baseline captured on the Jetson (`~/baseline.txt`). Nothing moved or commanded before it.**
- Posture note recorded in the file at capture: "supine on mat, propped on
  backpack, no hoist yet (arrives Wednesday), same posture as session 01,
  nothing moved or commanded yet this session".
- **Live slots (26): 0–13, 15–19, 22–26, 29, 30. Identical to session 01.**
- **Watched: 19, 26, 29, 30 all LIVE.**
- Header (clock, control-channel owner, IMU, rate) to be read once the
  file is copied to the laptop.

**11:58: Dead end: copying the baseline failed. `cp: :~/baseline.txt: No such file or directory`.**
- The scp ran in a **different laptop tab** where `R1` was never set (its
  prompt was still the full default, not `$ `). An empty `$R1` turned the
  source into `:~/baseline.txt`, so scp treated it as a local file and
  handed it to `cp`, which failed. Harmless; nothing reached the robot.
- Lesson: the shell variable lives per tab. Every new tab needs the setup
  line. That tab's default prompt also shows the laptop's user/host name,
  which is identifying if it's on camera.

**11:58: Baseline file copied to the laptop: `data/baseline-20261024-115828.txt` (1787 bytes; the filename has the laptop's time *of the copy*).**

Header, as captured on the Jetson:
```
jetson clock: 2026-10-25 02:57:03        (= 11:57:03 PDT, correct)
control channel: form='0' name='ai'
lowstate: 1052 Hz  mode_machine=1  mode_pr=0  tick=427827
imu rpy deg: roll -18.3  pitch -76.4  yaw 12.0
live slots (26): 0-13, 15-19, 22-26, 29, 30
watched: 19=LIVE 26=LIVE 29=LIVE 30=LIVE
```
- **Control channel: the `ai` service holds it again** (same form/name as
  session 01). It was released at the end of session 01 and the robot was
  powered off. **Observed: a power cycle restores the `ai` service.**
  (Whether the remote's damping works while released is still untested.)
- **Rate 1052 Hz, mode_machine 1, mode_pr 0: unchanged from session 01.**
- **tick 427827.** If it's in ms, the robot booted 7 min 08 s before the
  capture, i.e. ~11:49:55 PDT, just before this session started at 11:50.
  If the operator confirms they powered on ~11:50, that supports
  **tick = milliseconds**. That would also mean session 01's robot really did
  (re)boot ~14:38. Asking.
- **⚠ IMU contradicts the posture note.** The note says "same posture as
  session 01". The IMU says roll **-18.3** (s01: +12.3), pitch **-76.4**
  (s01: -60.6), yaw 12.0 (s01: -9.3). The torso is ~16° flatter and tipped
  to the **other** side. Still supine and propped, but not the same pose.
  Captured as-is; the note isn't edited.
- **Joint positions vs the session 01 baseline (14:57):** 20 of 26 live
  slots differ by >10°. Biggest: slot 26 +88°, slot 29 +54°, slot 2 -51°,
  slot 25 +44°, slot 18 +32°. Slot 19 = -96.8° (session 01 ended at -65.4°
  after run 2, so it moved another ~31° since). **Inferred:** the limp robot
  was handled/repositioned between sessions. These differences are posture,
  not the map. The map itself (which slots are live) is **identical**.
- **Temperatures 25–32 °C** (s01: 32–43 °C). Cooler, consistent with ~7 min
  since boot vs ~19 min in s01.
- **Position spread over 0.5 s (527 samples) is 0.00° on every slot.**
  The robot is perfectly still, or the encoders read noise-free at rest.
  Worth noting. Session 01's one-shot readout had small nonzero velocities.
  The live display will show whether values update when a joint moves.

**11:59: Step 4: wrote `src/live_display.cpp` (not yet copied or compiled).**
- Read-only. Redraws 10×/s. Default slots 19, 26, 29, 30; any slots can be
  passed as arguments. Shows only: slot, degrees, movement since the
  display started. No joint names (unconfirmed names don't go on camera).
  Font size comes from the terminal's zoom, not the program.

**12:00: `live_display.cpp` copied (2343 bytes, matches local).** Next: compile on the Jetson.

**12:00: `live_display` compiled cleanly.** (Both prompts are now `$ `. A clean compile itself confirms
this was the robot tab: the laptop has no Unitree SDK, so it would have failed there.)

**12:03: Live display running on the robot tab. Shows 19 = -96.8, 26 = 98.8, 29 = 23.3, 30 = 115.0, all "move +0.0".**
- **Matches the 11:57 baseline exactly** on all four slots. Nothing has
  moved since the baseline, and the display reads the same stream.
- Not yet shown: that the values *update* when a joint moves (the 0.00
  spread question). That needs motion, and nothing is being moved this step.
- Legibility at phone size: not yet confirmed by the operator.
- **Session brief steps 1–4 complete.** Nothing released or commanded this
  session so far.

**12:05: Operator moved "the wrist" by hand with the live display running.**
```
slot   deg    move
 19   -38.5  +58.2
 26    80.8  -18.0
 29    23.8   +0.5
 30   115.3   +0.3
```
- **The display updates live. Confirmed.** So the 0.00° spread in the
  baseline meant the robot was still, not that the stream was frozen.
- Slot 19 +58.2°: consistent with the left wrist twist (as in session 01).
- **⚠ Surprise: slot 26 moved -18.0° too.** In session 01, twisting the left
  wrist moved 15/17/18/19, **not 26** (26 stayed under 3°). Slot 26 is
  *inferred* to be the right wrist. Unresolved. Possibilities: the operator
  touched/steadied the other arm, the body shifted on the backpack (29/30
  moved ~0.3–0.5°, a hint the body moved slightly), or slot 26 isn't what
  the pattern suggests. Asked the operator which wrist and whether the
  other arm was touched.
- The display doesn't show 15–18, so arm leakage on the left side isn't visible here.

**12:05: Operator: they twisted BOTH wrists, left and right.** The slot 26 surprise is resolved.
- Slot 26 moved (-18°) when the **right** wrist was twisted. That fits the
  inference **slot 26 = right wrist**, the first observation supporting it.
- Not a clean confirmation: both wrists moved together. But session 01
  showed 26 does **not** respond to the left wrist, so the right wrist is
  the only candidate here. Status for the map: **26 = right wrist, supported
  by observation (not isolated)**. One right-wrist-only twist would confirm it.
- Slot 19 = left wrist: consistent again (+58°).

**12:19: Pre-commit identifier scan of every session 02 file + the notes change.**
- Searched for MAC addresses, IPv4 addresses (incl. the laptop's static
  address and the Jetson's), link-local addresses, hostnames (incl. the
  robot's default), SSIDs, serial numbers, the laptop username/host, and
  the adapter model.
- **Found none.** The MACs that went on screen at 11:56 were never written
  into any file. This log deliberately described them without copying
  them. The baseline file contains only slot data, the posture note, the
  Jetson clock and the mode/IMU header.
- Kept: `eth10` (an interface *name* the SDK needs, the same on this robot
  model, not unique to this unit) and the robot's timezone (a vendor
  setting).
- Nothing stripped, so nothing regenerated.

**12:23: Added after the fact. Isolated hand tests, OPERATOR-REPORTED from the footage. Claude didn't see these readings live.**
After the both-wrists twist, the operator ran isolated tests on camera with
the live display. The values come from the operator's predictions file
(written ~12:16), not from Claude's session. Exact times not recorded.
| slot | test | reading (live display, "move" column = change since display start) |
|---|---|---|
| 26 | right wrist rotated alone, left untouched | swung 29.3°; display went -18.0 → +11.3 |
| 29 | head nod | +10.6°, an order of magnitude above its drift |
| 30 | head turned left→right, then back | -115.3°, then +74.7° on return |
| 19 | left wrist rotated alone | responded (no value recorded) |
- If accurate: **26 = right wrist (isolated), 29 = head pitch, 30 = head yaw.**
  That matches the vendor's logical names via `joint_idx_in_idl` (assumed
  mapping for logical 23–25 → wire 26, 29, 30. Those table entries still
  haven't been read).
- Status in the map: **operator-reported, from footage.** Not observed by Claude.

**12:23: Predictions file `SESSION_PREDICTIONS_CAS_R1_02_03.md` (operator/Cassian, written 23 Oct) added to this session folder.**
- The repo copy has three edits, approved by the operator, removing: the
  operator's name, one pronoun, and two references that would disclose
  content from sealed prediction files. Nothing else changed.
- Note for the record: the file's Prediction 1 premise ("unnamed joints",
  "names in the wrong places") rests on reading the vendor enum as wire
  slots. Session 01 retracted that reading (log 15:06): the enum names all
  four joints through `joint_idx_in_idl`. The file is left as written.

**12:26: Prediction 3 check requested (does `ai` hold the audio device?).**
- The prediction was committed in b72cec1 *before* this check, so a
  resolution committed afterwards fits the before/after convention.
- Method caveat: `aplay -l` only lists audio devices. It can't show who
  holds one. Plan: `aplay -l` (what exists), then `sudo fuser -v /dev/snd/*`
  (which processes have the device open). `sudo` is needed because the
  service likely runs as root, and without it fuser would wrongly show
  nobody.

**12:27: `aplay -l` on the Jetson.** Only NVIDIA's built-in audio:
- card 0 "HDA": 4 HDMI outputs (HDMI 0–3).
- card 1 "APE": 22 internal Tegra audio-routing endpoints (XBAR/ADMAIF, ADSP).
  These exist on every Orin and aren't a speaker by themselves.
- **No USB audio device, and no card that looks like the robot's speaker
  or microphone.** Inferred (not confirmed): the robot's speaker/mic hang
  off another board, like the Wi-Fi (see 11:56). If so, Prediction 3 can't
  be resolved from the Jetson. The device it's about isn't here.
- Camera note: ~100 lines of output. Claude should have filtered it
  (e.g. `aplay -l | grep ^card`). A blob on screen.

**12:27: `sudo fuser -v /dev/snd/*`, process names only: `pulseaudio`.**
- The only thing holding the Jetson's audio devices is **PulseAudio**, the
  standard Linux sound server. Holding devices is its normal job; it lets
  several programs play at once through it. **Nothing named `ai` holds
  Jetson audio.**
- **Prediction 3 status: UNRESOLVED.** No robot speaker/mic device exists on
  the Jetson (only HDMI + internal routing), so the device the prediction
  is about isn't here. What we can say: on the Jetson, audio goes through
  PulseAudio, which allows sharing, not exclusive ownership by `ai`.
  Still unknown: whether the `ai` service even runs on the Jetson.
- To resolve: find the board that drives the speaker, or test playback
  with the `ai` service running.

**12:29: Predictions file updated by the operator/Cassian (v3): Prediction 3 scored FALSIFIED ("three ways").** Repo copy refreshed, with the same three privacy edits. Only additions vs the committed copy.
- **Claude's view, for the record. Disagrees with the scoring.** The file's own
  criteria, written beforehand: confirmed if "the audio device is held, or
  playback conflicts while `ai` is running"; falsified if "a second audio
  source plays cleanly with `ai` running and untouched". **No playback was
  tested**, and the device the prediction is about (the robot's speaker)
  **wasn't found on the Jetson**. What was observed: PulseAudio holds the
  Jetson's HDMI/internal audio devices. That falsifies "`ai` holds the
  *Jetson's* audio devices", but not the prediction as written. Claude's
  score: **unresolved (premise not testable from the Jetson)**.
- Specific overstatements in v3 (observed vs claimed):
  - "The `ai` service is not involved in audio at all": only shown for the
    Jetson's devices. Whether `ai` runs on the Jetson is unknown.
  - "A USB speaker or DAC is picked up automatically… nothing to fight":
    a plan, not yet tested.
  - "Second board": inferred from two absences (Wi-Fi, audio). The file
    says so itself.
- The file is committed as the operator wrote it. This dissent is in the log.

**12:33: Session 02 ends here. The same sitting continued as session 03 (the voice work), logged in `../03-voice/log.md`.**
