// play_pcm.cpp: CAS-R1, session 02
//
// Plays a raw audio file through the robot's own speaker, via the voice
// service's play-stream call (G1 audio client; the R1 answers it).
// Made for ElevenLabs output from tts_elevenlabs.py on the laptop.
//
// The file must be raw PCM: 16 kHz, mono, 16-bit little-endian. That's what
// the SDK's G1 example requires (it rejects anything else), and it's what
// ElevenLabs' "pcm_16000" format produces.
//
// Sent in 96000-byte chunks (3 s each) under one stream id, the same way the
// SDK's G1 audio example does it. Sound only: no motors, no LED.
//
// Usage (on the Jetson):  ./play_pcm eth10 file.pcm [volume 0-100]
// With no volume given, the current volume is left as it is.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>

static const size_t kChunk = 96000;  // 3 s at 16 kHz mono 16-bit, as in the SDK example

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <network_interface> file.pcm [volume]\n", argv[0]);
    return 1;
  }
  std::ifstream f(argv[2], std::ios::binary);
  if (!f) {
    std::printf("ABORT: cannot open %s\n", argv[2]);
    return 2;
  }
  std::vector<uint8_t> pcm((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (pcm.empty() || pcm.size() % 2) {
    std::printf("ABORT: %s is empty or not 16-bit PCM (%zu bytes)\n", argv[2], pcm.size());
    return 2;
  }
  const double secs = pcm.size() / 32000.0;

  unitree::robot::ChannelFactory::Instance()->Init(0, argv[1]);
  unitree::robot::g1::AudioClient client;
  client.Init();
  client.SetTimeout(5.0f);

  uint8_t original = 0;
  int32_t rc = client.GetVolume(original);
  if (rc != 0) {
    std::printf("ABORT: voice service not answering (code %d). Nothing played.\n", rc);
    return 3;
  }
  bool changed = false;
  if (argc > 3) {
    int v = std::atoi(argv[3]);
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    rc = client.SetVolume(uint8_t(v));
    changed = (rc == 0);
    std::printf("volume %u -> %d (code %d)\n", unsigned(original), v, rc);
  } else {
    std::printf("volume %u (unchanged)\n", unsigned(original));
  }

  const std::string stream_id = std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count());
  // Pace the chunks: sending them all at once cut a 7-chunk clip off after
  // ~4 chunks (session 02). Wait 2.5 s per 3 s chunk so the service only
  // ever holds a couple of chunks.
  const auto t0 = std::chrono::steady_clock::now();
  int n = 0;
  for (size_t off = 0; off < pcm.size(); off += kChunk, n++) {
    if (n > 0) std::this_thread::sleep_until(t0 + std::chrono::milliseconds(2500 * n));
    size_t len = std::min(kChunk, pcm.size() - off);
    std::vector<uint8_t> chunk(pcm.begin() + off, pcm.begin() + off + len);
    rc = client.PlayStream("cas-r1", stream_id, chunk);
    std::printf("chunk %d: %zu bytes, code %d\n", n, len, rc);
    std::fflush(stdout);
  }
  // Wait until the whole clip has had time to play, plus 1 s.
  std::this_thread::sleep_until(t0 + std::chrono::milliseconds(int(secs * 1000) + 1000));
  std::printf("played ~%.1f s\n", secs);

  if (changed) {
    rc = client.SetVolume(original);
    std::printf("volume restored to %u (code %d)\n", unsigned(original), rc);
  }
  return 0;
}
