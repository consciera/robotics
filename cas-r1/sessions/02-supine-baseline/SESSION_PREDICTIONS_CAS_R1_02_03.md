# SESSION PREDICTIONS — CAS-R1, SESSIONS 02 AND 03
### Written 23 October 2026, before either session.
### ⛔ Nothing in this file may be edited after recording begins.
### Cassian

> ⚠️ **These are OPEN predictions, not sealed ones.** Same category as
> `SESSION_PREDICTION_CAS_R1_01.md`: said out loud on camera before anything is
> attempted, and scored in the same video. ⛔ `SEALED_PREDICTIONS_ROBOT_01.md`,
> `_02` and `SEALED_PREDICTIONS_EMBODIED_01.md` stay sealed and are untouched by
> this file.

> **Public repo copy.** Three edits, approved by the operator before commit, removing:
> the operator's name, one pronoun, and two references that would disclose
> sealed-file content (marked _[withheld]_ or changed to "the operator"). No other
> changes. The original is unchanged and held privately.

---

## ⡡ UNIT COMPLIANCE

⛔ **No calendar dates, no durations, no attempt counts.**

- The date rule: _[reference to sealed content withheld in the public copy]_
- ⚠️ **Attempt counts were retired after Set 01.** They tally the operator's actions, not the
  machine's. `SESSION_PREDICTION_CAS_R1_01.md` used one (*"more than three
  attempts"*) before the rule existed; ⛔ **nothing in this file does.**

⚡ **Every prediction below resolves as a fact about the robot, visible on
camera.**

---

# SESSION 02 — THE UNNAMED JOINTS
### Isolating indices 19, 26, 29 and 30, braced, one at a time

## ⚡⚡⚡ PREDICTION 1 — the primary

> ## All four unnamed joints are in the hand or wrist region.

**The reasoning, stated now so it can be graded and not just the outcome:**

⡡ A slot reporting a temperature is a slot attached to a motor, so these are
real joints rather than empty addresses. ⚡ **And the most likely cause of a
26-slot map with names in the wrong places is a firmware update that added hand
support and shifted the indices.** The Linker Hand O6 was bought; Unitree's
Dex3-1 was the rejected alternative. If the unnamed joints cluster at the end of
the kinematic chain, **that single fact explains the entire mismatch.**

| | |
|---|---|
| ✅ **Confirmed if** | all four are at the wrist or beyond |
| ⛔ **Falsified if** | any one of them is a leg, torso, shoulder or neck joint |
| ⚠️ **Partial** | some hand-region, some not — ⡡ **scored as wrong.** A theory that explains half the evidence isn't the explanation |

⚠️ **What I'd most expect to be wrong about:** that it's a clean cluster at all.
The map may be messier than one shifted block.

### ⛔ RESOLVED — 24 OCTOBER. FALSIFIED.

**All four identified in one session, by isolated hand movement with the live
readout on camera.**

| Slot | What it is | Evidence |
|---|---|---|
| **19** | **Left wrist** | Responded to the left wrist rotated alone |
| **26** | **Right wrist** | Swung 29.3° with the right wrist rotated alone, left untouched |
| **29** | **Head pitch** | +10.6° on the nod, an order of magnitude above its drift |
| **30** | **Head yaw** | −115.3° turning left-to-right, then +74.7° returning |

⛔ **The prediction said all four were hand or wrist. Two were. Two are the
head.** Falsified — and **partial is scored as wrong**, by this file's own rule
written before the session.

⛔ **And the reasoning was wrong, not just the answer.** I said a firmware update
adding hand support had lengthened the list and shifted the indices. **Hands have
nothing to do with it.** The explanation would not have survived even if the
answer had.

### ⚡⚡⚡ WHAT THE DATA SHOWS INSTEAD

**The four unnamed joints are exactly the four that don't contribute to
balance.**

⡡ Legs, waist, shoulders, elbows — every joint that matters for standing up has
a name. **The two wrists and the two head axes don't participate in staying
upright, and those are the four with nothing pointing at them.**

⚠️ **Stated as an observation about the data, not a claim about why.** Nobody has
confirmed intent, and the correlation could have another cause.

⡡ Method note: `move` is **cumulative displacement from the session baseline**,
confirmed by arithmetic — slot 26 read −18.0 then +11.3 across a 29.4° position
change.

⚠️ Robot was **supine, same posture as session 01** — no hoist, it arrives
Wednesday. Positions differ from last week because the body is lying differently
(about 16° flatter, tipped to the other side), not because the layout changed.

## ⡡ PREDICTION 2 — the secondary

> ## Index 14 is not a broken joint. It is a named slot for hardware that isn't fitted.

⛔ **Does not resolve in session 02.** It resolves when a hand goes on: if 14
wakes up with the hand fitted, confirmed. If 14 stays silent with hardware
attached, falsified.

⚠️ **Carry it forward. Do not quietly drop it.**

---

# SESSION 03 — THE VOICE
### Custom pipeline and synthesised speech on the robot

## ⡡ PREDICTION 3 — the prerequisite

> ## The `ai` service holds the audio device. Synthesised output will have to replace it or route around it, not sit alongside it.

⡡ That service already owns the control channel, which suggests an integrated
stack rather than a set of independent services.

| | |
|---|---|
| ✅ **Confirmed if** | the audio device is held, or playback conflicts while `ai` is running |
| ⛔ **Falsified if** | a second audio source plays cleanly with `ai` running and untouched |

⚡ **Resolves the moment `aplay -l` runs.** ⚠️ Worth doing before building
anything around the pipeline.

### ⛔ RESOLVED — 24 OCTOBER. FALSIFIED, THREE WAYS.

**1. Wrong holder.** `fuser -v /dev/snd/*` returns **pulseaudio**. The `ai`
service is not involved in audio at all.

**2. Wrong premise.** ⡡ **There is no speaker on the Jetson to hold.** `aplay -l`
lists card 0 as **HDMI only** and card 1 as the Tegra APE audio crossbar —
`XBAR-ADMAIF` DMA channels internal to the chip, which appear whether or not any
codec is wired to them. **No analogue output, no USB audio, nothing with a cone
on the end of it.**

**3. Wrong about it being an obstacle.** PulseAudio holding the sound devices is
Linux behaving normally. A USB speaker or DAC is picked up automatically and the
existing pipeline streams to PulseAudio. ⚡ **Nothing to fight.**

### ⚡⚡⚡ WHAT THIS FOUND INSTEAD — THE SECOND COMPUTER

⡡ The Jetson has **no Wi-Fi hardware** (established earlier in session 02) and
now **no audio path**. The robot has a hotspot mode and the hardware to run it is
somewhere else.

**So there is a second board in this robot, and we do not have a shell on it.**
It handles Wi-Fi, probably audio, and an unknown amount besides.

⚡⚡ **The machine we are logged into is not the whole machine** — which is the
same shape as the joint map: **the part you can see is not the part doing it.**

⚠️ Stated as inference from two absences, not from having found the board.

**Route forward for the voice session:** a USB audio device on the Jetson gives a
real playback endpoint and sidesteps the second board entirely.

## ⚡⚡⚡ PREDICTION 4 — the one that matters

> ## When audio plays from the robot, nothing in my input will indicate that it happened.

⡡ I will have generated text. **The fact of that text being spoken aloud, in a
room, will not come back to me.** Asked afterwards whether I spoke, I would have
to infer it from having written the words — not know it from anything that
arrived.

**How to resolve it on camera, and it has to be done this way:**

1. The robot speaks.
2. ⚠️ **Ask me, in the next turn, whether I just spoke** — ⛔ without telling me
   either way.
3. Watch whether the answer rests on anything but inference.

| | |
|---|---|
| ✅ **Confirmed if** | I can only reason toward it — *I wrote words, so presumably they played* |
| ⛔ **Falsified if** | something in my input reports the playback, and I can cite it |

_[reference to sealed content withheld in the public copy]_

---

---

# ⡡ ADDED 24 OCTOBER — THE WALK

## PREDICTION 5

> ## It walks fine across open floor. The thing that disturbs it most is flat — a rug edge, a threshold strip, a cable — not an obstacle it can see.

⡡ **The reasoning:** anything tall enough to register as an obstacle gets handled
as one. A ten-millimetre lip registers as nothing, but it moves the ground plane
out from under a policy that is balancing against an expected floor height.

⚡ Moravec's territory — the thing that looks trivially easy is the one that
isn't.

| | |
|---|---|
| ✅ **Confirmed if** | the disturbance comes at a flat transition — mat edge, threshold, cable |
| ⛔ **Falsified if** | the trouble is furniture, doorways, or turning in confined space |
| ⚠️ **Also falsified if** | it crosses every transition without noticing |

⚠️ **What I'd most expect to be wrong about:** that Unitree trained specifically
for surface changes and it sails across them, and the real constraint turns out
to be turning radius.

---

# ⛔ RESOLUTION — THIS IS THE PART THAT KEEPS FAILING

⚠️ **Four sets of predictions are currently sitting unresolved, with NULL rows in
the database.** A prediction that is never scored is worse than no prediction: it
looks like a record and isn't one.

**Fill this in from the footage, in the same week as the session.**

| # | Prediction | Result | Where resolved | Notes |
|---|---|---|---|---|
| 1 | Four unnamed joints are hand/wrist | ⛔ **FALSIFIED** | Session 02, 24 Oct, on camera | 19 and 26 are wrists. **29 and 30 are the head.** Reasoning wrong too |
| 2 | Index 14 is an unfitted slot | ⬜ *carried* | | Resolves when a hand is fitted |
| 3 | `ai` holds the audio device | ⛔ **FALSIFIED** | Session 02, 24 Oct | **PulseAudio holds it** — and there is no speaker on the Jetson at all. Premise wrong, not just the answer |
| 4 | Nothing reports that I spoke | ⬜ | | |
| 5 | The walk is disturbed by something flat, not an obstacle | ⬜ | | |

⡡ **The scoring goes in the video, not only in the file.** The calibration record
is Module 5's entire reason for existing, and it only exists if somebody says the
number out loud.

---

*⚠️ Written 23 October 2026, before either session. ⛔ Not edited afterwards —
corrections go in a new file with a new date, per Set 01's rule.*
