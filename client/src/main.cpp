#include "engine.hpp"
#include "json/json.h"
#include <SFML/System/Vector2.hpp>
#include <arpa/inet.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define PORT 8080
#define MAXLINE 1024
#define JSON_USE_STRING_VIEW 0

#ifdef __linux__
int flags = MSG_CONFIRM;
#elif __APPLE__
int flags = 0; // macOS doesn't support MSG_CONFIRM
#else
int flags = 0;
#endif

std::atomic<bool> clientRunning{true};

void client_send(int sockfd, struct sockaddr_in *servaddr,
                 std::vector<Planet> *planets, std::mutex *planets_mutex) {

  while (clientRunning) {
    std::stringstream coords_stream;
    {
      std::lock_guard<std::mutex> lock(*planets_mutex);
      for (int i = 0; i < planets->size(); i++) {
        coords_stream << "x" << std::to_string(i) << ":"
                      << std::to_string(planets->at(i).getPosition().x) << " y"
                      << std::to_string(i) << ":"
                      << std::to_string(planets->at(i).getPosition().y) << "\n";
      }
    }

    std::string coords = coords_stream.str();

    if (coords.length() > 65535) {
      std::cerr << "Warning: packet too large (" << coords.length() << " bytes)"
                << std::endl;
      coords = coords.substr(0, 65535);
    }

    int bytes_sent =
        sendto(sockfd, coords.c_str(), coords.length(), flags,
               (const struct sockaddr *)servaddr, sizeof(*servaddr));

    if (bytes_sent < 0) {
      perror("sendto failed");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}
void client_receive(int sockfd, char *buffer, struct sockaddr_in *servaddr) {

  while (clientRunning) {
    socklen_t len = sizeof(*servaddr);

    int n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
                     (struct sockaddr *)servaddr, &len);

    if (n > 0) {
      buffer[n] = '\0';
      std::cout << "Server: " << buffer;
    } else if (n == 0) {
      std::cout << "Server closed connection";
      break;
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      perror("recvfrom failed");
      break;
    }
  }
}

void applyGravity(std::vector<Planet> &planets, float G, float timeStep) {
  for (int i = 0; i < planets.size(); i++) {
    sf::Vector2f totalForce(0, 0);

    for (int j = 0; j < planets.size(); j++) {
      if (i == j)
        continue;

      sf::Vector2f direction =
          planets[j].getPosition() - planets[i].getPosition();

      float distance =
          std::sqrt(direction.x * direction.x + direction.y * direction.y);

      if (distance < 1.0f)
        continue;

      direction /= distance;

      float forceMagnitude = G * planets[i].getMass() * planets[j].getMass() /
                             (distance * distance);

      totalForce += direction * forceMagnitude;
    }

    sf::Vector2f newAcceleration = totalForce / planets[i].getMass();
    planets[i].setAcceleration(newAcceleration);

    sf::Vector2f newVelocity =
        planets[i].getVelocity() + newAcceleration * timeStep;
    planets[i].setVelocity(newVelocity);

    sf::Vector2f newPosition =
        planets[i].getPosition() + newVelocity * timeStep;
    planets[i].setPosition(newPosition);
  }
}

void applyCollision(std::vector<Planet> &planets) {
  for (int i = 0; i < planets.size(); i++) {
    for (int j = i + 1; j < planets.size(); j++) {

      if (planets[i].isColliding(planets[j])) {

        sf::Vector2f collisionVector =
            planets[j].getPosition() - planets[i].getPosition();
        float distance = std::sqrt(collisionVector.x * collisionVector.x +
                                   collisionVector.y * collisionVector.y);

        if (distance > 0) {
          collisionVector /= distance;

          float overlap =
              (planets[i].getRadius() + planets[j].getRadius()) - distance;
          planets[i].setPosition(planets[i].getPosition() -
                                 collisionVector * overlap * 0.5f);
          planets[j].setPosition(planets[j].getPosition() +
                                 collisionVector * overlap * 0.5f);

          sf::Vector2f v1 = planets[i].getVelocity();
          sf::Vector2f v2 = planets[j].getVelocity();
          float m1 = planets[i].getMass();
          float m2 = planets[j].getMass();

          float v1n = v1.x * collisionVector.x + v1.y * collisionVector.y;
          float v2n = v2.x * collisionVector.x + v2.y * collisionVector.y;

          float restitution = 0.9f;
          float u1n =
              ((m1 - m2) * v1n + 2 * m2 * v2n) / (m1 + m2) * restitution;
          float u2n =
              ((m2 - m1) * v2n + 2 * m1 * v1n) / (m1 + m2) * restitution;

          planets[i].setVelocity(v1 + collisionVector * (u1n - v1n));
          planets[j].setVelocity(v2 + collisionVector * (u2n - v2n));
        }
      }
    }
  }
}

int main() {
  std::mutex planets_mutex;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  char buffer[MAXLINE];
  struct sockaddr_in servaddr;
  struct timeval tv;
  tv.tv_sec = 1; // 1 second timeout
  tv.tv_usec = 0;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  // servaddr.sin_addr.s_addr = INADDR_ANY;
  const char *SERVER_IP = "127.0.0.1";
  if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
    std::cerr << "Invalid server IP address!" << std::endl;
    close(sockfd);
    return 1;
  }

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  // engine part
  std::vector<Planet> planets;

  // plantet.json parsiong
  std::ifstream planet_file("assets/planet.json", std::ifstream::binary);
  if (!planet_file) {
    std::cerr << "can't open planet.json";
    close(sockfd);
    return 1;
  }
  Json::Value planets_info;
  planet_file >> planets_info;

  // window creation
  sf::RenderWindow window(
      sf::VideoMode::getDesktopMode(), "cyan planet",
      sf::Style::Fullscreen); // sf::RenderWindow
                              // window(sf::VideoMode::getDesktopMode(),
                              // "cyan planet"); - for window mode

  window.setFramerateLimit(60);

  const Json::Value &planets_array = planets_info["Planets"];

  // Planet init
  for (const auto &planet_data : planets_array) {
    sf::Vector2f planetPosition, planetVelocity;

    Planet p(planet_data["radius"].asFloat(), planet_data["mass"].asFloat());

    planetPosition.x = planet_data["x"].asFloat();
    planetPosition.y = planet_data["y"].asFloat();

    planetVelocity.x = planet_data["velocity"][0].asFloat();
    planetVelocity.y = planet_data["velocity"][1].asFloat();

    if (planet_data.isMember("color")) {
      p.setColor(planet_data["color"][0].asInt(),
                 planet_data["color"][1].asInt(),
                 planet_data["color"][2].asInt());
    }

    p.setPosition(planetPosition);
    p.setVelocity(planetVelocity);

    planets.push_back(p);
  }

  std::thread client_receive_thread(client_receive, sockfd, buffer, &servaddr);
  std::thread client_send_thread(client_send, sockfd, &servaddr, &planets,
                                 &planets_mutex);

  const float G = 100.0f;
  const float timeStep = 0.016f;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        window.close();
    }

    {
      std::lock_guard<std::mutex> lock(planets_mutex);
      applyGravity(planets, G, timeStep);
      applyCollision(planets);
    }

    window.clear();

    {
      std::lock_guard<std::mutex> lock(planets_mutex);
      for (int i = 0; i < planets.size(); i++) {
        planets[i].draw(window);
      }
    }

    window.display();
  }

  // clinet thread termination
  clientRunning = false;

  if (client_receive_thread.joinable()) {
    client_receive_thread.join();
  }

  if (client_send_thread.joinable()) {
    client_send_thread.join();
  }

  close(sockfd);

  return 0;
}
