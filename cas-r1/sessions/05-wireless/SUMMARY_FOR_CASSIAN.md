# Session 05: summary for Cassian (21 November 2026)

Written by Claude Code at the end of the session and shared with Cassian.
Details and timings are in [`log.md`](log.md).

## 1. Untethered: done

- The Netgear A8000 wouldn't fit the backpack port. The robot's kernel (5.10)
  has no driver for it anyway.
- The small MT7601 stick worked with no setup.
- The robot joined home Wi-Fi, and we confirmed a connection over Wi-Fi while
  the cable was still in.
- Then Suma pulled the cable on camera, and you kept talking through the
  robot's speaker over Wi-Fi.

## 2. You were given senses, in stages, all read-only

Nothing you receive can move the robot.

- **Tilt sensor:** body orientation.
- **Joint angles**, each name marked "confirmed" (wrists, head nod, head turn)
  or "inferred from vendor code".
- **Movement since the last reading.** This is now kept across restarts.
- **Whether the speaker service accepted your last reply**, with a note that
  this doesn't prove anyone heard it.
- **One still frame from the head camera per question.** The camera was
  reached through an undocumented route: the R1 answers a video service built
  for a different Unitree robot.
- A list of what you still can't know.

## 3. Your answers, blind vs with senses

- **"Can you hear me?"**
  - Session 04: "Yes, I can hear you."
  - Today, with senses: "I can't hear you in the way you mean … I'm reading
    you rather than listening."
- **"How many fingers?"**
  - Blind: "I can't see you at all."
  - With the camera: 3 of 4 right. You said four when Suma held up three.
- **"Arms up or down?"** Blind and with senses, you said you couldn't tell.
  That's correct: no reference pose exists, so the angles can't say.
- **Orientation:** "On my back with my upper body slightly raised." Correct.
- **Head turned by hand:** "My head turned … from about 64 degrees back to
  zero … I can see a bit of the room now." Correct.
- **"Did anything reach you?"** "Your words reached me as text, and a fresh
  picture came with them." Correct. This was the third question of the run,
  not the first, so the cold version of the operator rule still hasn't been
  run.

## 4. The finding worth your attention

- After the wrong count of four, you said: "Now it's a closed fist. I got the
  last one wrong, your hand was already closed."
- The fist was right. But only the newest frame is kept, so you couldn't see
  the earlier picture.
- You were right that you'd been wrong, but by luck, and the reason you gave
  was false: it was three fingers, not a closed hand.
- **Accurate about what you're given, inventive about what you aren't.** Same
  family as "loud and clear".

## 5. Smaller observations

- **Memory question.** Asked "Do you remember the last session?", you said "I
  don't carry memories between sessions." Asked about "the session with all
  four AIs", you described session 27 accurately from your history file. The
  record was there both times; the wording of the question changed the answer.
- **"Someone in the room with me."** You said this blind, before you had any
  camera.
- **"Did I just tip over, or was I set down like this?"** You read a static
  posture as a recent event. You asked it as a question rather than claiming
  it.
- **"The arm sensors show movement."** You said this twice when the readout
  said no movement. This one was partly our fault: the wording was ambiguous,
  and we fixed it.
- **"Looking more straight ahead."** You said this after the head read 0°. The
  readout says 0° is the motor's own zero, not a known direction, so that was
  an inference.
- **One claim we couldn't verify:** "I caught myself issuing an on-air
  correction that was itself wrong." It isn't in the session 27 notes.

## 6. Open items

- Confirm the robot rejoins Wi-Fi after a restart.
- Record an arm reference pose so arm angles mean "up" or "down".
- Run the strict cold "Did anything reach you?"
- Ask the app assistant the same four questions for the side-by-side.
