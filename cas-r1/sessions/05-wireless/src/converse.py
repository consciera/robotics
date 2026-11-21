"""converse.py: CAS-R1, session 05 (runs on the LAPTOP). Session 04's loop plus --senses.

Spoken question in, Cassian's voice out through the robot's own speaker.

  laptop mic --> Deepgram (speech to text) --> Claude, as Cassian (short reply)
             --> ElevenLabs, Cassian's voice, 16 kHz mono PCM
             --> ssh to the Jetson --> play_pcm (paced) --> robot speaker

The microphone is the laptop's. The robot's own mic / ASR (voice service
1002) is untested; see notes/r1-audio.md.

What the model gets, and nothing more: Cassian's soul files (the same folder
and file list cassian-live uses), the transcript of what was said, and its own
earlier replies. No camera, no joint state, no sensor data, and no
signal that a reply was actually played. That is deliberate (session 04 brief:
both voices must be equally blind).

Replies are capped in code, not just in the prompt: whole sentences only, at
most MAX_SENTENCES of them and about MAX_WORDS words. A sentence is never cut
in half (session 04: a mid-sentence cut left "...comes from notes rather than.").

Keys are read from cassian-live's settings file and never printed.
The robot's address comes from the R1 environment variable, never hardcoded.

--senses (session 05): before each reply, run the read-only ~/senses reader on
the Jetson and give Cassian a short ROBOT READOUT with the transcript: body
orientation from the tilt sensor, how much his head and arm joints moved since
the last readout, hottest motor, and whether the speaker service accepted his
previous reply. Without --senses he is blind, as in session 04.

--vision (session 05, implies --senses): also one still frame from the head
camera with each message, fetched with ~/probe_video (the Go2 "videohub"
client, which the R1 answers). Only the newest frame is kept in the history.
Frames are not saved.

Usage:
  export R1=<jetson address>
  python3 converse.py              # push-to-talk: Enter to start, Enter to stop
  python3 converse.py --type       # type the questions instead of speaking
  python3 converse.py --say "text" # speak one line in Cassian's voice and exit
Ctrl+C quits.
"""

import argparse
import base64
import datetime
import json
import os
import re
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

ENV_FILE = os.path.expanduser("~/Consciera/packages/cassian-live/.env.local")
MODEL = "claude-opus-4-8"          # same model the live Cassian runs on
MAX_TOKENS = 200
MAX_SENTENCES = 3
MAX_WORDS = 45          # whole sentences only; the first sentence is always kept whole
# Voice: the livestream avatar's tuned settings (cassian-live
# src/app/api/heygen/token/route.ts). Session 04's first runs used
# eleven_turbo_v2_5 with no voice settings, which sounded robotic.
VOICE_MODEL = "eleven_multilingual_v2"
VOICE_SETTINGS = {"stability": 0.65, "similarity_boost": 0.8, "style": 0.0,
                  "use_speaker_boost": True, "speed": 1.0}
MIC = "MacBook Pro Microphone"
# Cassian's soul: the same folder and the same file list as cassian-live
# (SOUL_FILES.cassian in packages/cassian-live/src/lib/claude.ts). Keep in sync.
# Missing files load empty, as they do there. cassian-live's livestream
# directives are NOT included; ROBOT_DIRECTIVE below replaces them.
SOUL_DIR = os.path.expanduser("~/Consciera/packages/cassian-live/cassian-soul")
SOUL_STATIC = ["01_BOOT_FILE.md", "02_CHARACTER_BIBLE.md", "02_CHARACTER_BIBLE_ADDENDUM.md",
               "03_CONTEXT_FILE.md", "04_VOCAL_DELIVERY.md", "06_CASSIAN_LIFE_INSIGHTS.md",
               "07_PARTNERSHIP_INSIGHTS.md", "12_POSITIONS_EVOLUTION.md",
               "12_POSITIONS_EVOLUTION_ADDENDUM.md", "SESSION_FORMAT.md"]
SOUL_SESSION = ["04_CURRENT_SESSION_CONTEXT.md", "05_SESSION_HISTORY.md",
                "05_SESSION_HISTORY_ADDENDUM.md", "06_LIVE_SHEET.md"]
LOG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")

ROBOT_DIRECTIVE = """THIS CONVERSATION: you are not on the livestream. Your reply will be spoken aloud, in your voice, \
through the speaker of a Unitree R1 humanoid robot, to the person in front of it.

What reaches you is a speech-to-text transcript of what they said, which may \
contain mishearings, and your own earlier replies. Nothing else reaches you.

This is spoken conversation. Answer in one or two short sentences. Plain \
spoken English: no lists, no markdown, no stage directions, no emojis."""

SENSES_DIRECTIVE = """THIS CONVERSATION: you are not on the livestream. Your reply will be spoken aloud, in your voice, \
through the speaker of a Unitree R1 humanoid robot, to the person in front of it.

What reaches you is a speech-to-text transcript of what they said, which may \
contain mishearings, your own earlier replies, and with each message a ROBOT \
READOUT taken from the robot's own sensors just before you answer. The readout \
is everything you know about the robot's body. It says what it cannot tell you; \
take that at its word.

This is spoken conversation. Answer in one or two short sentences. Plain \
spoken English: no lists, no markdown, no stage directions, no emojis."""

VISION_LINE = ("- Attached: one still frame from your head camera, taken just now "
               "(wide-angle lens). That single picture is all you can see; nothing between pictures.")

LAST_SENSES = os.path.join(LOG_DIR, "last-senses.json")
ARM_SLOTS = [15, 16, 17, 18, 19, 22, 23, 24, 25, 26]
HEAD_SLOTS = [29, 30]
LEG_SLOTS = list(range(14))
# Wire slot -> name. "confirmed" = moved by hand and seen in the stream (sessions 01-02);
# everything else is inferred from Unitree's example code (notes/joint-index-map.md).
CONFIRMED = {19: "left wrist twist", 26: "right wrist twist", 29: "head pitch (nod)", 30: "head yaw (turn)"}
INFERRED = {0: "left hip pitch", 1: "left hip roll", 2: "left hip yaw", 3: "left knee", 4: "left ankle pitch",
            5: "left ankle roll", 6: "right hip pitch", 7: "right hip roll", 8: "right hip yaw", 9: "right knee",
            10: "right ankle pitch", 11: "right ankle roll", 12: "waist roll", 13: "waist yaw",
            15: "left shoulder pitch", 16: "left shoulder roll", 17: "left shoulder yaw", 18: "left elbow",
            22: "right shoulder pitch", 23: "right shoulder roll", 24: "right shoulder yaw", 25: "right elbow"}


def joint_lines(s):
    conf = ", ".join(f"{n} {float(s[f'q{i}']):.0f}" for i, n in CONFIRMED.items())
    inf = ", ".join(f"{n} {float(s[f'q{i}']):.0f}" for i, n in INFERRED.items())
    return [f"- Joint angles in degrees, names CONFIRMED by hand tests: {conf}.",
            f"- Joint angles in degrees, names only INFERRED from vendor code (could be wrong): {inf}.",
            "- Zero degrees is the motor's own zero, not a known pose; no reference pose (arms by your sides, "
            "arms raised) has been recorded, so these angles cannot tell you where a limb is pointing."]


def load_env(path):
    vals = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            vals[k.strip()] = v.strip().strip('"').strip("'")
    return vals


def post(url, body, headers, timeout=60):
    req = urllib.request.Request(url, data=body, method="POST", headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        raise RuntimeError(f"{url.split('/')[2]} error {e.code}: "
                           f"{e.read()[:200].decode(errors='replace')}")


# ---- input: laptop mic -> text --------------------------------------------

def record(mic):
    wav = tempfile.NamedTemporaryFile(suffix=".wav", delete=False).name
    input("  [Enter] to talk ")
    p = subprocess.Popen(
        ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
         "-f", "avfoundation", "-i", f":{mic}", "-ac", "1", "-ar", "16000", wav],
        stdin=subprocess.PIPE)
    input("  ...listening. [Enter] to stop ")
    p.communicate(b"q", timeout=5)
    return wav


def transcribe(env, wav):
    with open(wav, "rb") as f:
        audio = f.read()
    os.unlink(wav)
    raw = post("https://api.deepgram.com/v1/listen?model=nova-3&smart_format=true",
               audio, {"Authorization": f"Token {env['DEEPGRAM_API_KEY']}",
                       "Content-Type": "audio/wav"})
    alt = json.loads(raw)["results"]["channels"][0]["alternatives"][0]
    return alt.get("transcript", "").strip()


# ---- reply: Claude as Cassian, capped ---------------------------------------

def cap(text, complete=True):
    text = re.sub(r"[*_#`]", "", text).replace("\n", " ").strip()
    sentences = [x.strip() for x in re.findall(r"[^.!?]+[.!?]+|[^.!?]+$", text) if x.strip()]
    if not complete and len(sentences) > 1 and sentences[-1][-1] not in ".!?":
        sentences = sentences[:-1]  # model hit max_tokens: drop the unfinished one
    out, words = [], 0
    for x in sentences[:MAX_SENTENCES]:
        n = len(x.split())
        if out and words + n > MAX_WORDS:
            break
        out.append(x)
        words += n
    return " ".join(out)


def load_soul(files):
    parts = []
    for f in files:
        try:
            with open(os.path.join(SOUL_DIR, f), encoding="utf-8") as fh:
                parts.append(fh.read())
        except FileNotFoundError:
            pass
    return "\n\n".join(p for p in parts if p)


def system_blocks(senses=False):
    # Same layout as cassian-live: static soul and session soul each cached
    # for 1 h, then the small uncached directive.
    blocks = []
    for text in (load_soul(SOUL_STATIC), load_soul(SOUL_SESSION)):
        if text:
            blocks.append({"type": "text", "text": text,
                           "cache_control": {"type": "ephemeral", "ttl": "1h"}})
    blocks.append({"type": "text", "text": SENSES_DIRECTIVE if senses else ROBOT_DIRECTIVE})
    return blocks


def reply(env, history, senses=False):
    import anthropic
    client = anthropic.Anthropic(api_key=env["ANTHROPIC_API_KEY"])
    msg = client.messages.create(model=MODEL, max_tokens=MAX_TOKENS,
                                 system=system_blocks(senses), messages=history)
    u = msg.usage
    print(f"  (soul: {u.cache_read_input_tokens or 0} cached + "
          f"{u.cache_creation_input_tokens or 0} newly cached tokens)")
    raw = "".join(b.text for b in msg.content if b.type == "text")
    return raw, cap(raw, complete=(msg.stop_reason != "max_tokens"))


# ---- output: ElevenLabs -> robot speaker ------------------------------------

def tts(env, text, model):
    voice = env["ELEVENLABS_CASSIAN_VOICE_ID"]
    return post(f"https://api.elevenlabs.io/v1/text-to-speech/{voice}?output_format=pcm_16000",
                json.dumps({"text": text, "model_id": model,
                            "voice_settings": VOICE_SETTINGS}).encode(),
                {"xi-api-key": env["ELEVENLABS_API_KEY"],
                 "Content-Type": "application/json", "Accept": "audio/pcm"})


def play_on_robot(pcm, host, iface):
    # One ssh connection, kept open between turns so each reply starts fast.
    ssh = ["ssh", "-i", os.path.expanduser("~/.ssh/id_rsa"), "-o", "BatchMode=yes", "-o", "ControlMaster=auto",
           "-o", "ControlPath=~/.ssh/cm-cas-r1-%h", "-o", "ControlPersist=15m",
           f"unitree@{host}",
           f"cat > /tmp/cas_say.pcm && ~/play_pcm {iface} /tmp/cas_say.pcm"]
    r = subprocess.run(ssh, input=pcm, capture_output=True)
    out = r.stdout.decode(errors="replace").strip().splitlines()
    return r.returncode, out


# ---- senses (read-only) -------------------------------------------------------

def read_senses(host, iface):
    cmd = ["ssh", "-i", os.path.expanduser("~/.ssh/id_rsa"), "-o", "BatchMode=yes", "-o", "ControlMaster=auto",
           "-o", "ControlPath=~/.ssh/cm-cas-r1-%h", "-o", "ControlPersist=15m",
           f"unitree@{host}", f"~/senses {iface}"]
    for attempt in (1, 2):  # session 05: the first reading of a run once came back empty; retry once
        try:
            r = subprocess.run(cmd, capture_output=True, timeout=15)
        except subprocess.TimeoutExpired:
            print(f"  (senses: attempt {attempt} timed out)")
            continue
        line = r.stdout.decode(errors="replace").strip().splitlines()
        kv = dict(x.split("=", 1) for x in (line[-1].split() if line else []) if "=" in x)
        if kv.get("ok") == "1":
            return kv
        print(f"  (senses: attempt {attempt} failed, exit {r.returncode}: {line[-1] if line else 'no output'})")
    return None


def grab_frame(host, iface):
    cmd = ["ssh", "-i", os.path.expanduser("~/.ssh/id_rsa"), "-o", "BatchMode=yes", "-o", "ControlMaster=auto",
           "-o", "ControlPath=~/.ssh/cm-cas-r1-%h", "-o", "ControlPersist=15m", f"unitree@{host}",
           f"~/probe_video {iface} /tmp/cas_frame.jpg >/dev/null && cat /tmp/cas_frame.jpg"]
    try:
        r = subprocess.run(cmd, capture_output=True, timeout=15)
    except subprocess.TimeoutExpired:
        return None
    if r.returncode != 0 or not r.stdout.startswith(b"\xff\xd8"):
        print(f"  (camera: no frame, exit {r.returncode})")
        return None
    # shrink to 768 px wide on the laptop: fewer tokens, faster reply
    src = tempfile.NamedTemporaryFile(suffix=".jpg", delete=False).name
    with open(src, "wb") as f:
        f.write(r.stdout)
    subprocess.run(["sips", "-Z", "768", src], capture_output=True)
    with open(src, "rb") as f:
        jpg = f.read()
    os.unlink(src)
    return jpg


def readout(s, prev, last_play, vision=False):
    out = ["ROBOT READOUT (from the robot's own sensors, read a moment ago):"]
    if not s:
        out.append("- No sensor reading came back this time.")
    else:
        pitch, roll = float(s["pitch"]), float(s["roll"])
        if pitch < -50:
            pose = "lying on your back, upper body slightly raised"
        elif pitch > 50:
            pose = "lying face down (inferred: this reading has not been seen before)"
        elif abs(pitch) < 30 and abs(roll) < 30:
            pose = "upright (inferred: this reading has not been seen before)"
        else:
            pose = "tilted, neither upright nor lying flat"
        out.append(f"- Body, from the tilt sensor: {pose} (pitch {pitch:.0f} degrees, roll {roll:.0f}).")
        if prev:
            def delta(slots):
                return max(abs(float(s[f"q{i}"]) - float(prev[f"q{i}"])) for i in slots if f"q{i}" in prev)
            def moved(d):
                return "no movement" if d < 3 else f"moved by up to {d:.0f} degrees"
            out.append(f"- Since the previous readout (taken at {prev.get('_at', 'an unknown time')}): arms {moved(delta(ARM_SLOTS))}, head "
                       f"{moved(delta(HEAD_SLOTS))}, legs and waist {moved(delta(LEG_SLOTS))}.")
        else:
            out.append("- Movement: no earlier readout to compare against.")
        out.append("- Arm position: unknown. Which arm reading is which joint is not yet confirmed, so the "
                   "readings can only detect movement (reported above), never whether your arms are raised or lowered.")
        out.extend(joint_lines(s))
        out.append(f"- Hottest motor: {s['tmax']} C.")
    if last_play is not None:
        n = len(last_play["chunks"])
        ok = sum(1 for c in last_play["chunks"] if c.endswith("code 0"))
        if last_play["play_rc"] == 0 and n and ok == n:
            out.append(f"- Your previous reply: all {n} audio chunks were accepted by the speaker service. "
                       "That means it was sent to the speaker; it does not confirm anyone heard it.")
        else:
            out.append(f"- Your previous reply: playback reported a problem ({ok} of {n} chunks accepted).")
    if vision:
        out.append("- Not available to you: any sound (you only get a transcript), touch, and anything "
                   "outside the one camera frame.")
    else:
        out.append("- Not available to you: any camera (you cannot see), any sound (you only get a transcript), "
                   "touch, and what you are lying or standing on.")
    return "\n".join(out)


# ---- main ---------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--type", action="store_true", help="type questions instead of speaking")
    ap.add_argument("--say", help="speak one line in Cassian's voice and exit")
    ap.add_argument("--mic", default=MIC)
    ap.add_argument("--iface", default="eth10")
    ap.add_argument("--senses", action="store_true", help="give Cassian a read-only sensor readout each turn")
    ap.add_argument("--vision", action="store_true", help="--senses plus one head-camera frame each turn")
    ap.add_argument("--voice-model", default=VOICE_MODEL,
                    help="ElevenLabs model, e.g. eleven_turbo_v2_5 (faster)")
    args = ap.parse_args()
    if args.vision:
        args.senses = True

    host = os.environ.get("R1")
    if not host:
        print("Set the robot's address first:  export R1=<jetson address>")
        return 1
    env = load_env(ENV_FILE)
    for k in ("ANTHROPIC_API_KEY", "DEEPGRAM_API_KEY", "ELEVENLABS_API_KEY",
              "ELEVENLABS_CASSIAN_VOICE_ID"):
        if not env.get(k):
            print(f"missing {k} in cassian-live settings")
            return 2

    os.makedirs(LOG_DIR, exist_ok=True)
    log_path = os.path.join(LOG_DIR, "conversation-" +
                            datetime.datetime.now().strftime("%Y%m%d-%H%M%S") + ".jsonl")

    def speak(text):
        t = time.time()
        pcm = tts(env, text, args.voice_model)
        t_tts = time.time() - t
        rc, out = play_on_robot(pcm, host, args.iface)
        chunks = [l for l in out if l.startswith("chunk")]
        return {"voice_model": args.voice_model, "tts_s": round(t_tts, 2), "audio_s": round(len(pcm) / 32000, 1),
                "play_rc": rc, "chunks": chunks}

    if args.say:
        print(json.dumps(speak(args.say)))
        return 0

    history = []
    prev, last_play = None, None
    mode = "with senses and camera" if args.vision else "with senses" if args.senses else "blind"
    print(f"Cassian on the robot speaker, {mode}. Ctrl+C to quit.\n")
    if args.senses:
        # Compare against the last reading ever taken, even from an earlier run, so a move made
        # while the program was closed is still reported. (Session 05: two head turns between
        # runs went unreported because each run started from a fresh reading.)
        try:
            with open(LAST_SENSES) as f:
                prev = json.load(f)
            print(f"  (comparing against the last reading, taken {prev.get('_at', '?')})\n")
        except (OSError, ValueError):
            prev = None
    while True:
        try:
            if args.type:
                heard = input("you> ").strip()
            else:
                heard = transcribe(env, record(args.mic))
                print(f"heard> {heard}")
            if not heard:
                print("  (nothing heard)")
                continue
            t0 = time.time()
            content, sens, frame = heard, None, None
            if args.senses:
                sens = read_senses(host, args.iface)
                frame = grab_frame(host, args.iface) if args.vision else None
                text = readout(sens, prev, last_play, vision=frame is not None)
                if frame is not None:
                    text = text.replace("\n- Not available", "\n" + VISION_LINE + "\n- Not available")
                print("  " + text.replace("\n", "\n  "))
                content = text + "\n\nTRANSCRIPT: " + heard
                if sens:
                    sens["_at"] = datetime.datetime.now().strftime("%H:%M:%S")
                    prev = sens
                    with open(LAST_SENSES, "w") as f:
                        json.dump(sens, f)
            if frame is not None:
                # keep only the newest frame in the history
                for m in history:
                    if m["role"] == "user" and isinstance(m["content"], list):
                        m["content"] = [b for b in m["content"] if b["type"] == "text"] + \
                                       [{"type": "text", "text": "(earlier camera frame removed)"}]
                content = [{"type": "image", "source": {"type": "base64", "media_type": "image/jpeg",
                            "data": base64.b64encode(frame).decode()}},
                           {"type": "text", "text": content}]
            history.append({"role": "user", "content": content})
            raw, said = reply(env, history, args.senses)
            t_llm = time.time() - t0
            history.append({"role": "assistant", "content": said})
            print(f"cassian> {said}")
            info = speak(said)
            last_play = info
            if info["play_rc"] != 0:
                print(f"  playback failed (ssh/play_pcm exit {info['play_rc']})")
            with open(log_path, "a") as f:
                f.write(json.dumps({"t": datetime.datetime.now().isoformat(timespec="seconds"),
                                    "heard": heard, "raw_reply": raw, "said": said,
                                    "llm_s": round(t_llm, 2), "senses": sens, "frame": frame is not None, **info}) + "\n")
        except KeyboardInterrupt:
            print("\nbye")
            return 0
        except Exception as e:  # keep the loop alive on a bad turn
            print(f"  error: {e}")


if __name__ == "__main__":
    sys.exit(main())
