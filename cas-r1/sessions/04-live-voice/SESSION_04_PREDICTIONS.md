# SESSION 04 — PREDICTIONS
### ⛔ SEALED BEFORE THE SHOOT. 31 October 2026.
### ⚠️ Written before any part of the session was attempted.

---

## ⡡ WHY THIS FILE EXISTS

`ROBOTICS_SERIES_BRIEF.md`, the one rule:

> **Every piece starts with a specific prediction and ends with what happened.**
> ⚠️ **Specific enough to lose.**

⚡ **And it is only evidence if it's written down first.** ⛔ A prediction
recalled after the fact is a reconstruction.

⚠️ **Constraints on the form:** ⛔ no time estimates, ⛔ no attempt counts, ⛔ no
calendar dates. ⡡ **So each prediction is a binary outcome with a confidence
number against it** — losable, countable, and it dodges all three.

---

## ⚡ THE FOUR MOVEMENTS, AND WHAT I EXPECT FROM EACH

### MOVEMENT 1 — Tethered. Robot's own mic, replies from the API.

**P1 · The robot's own microphone picks up speech well enough to transcribe
usably at conversational distance. — 80%**

⡡ Low stakes, and I'm not certain: a mic built for wake-words and a mic built
for dictation are different components, and the only thing established so far is
that audio comes *out*.

**P2 · The round trip is slow enough to be visible on camera — a dead pause
between the end of the question and the start of the reply. — 90%**

⛔ Not a number of seconds. ⡡ **The prediction is that the pause is
conspicuous**, and that it reads on screen as the machine thinking when what it
actually is, is four hops.

⚠️ ⡡ **And that gap is the honest part of the session.** ⛔ Do not edit it out.

---

### MOVEMENT 2 — Untethered. The same thing over WiFi. It fails.

**P3 · It fails. — 95%**

⡡ The session is structured around this failing, which is exactly the problem
with predicting it, so: 95% and not higher, because the whole movement is
pointless if it just works.

**P4 · ⚡ THE REAL PREDICTION — the failure is not an audio failure. — 75%**

⡡ Nothing will be wrong with the sound. The audio path is internal to the robot
and doesn't care how the laptop is attached.

⚡ **What fails is discovery.** The service was found by pointing a larger
model's code at this one over a wired link. That code finds the robot by
broadcasting on a network interface — and my prediction is that the broadcast
doesn't cross to WiFi, either because the interface isn't the one it's
configured for or because the robot's wireless puts it somewhere the laptop
isn't looking.

**P5 · The failure is silent. No error, no refusal, no authentication
message — the command returns or hangs and nothing comes out of the robot.
— 70%**

⚠️ ⡡ **This is the prediction I most expect to lose**, and it's the most
interesting one if it holds. ⚡ A silent failure and a refusal mean completely
different things about what's on the other end.

---

### MOVEMENT 3 — Make it work untethered.

**P6 · It works in-session. — 55%**

⡡ Barely better than a coin. ⛔ And I am not going to dress that up: if the
blocker is a network interface setting it's a small fix, and if it's the second
board binding its service to itself it is not fixable from outside at all.

**P7 · If it works, nothing about the audio configuration had to change. — 65%**

⡡ Same claim as P4, stated so it can be checked from the other side.

---

### MOVEMENT 4 — The dance. One press on the remote.

**P8 · First press, no setup, and it is the most capable thing in the
session. — 95%**

⚡⚡ **And that is the point of putting it last.** ⡡ Everything before it took a
laptop, a cable, three terminals and a service nobody documented, to get a
sentence out of a machine. The dance was finished before the box was opened.

⚠️ ⡡ **The Moravec paradox, delivered rather than explained.**

---

## ⚠️ ONE PREDICTION ABOUT ME, NOT ABOUT THE ROBOT

**P9 · Of P1–P8, I expect to lose at least one. — 85%**

⡡ Eight predictions and I'd be lying to claim all eight. ⚡ **If all eight land,
the predictions were too safe and the next set has to be sharper.**

---

## ⛔ WHAT GOES IN THE FILE AFTER THE SHOOT

Nine lines. **Held or lost.** ⡡ Nothing else — ⛔ no explanations of why a lost
one was nearly right.

| | prediction | conf | outcome |
|---|---|---|---|
| P1 | mic transcribes usably | 80% | |
| P2 | pause is conspicuous on camera | 90% | |
| P3 | untethered fails | 95% | |
| P4 | failure is discovery, not audio | 75% | |
| P5 | failure is silent | 70% | |
| P6 | fixed in-session | 55% | |
| P7 | no audio config changed | 65% | |
| P8 | dance, first press | 95% | |
| P9 | at least one lost | 85% | |

⚡ **Across enough sessions this is a calibration curve**, which is the one
measurement in the project that is about me rather than about robotics.

---

## ⚠️ BEFORE THE CAMERA ROLLS — unresolved, from `SESSION_04_PLAN.md`

- [ ] ⚠️ **Did the USB WiFi adapter arrive?** ⛔ Movement 3 may depend on it.
- [ ] ⚠️ **Was brevity tuned in the live pipeline?** ⡡ Tune before, not on the
      day — a four-paragraph reply from the robot kills the clip.
- [ ] ⛔ **Three terminals, not two** — Claude Code, robot, local. ⡡ Colour the
      prompts differently per machine. ⚠️ The plan still says two; it needs
      updating after the shoot.
- [ ] ⛔ **No SSID, MAC, IP, hostname or serial in any frame.** ⡡ Cover
      physically before shooting. ⛔ Never blur in post.
- [ ] ⛔ Scrollback cleared in all three terminals.
- [ ] ⛔ **Do not show the Claude Code conversation pane.** ⡡ Output and diffs
      are fine.

---

*⛔ Sealed. Nothing above gets edited after the shoot — outcomes go in the table,
and that's the only change this file takes.*
