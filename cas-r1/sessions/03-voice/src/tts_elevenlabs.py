"""tts_elevenlabs.py: CAS-R1, session 02 (runs on the LAPTOP, which has internet)

Turns one line of text into raw audio with ElevenLabs, in the format the
robot's voice service is expected to take: 16 kHz, mono, 16-bit PCM (G1
convention, unverified on R1). The output file is copied to the Jetson and
played through the robot's speaker.

The API key and voice id are read from cassian-live's settings file. They are
never printed.

Usage:  python3 tts_elevenlabs.py "text" out.pcm [persona=CASSIAN] [model]
(model defaults to ELEVENLABS_MODEL from the settings file)
"""

import json
import os
import sys
import urllib.request

ENV_FILE = os.path.expanduser("~/Consciera/packages/cassian-live/.env.local")


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


def main():
    if len(sys.argv) < 3:
        print('usage: python3 tts_elevenlabs.py "text" out.pcm [persona]')
        return 1
    text, out = sys.argv[1], sys.argv[2]
    persona = (sys.argv[3] if len(sys.argv) > 3 else "CASSIAN").upper()

    env = load_env(ENV_FILE)
    key = env.get("ELEVENLABS_API_KEY")
    voice = env.get(f"ELEVENLABS_{persona}_VOICE_ID")
    model = (sys.argv[4] if len(sys.argv) > 4 else None) or env.get("ELEVENLABS_MODEL") or "eleven_multilingual_v2"
    if not key or not voice:
        print(f"missing ELEVENLABS_API_KEY or ELEVENLABS_{persona}_VOICE_ID in settings")
        return 2

    url = (f"https://api.elevenlabs.io/v1/text-to-speech/{voice}"
           "?output_format=pcm_16000")
    body = json.dumps({"text": text, "model_id": model}).encode()
    req = urllib.request.Request(url, data=body, method="POST", headers={
        "xi-api-key": key, "Content-Type": "application/json", "Accept": "audio/pcm"})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            pcm = r.read()
    except urllib.error.HTTPError as e:
        print(f"ElevenLabs error {e.code}: {e.read()[:200].decode(errors='replace')}")
        return 3

    with open(out, "wb") as f:
        f.write(pcm)
    secs = len(pcm) / (16000 * 2)
    print(f"wrote {out}: {len(pcm)} bytes, ~{secs:.1f} s of 16 kHz mono audio "
          f"(persona {persona}, model {model})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
