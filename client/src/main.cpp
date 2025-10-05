#include "engine.hpp"
#include <SFML/System/Vector2.hpp>
#include <arpa/inet.h>
#include <atomic>
#include <bits/stdc++.h>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <math.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#define PORT 8080
#define MAXLINE 1024

std::atomic<bool> clientRunning{true};

void client_send(int sockfd, struct sockaddr_in *servaddr,
                 sf::Vector2f *planetPosition) {

  std::string coords;

  while (clientRunning) {
    coords = "x:" + std::to_string(planetPosition->x) +
             " y:" + std::to_string(planetPosition->y) + "\n";

    sendto(sockfd, coords.c_str(), coords.length(), MSG_CONFIRM,
           (const struct sockaddr *)servaddr, sizeof(*servaddr));

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

int main() {
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
  Planet planet;
  sf::Vector2f planetPosition;
  sf::Vector2f rotationCenter;
  int moveEveryNFrames = 1, frameCounter = 0;
  float t = 0; // parameter

  std::thread client_receive_thread(client_receive, sockfd, buffer, &servaddr);
  std::thread client_send_thread(client_send, sockfd, &servaddr,
                                 &planetPosition);

  // plantet.json parsiong
  std::ifstream planet_file("assets/planet.json", std::ifstream::binary);
  if (!planet_file) {
    std::cerr << "can't open planet.json";
    return 1;
  }
  Json::Value planet_info;
  planet_file >> planet_info;

  // planet.json into a string
  Json::StreamWriterBuilder writer;
  std::string planet_info_str = Json::writeString(writer, planet_info);

  // window creation
  sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "cyan planet");
  window.setFramerateLimit(60);

  planet.setRadius(planet_info["Planet"]["radius"].asFloat());
  planet.init();
  rotationCenter.x = planet_info["Planet"]["x"].asFloat();
  rotationCenter.y = planet_info["Planet"]["y"].asFloat();

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        window.close();
    }
    frameCounter++;
    if (frameCounter >= moveEveryNFrames) {
      planetPosition.x = rotationCenter.x + 100 * cos(t);
      planetPosition.y = rotationCenter.y + 100 * sin(t);
      frameCounter = 0;
    }
    t += 0.05; // velocity = 0.05
    planet.setPosition(planetPosition);

    window.clear();
    planet.draw(window);
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
