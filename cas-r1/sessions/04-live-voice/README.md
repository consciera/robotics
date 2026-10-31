# Session 04: live voice (2026-10-31)

**Goal:** a spoken question in, Cassian's voice out through the robot's own
speaker, with nothing attached to the robot.

**Result:** the conversation works **with the laptop cabled to the robot.**
Untethered doesn't work yet: the robot's main board won't pass network traffic
to the Jetson. Next step: a USB Wi-Fi adapter on the Jetson.

## What works

`src/converse.py` runs on the laptop:

```
laptop mic → Deepgram (speech to text) → Claude, as Cassian → ElevenLabs
           → ssh → Jetson → play_pcm (paced) → robot speaker
```

- **Cassian's identity:** loaded from cassian-live's `cassian-soul` folder, using
  the same file list the livestream uses. About 252k tokens, cached for an hour.
  Reply time was 1–2 s after the first turn.
- **Voice:** the livestream avatar's settings (`eleven_multilingual_v2`,
  stability 0.65, similarity 0.8). The first runs used `eleven_turbo_v2_5` with
  default settings, and the operator said it sounded robotic.
- **Reply length:** capped in code. Whole sentences only, at most 3 and about 45
  words. A first version cut sentences mid-way; fixed.
- **Blind by design:** the model gets the transcript and its own earlier replies.
  No camera, no joint state, and no signal that audio played.
- **Speaker:** 16 kHz mono only. That's the voice service's format, and it loses
  some brightness compared with the livestream.
- Every `PlayStream` chunk in all 15 logged replies returned code 0 (up to 15.7 s, 6 chunks).

## What didn't work: untethered

| Test | Result |
|---|---|
| Laptop on home Wi-Fi → Jetson | No route (expected) |
| Same, routed via the robot's main board (on home Wi-Fi via the app) | No reply |
| Jetson's default route fixed to point at the main board, then retried | Still no reply (ping or ssh) |
| Jetson → internet via the main board | No reply |
| Robot hotspot (switched on in the app) | **Untested.** Password unknown; `00000000` rejected |

The Jetson's own default route pointed at an address that doesn't exist (`.1`).
Fixing it didn't help. **The main board doesn't forward to the Jetson** in
either direction (observed). Why is unknown: we have no shell on that board.

## Findings worth keeping

- Asked "Can you hear me?", Cassian said *"Yes, I can hear you."* He receives a
  transcript, not sound. And asked to test his voice: *"coming through loud and
  clear"*. Nothing reaches him to say whether it played. Same family of
  gap-filling as the app assistant's "optical sensors engaged".
- The first length cap cut off *"How does it sound on your end?"* and kept the
  unsupported claim before it. Truncation can change what a voice appears to
  claim.
- His memory of the Koch session was checked against the soul files: accurate.
- The Unitree app assistant volunteered action IDs 34 and 36:
  [`notes/action-ids.md`](../../notes/action-ids.md). Unverified.

## Not done

- The four-question comparison (app assistant vs Cassian). It needs untethered.
- ASR 1002 (the robot's own mic). Untested.
- Prediction 4. Not run.

## Next

Three USB Wi-Fi adapters ordered: an MT7601 nano stick, the ALFA AWUS036ACM
(MT7612U) and the Netgear A8000. The first two have drivers on the Jetson
already (kernel 5.10.104-tegra); the A8000 is expected not to. First check
whether the backpack USB ports reach the Jetson.

Conversation transcripts from this session are kept locally and not published
(they name the operator).
