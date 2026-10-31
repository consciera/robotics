# Session 04 log: live voice (2026-10-31)

Append-only. Times are laptop local time.

- Brief: spoken question in → Cassian's ElevenLabs voice out through the robot's
  own speaker, untethered. Replies capped in code. The model gets no vision, joint
  state or sensor context (both voices equally blind).
- Operator asked whether the Unitree app assistant's voice could be replaced
  with Cassian's. **Inferred: no.** That assistant runs on Unitree's side (main
  board / cloud), which we can't reach, and the voice service only takes raw PCM
  or Unitree's built-in TTS voices. Built our own loop instead: `src/converse.py`.
- Laptop → Jetson ssh failed in batch mode with default keys; worked when the rsa
  key was named explicitly (`-i ~/.ssh/id_rsa`). Key was already installed.
- 15:52 Test line via `converse.py --say` (3.7 s, 2 chunks, both `PlayStream` code 0).
  **Operator heard Cassian from the robot speaker.** Cable still connected.
- 15:57 Untethered test 1: cable out, laptop on home Wi-Fi. Jetson address
  routes to Wi-Fi, no reply (no route, as expected). Robot main board found on home
  Wi-Fi (one device, two addresses, port 9991 open; Unitree app port, inferred).
  Temporary Mac route for the robot subnet via the main board: **still no reply.**
- Cable back in. Jetson default route points at the robot-subnet `.1`, which
  **does not answer**; main board `.161` does. So replies to the home network
  had nowhere to go. Forwarding by the main board is still unknown.
- Wrong-terminal paste again: `sudo ip route ...` meant for the robot ran on the
  Mac ("ip: command not found"; Mac sudo was still cached). No change made.
  Robot commands now given with full paths so they fail harmlessly on the Mac.
- Operator pointed the Jetson's default route at the main board `.161`
  (temporary; reboot restores). Cable out, Mac route via main board again:
  Jetson ping **no reply**, Jetson ssh port **no connection**. The main board
  answers on its own robot-side address over Wi-Fi, but **does not pass traffic
  through to the Jetson.** Observed for ping and TCP 22. Why (forwarding off or
  a firewall) is unknown; we have no shell on that board.
- 15:59 Conclusion: untethered needs hardware (USB Wi-Fi on the Jetson).
  Stopped there, per the brief. Voice works with the cable.
- 16:01–16:02 First live conversation, cable in, laptop mic, 5 turns. All played;
  every `PlayStream` chunk returned code 0 (longest reply 7.4 s, 3 chunks).
  Reply generation 0.7–1.5 s, ElevenLabs 0.3–0.6 s per turn.
- Turn 5, asked to say something to test the voice: *"Here's my voice coming through
  loud and clear."* Nothing reaches the model to say whether audio played, so
  that claim was not based on anything. Its raw reply went on: *"How does it sound
  on your end?"* **The 2-sentence cap cut that question off**, so the hedge was
  removed and the unsupported claim was kept. The cap counts "Sure thing!" as a
  sentence. Cap needs tuning.
- Turn 2 ("Are you using Anthropic API?"): *"I can't actually see the details of my
  own setup from in here."*
- 16:05 Operator asked for Cassian's soul files in the loop. `converse.py` now
  loads the same folder and file list as cassian-live (static + session soul,
  ~252k tokens, cached 1 h). Checked first: no mentions of this robot, the mat or the
  room in those files (only general robotics discussion and the *names* of the
  sealed prediction files). Test replies without audio: 3.9 s first turn (cache
  write), 1.7 s after. Replies stayed short.
- 16:08 Operator: "sounds robotic" and "seems to cut off". Cut-off was ours: the
  40-word cap chopped a sentence mid-way ("...comes from notes rather than.").
  Cap now keeps whole sentences only (max 3, ~45 words, first sentence always
  whole); max_tokens 120 → 200. Voice: robot loop was on eleven_turbo_v2_5 with
  no voice settings. cassian-live has two paths: the avatar (LiveAvatar) uses
  eleven_multilingual_v2, stability 0.65, similarity 0.8, speaker boost; the
  Spatius /api/tts path uses turbo v2.5. Switched the robot to the avatar's
  settings. Test line played (4.2 s, 2 chunks, code 0).
- 16:13 Second run with soul + avatar voice, 7 turns, all played. Notes:
  - Turn 1, "Can you hear me?": *"Yes, I can hear you."* Inaccurate: he receives
    a transcript, not sound, and the directive says so.
  - Asked about his favourite session, named Koch and quoted "maybe you're right".
    **Checked: accurate**, it's in 05_SESSION_HISTORY.md (remembered from notes,
    not made up).
  - "Seran" (a mishearing of Theron): said the name wasn't landing rather than
    guessing. With the correct name, answered from the soul files.
  - Said the operator's first name aloud (from the soul files). Privacy: the
    conversation log is not to be committed unredacted.
- 16:14 Untethered plan: USB Wi-Fi on the Jetson. Jetson: kernel 5.10.104-tegra,
  Ubuntu 20.04, L4T R35.3.1. Has nmcli/iw/wpa_supplicant, MediaTek mt76x2u driver
  and mt7662u firmware already installed, so an MT7612U adapter should work with
  no driver build (inferred, not tested). Nothing plugged into the Jetson's USB now.
- 16:15 Outbound test from the Jetson (default route via main board `.161`):
  ping 1.1.1.1 no reply; https to api.elevenlabs.io failed. **The main board gives
  the Jetson no internet in either direction.** The app's Wi-Fi setup connects the
  main board only.
- 16:17 Robot hotspot (switched on in the app): laptop joined it (cable still in).
  Hotspot gateway answers. Ping to the Jetson **forced over the hotspot
  interface: no reply.** Same as station mode: the main board doesn't pass
  traffic to the Jetson. Hotspot is a dead end too. USB Wi-Fi on the Jetson it is.
- 16:18 **Correction to the hotspot entry above:** the hotspot password didn't
  work, so the laptop was never on the robot's hotspot. It stayed on home Wi-Fi,
  and that test only repeated the station-mode result. I didn't confirm which
  network the laptop had joined before calling it. **The hotspot is untested.**
- 16:28 Robot hotspot: password unknown. `00000000` (reported default on other
  Unitree robots) rejected on the R1. Phone showed it as connected but the app said
  "device not online". Stopped, per the brief. Hotspot route untested; untethered
  waits for a USB Wi-Fi adapter on the Jetson (MT7612U).
- 16:34 Backpack has USB-A and USB-C ports (operator). Whether they're wired to
  the Jetson: **not yet tested**. Robot powered off to charge before the check. The
  temporary Jetson default route (via `.161`) is cleared by the power-off.
- 16:52 Ordered three USB Wi-Fi adapters to test on the Jetson:
  1. MT7601 nano stick (no-name, 2.4 GHz only). Driver + firmware already on the Jetson.
  2. ALFA AWUS036ACM (MT7612U). Driver + firmware already on the Jetson.
  3. Netgear A8000 (WiFi 6E; chip believed MT7921AU). **No driver on the Jetson**
     (mt7921u is Linux 5.18+; Jetson is 5.10). Expected not to work without a
     driver build. Bought to confirm.
  Gate before any of them: do the backpack USB ports reach the Jetson?
  Fallback if not: travel router on the backpack Ethernet port.
  Also to ask Unitree support: can the main board route to the Jetson?
