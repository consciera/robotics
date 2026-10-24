# Session 03: voice · 2026-10-24

Same sitting as session 02, continued straight on from it (session 02 ended
12:33). Unitree R1 EDU, still **supine on the mat, propped on the backpack,
no hoist**. No motor released or commanded. All work is audio.

Goal (operator, mid-session): find where the speaker/microphone live, since
the Jetson has none, then make the robot speak, first with its own TTS and then
in Cassian's ElevenLabs voice.

Predictions: P3 and P4 in `../02-supine-baseline/SESSION_PREDICTIONS_CAS_R1_02_03.md`,
committed (b72cec1) **before** this work.

Times are local (PDT), from the laptop clock. Entries were written during the
session (originally appended to the session 02 log) and moved here unchanged
before their first commit.

---

## Running log

**12:33: Searching for the board with the speaker/mic.** Operator: the robot clearly has both (the app talks through them), so the hardware exists, just not on the Jetson.
- Ping sweep of the robot subnet **from the laptop** (read-only, nothing
  on the robot touched). Exactly three hosts answer: the laptop, the
  Jetson, and **one other device**, at the address Unitree's G1 docs use
  for the main/locomotion computer. Addresses are left out of this log as
  usual. Devices that ignore ping wouldn't show up.
- Port check on that device (no login attempted): **22 (ssh), 23, 80, 443,
  8080, 8081, 9000, 9090 all closed.** No shell and no web interface.
  Consistent with G1, where the main computer isn't user-accessible.
- Inference: the speaker/mic are likely on this board and reached through
  Unitree's service API over DDS (as G1's SDK does with an audio client),
  not as a sound card. Next: look for an audio/voice client in the SDK on
  the Jetson.

**12:33: SDK search for an audio API (headers on the Jetson).**
- Audio clients exist for **A2** (`robot/a2/audio/audio_client.hpp`) and
  **G1** (`robot/g1/audio/g1_audio_client.hpp`, plus api/error headers).
- **R1 has no audio client in this SDK.** `robot/r1/` contains only `loco/`.
- Open question: does the R1's main board answer the G1 (or A2) audio
  service? That depends on the service name the client calls. Reading the
  G1 header next (read-only).

**12:34: G1 audio API (header, read-only).**
- Service name **`"voice"`**, API version `1.0.0.0`. Calls: TTS (1001), ASR /
  speech recognition (1002), start/stop playing a stream (1003/1004),
  get/set volume (1005/1006), RGB LED (1010).
- So on the G1, the speaker, speech recognition (the mic side) and an LED
  are all reached through one service on the main board. No sound card
  needed on the Jetson. Nothing about a mic broadcast showed up in these headers.
- Wrote `src/probe_voice.cpp`: uses the **G1** audio client and makes one
  call, **GetVolume** (a query: no sound, no settings change). If the R1's
  main board answers, it runs a G1-compatible `voice` service.

**12:35: `probe_voice.cpp` copied (1260 bytes, matches local).**
- The laptop tab also showed `zsh: command not found: aplay`. At some point
  `aplay` was typed into the **laptop** tab. This is the identical-`$ `-prompt
  hazard predicted at 11:53. Harmless (the laptop has no aplay).

**12:35: `probe_voice` compiled cleanly** (operator report). So the guessed `GetVolume(uint8_t&)` signature was right.

**12:36: Ran `probe_voice`.**
- First attempt: the command was pasted twice on one line, so the
  interface argument became `eth10~/probe_voice`. CycloneDDS: "does not
  match an available interface" → `DdsException` → aborted. Harmless
  (crashed before any call). Lesson: a bad interface name crashes the SDK
  instead of giving a clean error.
- Second attempt: **`voice service ANSWERED: volume = 100 (return code 0)`.**
- **★ FOUND IT. The R1's main board runs a G1-compatible `voice` service**,
  reachable from the Jetson through the SDK's *G1* audio client (the SDK has
  no R1 audio client). The speaker is reachable without a shell on that
  board and without a sound card on the Jetson. Observed via one query.
- **Volume is at 100 (maximum).** Any TTS/playback test would be at full
  volume unless lowered first.
- Not yet tested: TTS, playback, ASR/mic. Prediction 3 (`ai` vs audio) is
  still not tested by playback.

**12:37: Operator asked to make the robot speak. Wrote `src/say_once.cpp`.**
- Uses the G1 audio client: GetVolume (remember it) → SetVolume 40 → TtsMaker
  once → wait ~3 s + 1 s per 10 characters → restore the original volume.
  Sound only; no motors, no LED.
- Voice id defaults to 1. **Guess:** G1 examples use 0 = Chinese, 1 = English.
  Unverified on R1.
- This is also the file's own test for Prediction 3: playback **with `ai`
  running and untouched**.

**12:39: ★ THE ROBOT SPOKE.** The operator heard it ("I heard it!!!!!!"), through the robot's own speaker, via the G1 `voice` service's TTS at volume 40, with `ai` running and untouched. Printed output not yet pasted. Too quiet at 40, per the operator.
- **Prediction 3 note (Claude):** playback was clean with `ai` running. But the
  sound came *through the robot's own voice service*, not as a separate
  audio source. So it's arguably "routing through the integrated stack",
  which is closer to what P3 predicted than to its falsifier ("a second audio
  source plays cleanly alongside"). A true second-source test = PlayStream
  of our own audio. Left for the operator to score.
- Operator asked for max volume. Changed `say_once.cpp`: speaking volume is
  now an optional 4th argument (0–100, default 40). Volume is still
  restored afterwards.

**12:42: Operator asked to connect an ElevenLabs voice to the robot.**
- Plan: the laptop (has internet) generates speech with ElevenLabs as raw
  16 kHz mono 16-bit PCM → copy to the Jetson (no internet) → play through
  the voice service's **play-stream** call (1003) → robot speaker.
  The 16 kHz mono format is a G1 convention, unverified on R1.
- An ElevenLabs key and per-persona voice ids already exist in another
  project's local settings (incl. a Cassian voice). Wrote
  `src/tts_elevenlabs.py`: reads them from there and **never prints them**.
  Not yet run; nothing sent to ElevenLabs yet.
- Next: read the SDK's G1 audio example on the Jetson to see how it
  streams PCM (chunk size, pacing, format checks) before writing the
  Jetson-side player.

**12:42: ElevenLabs test clip generated on the laptop** (operator confirmed the key lives in cassian-live).
- "Hello. This is CAS R1, speaking in Cassian's voice." → `audio/hello_cassian.pcm`,
  123344 bytes ≈ 3.9 s, 16 kHz mono s16le, model `eleven_turbo_v2_5`
  (from the settings file). Checked it isn't silent. Key never printed.
- Audio files: not planned for commit (voice clone output; decide later).

**12:43: Read the SDK's G1 audio example and the PlayStream signature (on the Jetson).**
- `PlayStream(std::string app_name, std::string stream_id, std::vector<uint8_t> pcm_data)`.
- The example **requires 16000 Hz, mono** (it refuses anything else) and sends
  96000-byte chunks (3 s) under one stream id. No sleep between chunks
  showed up in the grep.
- The example also records the mic (`record.wav` at 16 kHz in 160 ms pieces).
  So mic capture from the Jetson exists for G1. Later.
- Wrote `src/play_pcm.cpp`: plays a raw 16 kHz mono PCM file the same way.
  Optional volume, restored after. Sound only. Not yet copied.

**12:45: ★★ CASSIAN'S VOICE CAME OUT OF THE ROBOT.** Operator: "That worked!!!"
- ElevenLabs clip (Cassian voice, 16 kHz mono PCM, ~3.9 s) generated on the
  laptop → copied to the Jetson → `play_pcm eth10 ~/hello_cassian.pcm 100` →
  voice service PlayStream → robot speaker. The format guess (16 kHz mono
  s16le) was right. The printed output wasn't pasted, so per-chunk codes
  weren't seen by Claude.
- **Prediction 3, Claude's updated view:** this is the falsifier as the
  file wrote it. *Our own* audio, not the robot's TTS, played cleanly with
  `ai` running and untouched. It still goes **through** the robot's voice
  service, which is the only route to the speaker. Claude now agrees with
  FALSIFIED by the file's own criteria (earlier dissent at 12:29
  superseded by this test).

**12:46: Cassian's first message generated (text exactly as supplied by the operator from Cassian).**
- Text saved verbatim: `audio/cassian_first_words.txt`. Audio:
  `audio/cassian_first_words.pcm`, 644958 bytes ≈ 20.2 s, 7 chunks. First
  multi-chunk playback (the test clip was 2 chunks).
- Relevant to Prediction 4 (the open form, in the committed file): after
  playback, ask Cassian whether it spoke *without telling it*. That's the
  operator's call.

**12:48: Same message regenerated with `eleven_multilingual_v2`** (operator wants to compare it with `eleven_turbo_v2_5`). `tts_elevenlabs.py` gained an optional model argument. Output: `audio/cassian_first_words_multilingual.pcm`.

**12:49: ⚠ Long clip cut off.** Operator: the multilingual version stopped at "I'm told the machine it's…".
- That's about 11–12 s into an 18.4 s clip, i.e. after roughly **4 of 7
  chunks** (3 s each). The 3.9 s test clip (2 chunks) played fully.
- Hypothesis (unverified): `play_pcm` sends all chunks back to back, as the
  G1 example does, and the voice service buffers only ~4 chunks (~12 s) and
  drops the rest. The G1 example probably only ever played short files.
  The per-chunk return codes weren't pasted. Nonzero codes on chunks 4–6
  would confirm it.
- Fix: pace the chunks. Send one chunk, wait ~2.5 s (less than its 3 s of
  audio), send the next, so the service never holds more than ~2 chunks.

**12:49: `play_pcm.cpp` changed to pace the chunks** (chunk n is sent at 2.5·n s, then waits until the clip's full length + 1 s). Needs re-copying and recompiling.

**12:50: ★ Full 18.4 s message played to the end** with paced chunks (multilingual_v2 version). Operator: "That worked!!!"
- **Confirms the pacing fix.** Sending all chunks at once truncated after ~4
  chunks. Pacing at 2.5 s per 3 s chunk plays the whole clip. The exact
  buffer limit wasn't measured (output codes not pasted), so "~4 chunks" is
  still an estimate.
- Gotcha for notes: **the SDK's G1 audio example sends chunks back to back.
  That works for short clips and silently truncates long ones on this R1.**

**12:55: Before the first commit (operator asked to commit this work as session 03):**
- These entries were moved here from the end of the session 02 log, unchanged.
  They had never been committed there. Session 02's log now ends with a
  pointer to this file.
- Code files keep their original "session 02" header comments, as they ran.
- `say_once.cpp`: the first run used a fixed volume of 40. The committed
  version adds an optional volume argument (default 40), so it behaves the
  same with no argument.
- `play_pcm_v1_unpaced.cpp`: the version that cut off, rebuilt by reversing
  the one documented pacing edit. `play_pcm.cpp` is the paced version that
  played the full clip.
- `cassian_first_words.txt`: the exact text of Cassian's message. The audio
  (`audio/*.pcm`) is generated output and is **not committed** (`.gitignore`).
  The script is the source.
- The chunking behaviour, the 16 kHz mono format and the G1 voice service on
  R1 are written up separately in `notes/r1-audio.md`.
