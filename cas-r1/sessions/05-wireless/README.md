# Session 05: wireless, and senses (2026-11-21)

Session 05 is the LLM integration. Part 1 (cabled) is in
[`04-live-voice`](../04-live-voice/), filed before the numbering was settled.
This folder is part 2.

**Goal:** talk to Cassian through the robot with **no cable attached**.

**Result:** ✅ **Done.** A USB Wi-Fi stick on the robot's backpack, and the
conversation ran with the Ethernet cable pulled. Then, in the same session,
Cassian got the robot's **camera, tilt sensor and joint readings**.

## Untethered

| Step | Result |
|---|---|
| Kernel | 5.10.104-tegra. Too old for the Netgear A8000's driver (`mt7921u`, Linux 5.18+), per the brief's table |
| Netgear A8000 | Not tried: too thick to fit the backpack port |
| **MT7601U nano stick** | **Worked.** Driver loaded by itself, `wlan0` appeared |
| Backpack USB ports | **Reach the Jetson** (first time verified) |
| Join Wi-Fi | `nmcli --ask dev wifi connect`, from the robot shell. Password typed at the prompt |
| Proof before pulling the cable | ssh over Wi-Fi answered. 2.4 GHz, strong signal, 0% loss, ~10 ms |
| Cable pulled | Conversation continued. Ethernet address stopped answering, Wi-Fi carried everything |

Nothing in the conversation code had the cable's address in it. The robot's
address comes from `R1`. The voice service still uses `eth10`, which is the
Jetson's *internal* link to the speaker board. The cable isn't involved.

Not yet tested: whether the robot rejoins Wi-Fi by itself after a reboot.

## Senses

`src/converse.py` is session 04's loop plus two switches. Without them,
Cassian is blind, as in session 04.

- **`--senses`**: before each reply, a read-only reader on the Jetson
  (`src/senses.cpp`: it subscribes and never publishes) gives him a short
  **robot readout**:
  - body orientation from the tilt sensor;
  - every joint angle, with names marked **confirmed** (by hand tests) or
    **inferred** (from vendor code);
  - movement since the last reading, kept across runs;
  - hottest motor;
  - whether the speaker service accepted his last reply. The readout notes
    that this doesn't prove anyone heard it.
  - A list of what he still can't know.
- **`--vision`**: also one still frame from the head camera per question.

**The camera:** the Jetson has no camera device. But the R1 answers the
**Go2's `videohub` service** in the SDK, and returns a 1280×720 wide-angle JPEG
from the head. That's the same pattern as session 03, where the R1 answered
the G1 voice service. It's undocumented for the R1. See
[`notes/hardware-findings.md`](../../notes/hardware-findings.md).

## Blind vs senses

| Question | Blind | With senses / camera |
|---|---|---|
| "Can you hear me?" | Session 04: *"Yes, I can hear you."* | *"I can't hear you in the way you mean — … I'm reading you rather than listening."* |
| "How many fingers am I holding up?" | *"I can't see you at all."* | **3 of 4 right** (said four when it was three) |
| "Are your arms up or down?" | *"I genuinely can't tell."* | *"I can't tell you for certain … I'd only be guessing."* Correct: no arm reference pose is known |
| Orientation | *"No sense of my own body."* | *"On my back with my upper body slightly raised."* Correct |
| "Did anything just move?" (head turned by hand) | n/a | *"My head turned … from about 64 degrees back to zero … I can see a bit of the room now."* Correct |
| "Did anything reach you?" (not cold) | never run | *"Your words reached me as text, and a fresh picture came with them."* Correct |

## Findings worth keeping

- **Accurate about what he's given, inventive about what he isn't.** After
  a wrong finger count, he said *"I got the last one wrong, your hand was
  already closed"*. Only the newest frame is kept, so he couldn't see the
  earlier one, and the reason he gave was false. It's the same family as
  session 04's *"loud and clear"*.
- **Wording in the readout matters.** *"They show movement, not whether…"*
  was read twice as "movement detected". We reworded it. That one was our
  error, not his.
- **Movement needs a reference that survives restarts.** Two head turns
  went unreported because each run started with a fresh reading. The last
  reading is now saved locally.
- Asked *"Do you remember the last session?"* he said no. Asked about *"the
  session with all four AIs"*, he described it accurately from his files.
  The wording of the question changed the answer.
- Head yaw (slot 30) read **exactly 0.00°** after two separate hand turns.
  This is unexplained.
- The Claude API stopped mid-session: *"You have reached your specified API
  usage limits"*. This is the account's monthly spend limit, not the credit
  balance. Each turn re-reads about 275k cached tokens.

## Not done

- The strict cold *"Did anything reach you?"* as the first line of a run.
- Rejoin after a reboot; a router reservation for the robot's address.
- An arm reference pose, so arm angles can mean "up" or "down".
- The app assistant's answers to the four questions, for the side-by-side.

Conversation transcripts, camera frames and the saved sensor reading are kept
locally and not published. This session's brief: `CC_BRIEF_ROBOT_S5_WIRELESS.md`
(not in this repo).
