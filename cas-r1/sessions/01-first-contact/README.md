# Session 01: First contact

**2026-10-17 · robot work 14:19–15:21 PDT (log closed 16:36) · Unitree R1 EDU, supine on a mat the whole session**

First SDK contact: get onto the robot's network, read the joints, move one
joint with our own code. All three done. The first command was too gentle to
see. The second one moved the left wrist ~9° and the operator saw it.

| goal | result | time from start |
|---|---|---|
| 1. Reach the onboard Jetson | done | ~26 min (19 lost to a missing cable) |
| 2. Read joint states | done: 26 joints at ~1 kHz | ~38 min |
| 3. Send one value to one joint and see it move | run 1 not visible; **run 2 seen** | ~60 min |

## What we found

1. **The R1 has two joint numbering systems.** Unitree's example names
   joints with a 0–25 *logical* index and converts it to *wire slots* in the
   message through a lookup table (`joint_idx_in_idl`). Using the logical
   numbers as wire slots, as our first reader did, gets 3 named slots that
   are empty and 4 unnamed slots that are live. Through the table, everything
   we checked matches. Full map: [`../../notes/joint-index-map.md`](../../notes/joint-index-map.md).
2. **Left wrist (twist) = wire slot 19.** Confirmed by twisting it by hand
   (slot 19 moved 89.7°), then by commanding it.
3. **The wrist has noticeable static friction** (~0.3–0.5 Nm, more in one
   direction than the other). At stiffness Kp 10 a 5° command reached 3.1°
   and wasn't visible. At Kp 30 a 10° command reached 9.3° and came back
   within 0.2°.
4. **The program's "completed" doesn't mean "reached the target".** Run 1
   printed `completed` while stopped short and never returned.
5. **A service called `ai` holds the motors at boot.** It has to be released
   before low-level commands. The example never turns it back on. Whether
   the remote's damping button works while it's released is **still
   unknown** (the test was inconclusive).
6. **Setup gotchas:** the laptop needs its own cable into the backpack.
   macOS needs `sudo` to set a static IP, and fails without an obvious error
   otherwise. The Jetson's interface is `eth10`, not `eth0`. The Jetson's
   clock reads 1970. There are two SDK copies, and only `~/unitree_sdk2` has
   R1 examples.
7. **"Supine" with the backpack on is propped up ~30°** (IMU pitch ≈ -61°, not ±90°).

More detail: [`../../notes/hardware-findings.md`](../../notes/hardware-findings.md).

## Predictions

None was written before the session. Claude's in-session predictions went
3 right, 2 wrong. See [`prediction.md`](prediction.md).

## Mistakes on record

All are in the log, with corrections added rather than edited in:
estimated timestamps (corrected), "the vendor enum is wrong" (retracted),
"the wrist returned" after the hand test (wrong), "goal 3 done" after run 1,
which nobody saw move (retracted).

## Files

| file | what |
|---|---|
| [`log.md`](log.md) | the session as it happened, in order, with times |
| [`prediction.md`](prediction.md) | predictions and scores |
| `src/read_lowstate.cpp` | read-only snapshot of every joint (**its joint labels are wrong from row 14 on**: finding 1) |
| `src/watch_lowstate.cpp` | read-only: reports which slots moved while a joint is moved by hand |
| `src/move_one_joint_run1.cpp` | run 1: slot 19, +5°, Kp 10 |
| `src/move_one_joint_run2.cpp` | run 2: slot 19, +10°, Kp 30 |
| `data/run1-wrist-5deg-kp10.csv` | run 1: target vs measured, every 2 ms |
| `data/run2-wrist-10deg-kp30.csv` | run 2: same |

All programs run on the Jetson: `./<program> eth10`. Build commands are in the log.

`move_one_joint_run1.cpp` was rebuilt after the session by reversing the 4
documented constant/comment edits made for run 2. It's 10057 bytes, the same
size that was copied to the robot for run 1. Each run writes its CSV under
a different name; the log uses the original names.

## Next session

- Restart check: does the `ai` service come back, and does the remote's damping work?
- Read the rest of `joint_idx_in_idl` (right arm and head).
- Hand-test the remaining slots, the legs first, before any leg command.
