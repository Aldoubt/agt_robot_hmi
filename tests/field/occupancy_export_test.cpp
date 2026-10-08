#include <QCoreApplication>
#include <QTemporaryDir>
#include <fstream>
#include "map/occupancy_map.h"
#define CHECK(x)                                                          \
  do {                                                                    \
    if (!(x)) {                                                           \
      std::cerr << "failed line " << __LINE__ << ": " << #x << std::endl; \
      return 1;                                                           \
    }                                                                     \
  } while (0)
int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  QTemporaryDir temp;
  CHECK(temp.isValid());
  basic::OccupancyMap map(2, 3, Eigen::Vector3d(-1.234567890123, 2.345678901234, .123456789012), .123456789012);
  map.map_data << 0, -1, 100, 20, 80, 10;
  std::string base = temp.path().toStdString() + "/map";
  CHECK(map.Save(base + ".yaml"));
  auto yaml = YAML::LoadFile(base + ".yaml");
  CHECK(yaml["image"].as<std::string>() == "map.pgm");
  CHECK(yaml["mode"].as<std::string>() == "trinary");
  CHECK(yaml["free_thresh"].as<double>() < 50. / 255.);
  CHECK(yaml["resolution"].as<double>() == map.map_config.resolution);
  for (int i = 0; i < 3; ++i) CHECK(yaml["origin"][i].as<double>() == map.map_config.origin[i]);
  std::ifstream image(base + ".pgm", std::ios::binary);
  std::string line;
  for (int i = 0; i < 4; ++i) std::getline(image, line);
  std::string pixels((std::istreambuf_iterator<char>(image)), {});
  CHECK(pixels.size() == 6);
  CHECK(static_cast<unsigned char>(pixels[0]) == 254);
  CHECK(static_cast<unsigned char>(pixels[1]) == 205);
  CHECK(static_cast<unsigned char>(pixels[2]) == 0);
  CHECK(static_cast<unsigned char>(pixels[3]) == 205);
  CHECK(static_cast<unsigned char>(pixels[4]) == 0);
  CHECK(static_cast<unsigned char>(pixels[5]) == 254);
  basic::OccupancyMap loaded;
  CHECK(loaded.Load(base + ".yaml"));
  CHECK(loaded.map_data(0, 0) == 0 && loaded.map_data(0, 1) == -1 && loaded.map_data(0, 2) == 100);
  CHECK(loaded.map_data(1, 0) == -1 && loaded.map_data(1, 1) == 100 && loaded.map_data(1, 2) == 0);
  CHECK(!map.Save(temp.path().toStdString() + "/missing/map"));
  map.map_data(0, 0) = 101;
  CHECK(!map.Save(base));
  std::cout << "Qt PGM/YAML occupancy/orientation/precision/errors PASS" << std::endl;
}
