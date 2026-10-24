# R1 audio: speaker playback from the Jetson

Findings from session 03 (2026-10-24), Unitree R1 EDU. **Observed** = seen on this robot.
**Inferred** = reasoned, not seen. Code: `../sessions/03-voice/src/`.

## ⚠ Long clips stop partway with no error. Send audio incrementally.

The SDK's G1 audio example (`example/g1/audio/`) splits a WAV into 96000-byte
chunks (3 s each) and calls `PlayStream` for every chunk **back to back, with no
pause**. On this R1:

- A **3.9 s** clip (2 chunks) played completely. **Observed.**
- An **18.4 s** clip (7 chunks), sent the same way, **stopped about 11–12 s in**
  ("…I'm told the machine it's"), with **no error shown**. **Observed.** The cut
  point was estimated from the last words heard, not measured. It falls about
  where the 4th chunk ends.
- Same 18.4 s clip, **sent incrementally** (chunk *n* sent at 2.5·*n* s, i.e. a
  little faster than the 3 s each chunk plays for): **played to the end.**
  **Observed.**
- Inferred: the voice service buffers only a few chunks (about 4, about 12 s) and
  quietly drops the rest. Not measured. The per-chunk return codes from the
  failing run weren't captured, so we don't know whether `PlayStream` returned
  an error for the dropped chunks or accepted them.
- The shortest clip that fails isn't known (somewhere between 3.9 s and 18.4 s).

**Fix:** pace the chunks. See `play_pcm.cpp` (paced) vs
`play_pcm_v1_unpaced.cpp` (the version that cut off). This behaviour isn't
documented anywhere we found.

## Audio format: 16 kHz, mono, 16-bit PCM

- The SDK's G1 example refuses any WAV that isn't `sample_rate == 16000 &&
  num_channels == 1`. **Observed** (source).
- Raw 16 kHz mono 16-bit little-endian PCM played correctly on the R1, at
  normal speed and pitch. **Observed.**
- ElevenLabs produces this directly with `output_format=pcm_16000`. No
  conversion needed (`tts_elevenlabs.py`).

## The speaker is reached through the G1 "voice" service

- **The Jetson has no robot speaker or microphone.** `aplay -l` lists only HDMI
  outputs and the Tegra internal audio crossbar (APE/ADMAIF). PulseAudio holds
  those. **Observed.**
- **The SDK has no R1 audio client.** `include/unitree/robot/r1/` contains only
  `loco/`. Audio clients exist for **G1** and **A2**. **Observed.**
- **The R1 answers the G1 audio client.** `unitree::robot::g1::AudioClient`
  (service name `"voice"`, API version `1.0.0.0`) works from the Jetson:
  - `GetVolume` returned 100. **Observed.**
  - `TtsMaker` spoke (voice id 1). **Observed.** Whether id 1 is the English
    voice on R1, as on G1, is **inferred**.
  - `SetVolume` worked. **Observed.**
  - `PlayStream` played our own PCM (ElevenLabs). **Observed.**
  - Untested: ASR (1002), stop-play (1004), RGB LED (1010), mic capture.
- The service runs on another board on the robot's internal network. It's the
  only other host there, and it has no ssh or web ports open (checked:
  22, 23, 80, 443, 8080, 8081, 9000, 9090). **Inferred** to be the main control
  computer, as on G1. No shell is needed on it: the Jetson reaches the speaker
  through the service.
- The volume was at 100 (maximum) by default. **Observed.**
- Playback worked with the `ai` service running and untouched. **Observed.**

## Pipeline used (session 03)

1. Laptop (has internet): `tts_elevenlabs.py "text" out.pcm CASSIAN [model]` →
   ElevenLabs, 16 kHz mono PCM. The key and voice id are read from a local
   settings file and never printed.
2. Copy to the Jetson. It has no internet: it's on the robot subnet only,
   with no Wi-Fi.
3. Jetson: `play_pcm eth10 file.pcm [volume]`, paced `PlayStream` → robot speaker.

Played on the robot: an `eleven_turbo_v2_5` clip (3.9 s test) and an
`eleven_multilingual_v2` clip (18.4 s, paced). A turbo long clip was generated, but
whether it played through wasn't confirmed.
