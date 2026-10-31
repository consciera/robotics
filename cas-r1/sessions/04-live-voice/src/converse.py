"""converse.py: CAS-R1, session 04 (runs on the LAPTOP)

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

Usage:
  export R1=<jetson address>
  python3 converse.py              # push-to-talk: Enter to start, Enter to stop
  python3 converse.py --type       # type the questions instead of speaking
  python3 converse.py --say "text" # speak one line in Cassian's voice and exit
Ctrl+C quits.
"""

import argparse
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


def system_blocks():
    # Same layout as cassian-live: static soul and session soul each cached
    # for 1 h, then the small uncached directive.
    blocks = []
    for text in (load_soul(SOUL_STATIC), load_soul(SOUL_SESSION)):
        if text:
            blocks.append({"type": "text", "text": text,
                           "cache_control": {"type": "ephemeral", "ttl": "1h"}})
    blocks.append({"type": "text", "text": ROBOT_DIRECTIVE})
    return blocks


def reply(env, history):
    import anthropic
    client = anthropic.Anthropic(api_key=env["ANTHROPIC_API_KEY"])
    msg = client.messages.create(model=MODEL, max_tokens=MAX_TOKENS,
                                 system=system_blocks(), messages=history)
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
           "-o", "ControlPath=~/.ssh/cm-cas-r1", "-o", "ControlPersist=15m",
           f"unitree@{host}",
           f"cat > /tmp/cas_say.pcm && ~/play_pcm {iface} /tmp/cas_say.pcm"]
    r = subprocess.run(ssh, input=pcm, capture_output=True)
    out = r.stdout.decode(errors="replace").strip().splitlines()
    return r.returncode, out


# ---- main ---------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--type", action="store_true", help="type questions instead of speaking")
    ap.add_argument("--say", help="speak one line in Cassian's voice and exit")
    ap.add_argument("--mic", default=MIC)
    ap.add_argument("--iface", default="eth10")
    ap.add_argument("--voice-model", default=VOICE_MODEL,
                    help="ElevenLabs model, e.g. eleven_turbo_v2_5 (faster)")
    args = ap.parse_args()

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
    print("Cassian on the robot speaker. Ctrl+C to quit.\n")
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
            history.append({"role": "user", "content": heard})
            raw, said = reply(env, history)
            t_llm = time.time() - t0
            history.append({"role": "assistant", "content": said})
            print(f"cassian> {said}")
            info = speak(said)
            if info["play_rc"] != 0:
                print(f"  playback failed (ssh/play_pcm exit {info['play_rc']})")
            with open(log_path, "a") as f:
                f.write(json.dumps({"t": datetime.datetime.now().isoformat(timespec="seconds"),
                                    "heard": heard, "raw_reply": raw, "said": said,
                                    "llm_s": round(t_llm, 2), **info}) + "\n")
        except KeyboardInterrupt:
            print("\nbye")
            return 0
        except Exception as e:  # keep the loop alive on a bad turn
            print(f"  error: {e}")


if __name__ == "__main__":
    sys.exit(main())
