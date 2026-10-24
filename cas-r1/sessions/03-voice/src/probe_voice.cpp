// probe_voice.cpp: CAS-R1, session 02
//
// Asks one question of the robot's "voice" service, using the G1 audio client
// from the SDK (the SDK has no R1 audio client): "what is the volume?"
//
// GetVolume is a query. This program makes no sound, changes no setting, and
// calls nothing else (no TTS, no playback, no SetVolume, no LED).
//
//   answer, return code 0   -> the R1's main board runs a G1-compatible "voice" service
//   error / timeout         -> it doesn't (or not under that name/version)
//
// Usage (on the Jetson):  ./probe_voice eth10

#include <cstdint>
#include <cstdio>

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <network_interface>\n", argv[0]);
    return 1;
  }
  unitree::robot::ChannelFactory::Instance()->Init(0, argv[1]);

  unitree::robot::g1::AudioClient client;
  client.Init();
  client.SetTimeout(3.0f);

  uint8_t volume = 0;
  int32_t rc = client.GetVolume(volume);
  if (rc == 0)
    std::printf("voice service ANSWERED: volume = %u (return code 0)\n", unsigned(volume));
  else
    std::printf("voice service did not answer: return code %d\n", rc);
  return 0;
}
