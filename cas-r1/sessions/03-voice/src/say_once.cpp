// say_once.cpp: CAS-R1, session 02
//
// Makes the robot say one line through its own speaker, using the G1 audio
// client's text-to-speech (the R1's main board answers the G1 "voice" service).
//
// Sound only: no motors, no LED. Steps:
//   1. read the current volume and remember it
//   2. set the speaking volume (default 40; it was at 100, the maximum)
//   3. speak the text once
//   4. wait for it to finish, then restore the original volume
//
// Usage (on the Jetson):  ./say_once eth10 "text" [speaker_id] [volume 0-100, default 40]
// speaker_id: which TTS voice. G1 examples use 0 for Chinese and 1 for English
// (unverified on R1). Default 1.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>

static const uint8_t kDefaultVolume = 40;

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <network_interface> \"text\" [speaker_id]\n", argv[0]);
    return 1;
  }
  const std::string text = argv[2];
  const int32_t speaker = argc > 3 ? std::atoi(argv[3]) : 1;
  int v = argc > 4 ? std::atoi(argv[4]) : kDefaultVolume;
  if (v < 0) v = 0;
  if (v > 100) v = 100;
  const uint8_t kSpeakVolume = uint8_t(v);

  unitree::robot::ChannelFactory::Instance()->Init(0, argv[1]);
  unitree::robot::g1::AudioClient client;
  client.Init();
  client.SetTimeout(5.0f);

  uint8_t original = 0;
  int32_t rc = client.GetVolume(original);
  if (rc != 0) {
    std::printf("ABORT: could not read volume (code %d). Nothing played.\n", rc);
    return 2;
  }
  rc = client.SetVolume(kSpeakVolume);
  std::printf("volume %u -> %u (code %d)\n", unsigned(original), unsigned(kSpeakVolume), rc);
  if (rc != 0) {
    std::printf("ABORT: could not lower volume. Nothing played.\n");
    return 3;
  }

  rc = client.TtsMaker(text, speaker);
  std::printf("speak \"%s\" (voice %d): code %d\n", text.c_str(), speaker, rc);

  // Rough time for the speech to finish before restoring the volume.
  unsigned wait_s = 3 + unsigned(text.size() / 10);
  sleep(wait_s);

  rc = client.SetVolume(original);
  std::printf("volume restored to %u (code %d)\n", unsigned(original), rc);
  return 0;
}
