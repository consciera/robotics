# R1 joint index map

As of session 01 (2026-10-17). The live state stream is the ground truth; vendor code is a source to check against.

## The short version

The R1 has **two numbering systems**, and confusing them is easy:

- **Logical index (0–25).** The `enum R1JointIndex` in Unitree's
  `example/r1/low_level/r1_ankle_swing_example.cpp`.
- **Wire slot (0–34).** The position in `LowState_.motor_state[]` /
  `LowCmd_.motor_cmd[]` in the `hg` messages actually on the wire.

The example converts between them with a table, `joint_idx_in_idl[]`. **If
you use the enum values directly as wire slots, they don't match the
robot.** Three named slots are empty and four unnamed slots are live. We made
exactly that mistake in session 01 (`src/read_lowstate.cpp`). Through
`joint_idx_in_idl[]`, every mapping we've checked matches.

## What the live stream shows (observed)

Read with `src/read_lowstate.cpp`. The robot was supine, at rest.

- **26 wire slots report data:** 0–13, 15–19, 22–26, 29–30.
- **Empty (all zeros, mode 0):** 14, 20, 21, plus 27–28 and 31–34.
- Under the raw enum: 14 (`LeftShoulderPitch`), 20 (`RightShoulderRoll`) and
  21 (`RightShoulderYaw`) report **no data**. 19, 26, 29 and 30 have **no
  enum name** but are **live**.

## What the vendor code says

`enum R1JointIndex` (logical):

| logical | name | logical | name |
|---|---|---|---|
| 0 | LeftHipPitch | 13 | WaistYaw |
| 1 | LeftHipRoll | 14 | LeftShoulderPitch |
| 2 | LeftHipYaw | 15 | LeftShoulderRoll |
| 3 | LeftKnee | 16 | LeftShoulderYaw |
| 4 | LeftAnklePitch (= LeftAnkleB) | 17 | LeftElbow |
| 5 | LeftAnkleRoll (= LeftAnkleA) | 18 | LeftWristRoll |
| 6 | RightHipPitch | **29 (sic)** | RightShoulderPitch |
| 7 | RightHipRoll | 20 | RightShoulderRoll |
| 8 | RightHipYaw | 21 | RightShoulderYaw |
| 9 | RightKnee | 22 | RightElbow |
| 10 | RightAnklePitch (= RightAnkleB) | 23 | RightWristRoll |
| 11 | RightAnkleRoll (= RightAnkleA) | 24 | HEAD_PITCH |
| 12 | WaistRoll | 25 | HEAD_YAW |

`RightShoulderPitch = 29` breaks the sequence. By position it should be 19.
Probably a typo. *Inferred, not confirmed.*

`joint_idx_in_idl[]` (logical → wire), **only the part we read**:
logical 0–13 → wire 0–13; logical 14–18 → wire **15–19**. The entries for
logical 19–25 weren't printed in session 01.

## Wire-slot map

| wire slot | joint | status |
|---|---|---|
| 0–11 | legs, in enum order (L hip pitch/roll/yaw, knee, ankle pitch/roll; then R) | **inferred.** The table maps them 1:1, but they haven't been moved/tested |
| 12, 13 | waist roll, waist yaw (enum order) | **inferred** |
| 14 | (empty) | **observed** empty |
| 15–18 | L shoulder pitch, roll, yaw, L elbow | **inferred** from the table. 15, 17 and 18 moved during the wrist hand test (leakage); which is which isn't tested |
| **19** | **L wrist roll (twist)** | **CONFIRMED.** Hand twist moved slot 19 by 89.7°. Commanded moves on slot 19 turned the left hand (run 2, seen by the operator) |
| 20, 21 | (empty) | **observed** empty |
| 22–26 | R arm (shoulder pitch, roll, yaw, elbow, wrist roll) | **inferred** from the pattern only. The table entries weren't read |
| 27, 28 | (empty) | **observed** empty |
| 29, 30 | head pitch, head yaw | **inferred** from the pattern only. Slot 30 reads 110° at rest with exactly 0 torque, which is unexplained |
| 31–34 | (empty) | **observed** empty |

The empty-slot pattern matches the **G1's 29-slot layout** with the R1's
missing joints left out: waist pitch (14), wrist pitch/yaw (20–21, 27–28),
and the head added at 29–30. *Inferred.*

## Still to identify

- The rest of `joint_idx_in_idl[]` (logical 19–25). One `sed` on the example.
- A physical test (hand-move, `src/watch_lowstate.cpp`) for every slot except
  19. The legs especially, before any leg command.
- What slot 30's 110° rest reading means.
