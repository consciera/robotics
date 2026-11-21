// probe_video.cpp: CAS-R1, session 05
//
// Asks the "videohub" service (Go2's video client in the SDK) for ONE camera
// frame and saves it as a JPEG. A request for a picture: it moves nothing.
// Same idea as session 03, where the R1 answered the G1's voice client.
//
// Usage (on the Jetson):  ./probe_video eth10 /tmp/cas_frame.jpg

#include <cstdio>
#include <fstream>
#include <vector>

#include <unitree/robot/go2/video/video_client.hpp>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <network_interface> <out.jpg>\n", argv[0]);
    return 1;
  }
  unitree::robot::ChannelFactory::Instance()->Init(0, argv[1]);
  unitree::robot::go2::VideoClient vc;
  vc.SetTimeout(3.0f);
  vc.Init();

  std::vector<uint8_t> img;
  int ret = vc.GetImageSample(img);
  std::printf("GetImageSample rc=%d bytes=%zu\n", ret, img.size());
  if (ret != 0 || img.empty()) return 2;
  std::ofstream f(argv[2], std::ios::binary);
  f.write(reinterpret_cast<const char *>(img.data()), img.size());
  std::printf("saved %s\n", argv[2]);
  return 0;
}
