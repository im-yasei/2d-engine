#include "engine.hpp"
#include "json/json.h"
#include <SFML/System/Vector2.hpp>
#include <X11/X.h>
#include <arpa/inet.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <imgui-SFML.h>
#include <imgui.h>
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
  const float FRICTION_COEFFICIENT = 0.08f;
  for (int i = 0; i < planets.size(); i++) {
    for (int j = i + 1; j < planets.size(); j++) {

      if (planets[i].isColliding(planets[j])) {

        sf::Vector2f collisionVector =
            planets[j].getPosition() - planets[i].getPosition();
        float distance = std::sqrt(collisionVector.x * collisionVector.x +
                                   collisionVector.y * collisionVector.y);

        if (distance > 0) {
          collisionVector /= distance;

          float m1 = planets[i].getMass(), m2 = planets[j].getMass();
          float totalMass = m1 + m2;
          float ratio1 = m2 / totalMass;
          float ratio2 = m1 / totalMass;

          float overlap =
              (planets[i].getRadius() + planets[j].getRadius()) - distance;
          planets[i].setPosition(planets[i].getPosition() -
                                 collisionVector * overlap * ratio1);
          planets[j].setPosition(planets[j].getPosition() +
                                 collisionVector * overlap * ratio2);

          sf::Vector2f v1 = planets[i].getVelocity();
          sf::Vector2f v2 = planets[j].getVelocity();

          float v1n = v1.x * collisionVector.x + v1.y * collisionVector.y;
          float v2n = v2.x * collisionVector.x + v2.y * collisionVector.y;

          float restitution = 0.0f;
          float u1n =
              ((m1 - restitution * m2) * v1n + (1 + restitution) * m2 * v2n) /
              (m1 + m2);
          float u2n =
              ((m2 - restitution * m1) * v2n + (1 + restitution) * m1 * v1n) /
              (m1 + m2);

          planets[i].setVelocity(v1 + collisionVector * (u1n - v1n));
          planets[j].setVelocity(v2 + collisionVector * (u2n - v2n));

          // friction
          sf::Vector2f tangent(-collisionVector.y, collisionVector.x);

          float v1t = v1.x * tangent.x + v1.y * tangent.y;
          float v2t = v2.x * tangent.x + v2.y * tangent.y;

          float relativeTangentialSpeed = std::abs(v1t - v2t);

          if (relativeTangentialSpeed > 0.1f) {
            v1t *= (1.0f - FRICTION_COEFFICIENT);
            v2t *= (1.0f - FRICTION_COEFFICIENT);

            float new_v1n = planets[i].getVelocity().x * collisionVector.x +
                            planets[i].getVelocity().y * collisionVector.y;
            float new_v2n = planets[j].getVelocity().x * collisionVector.x +
                            planets[j].getVelocity().y * collisionVector.y;

            planets[i].setVelocity(collisionVector * new_v1n + tangent * v1t);
            planets[j].setVelocity(collisionVector * new_v2n + tangent * v2t);
          }
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

  // plantet.json parsing
  std::ifstream planet_file("assets/planets.json", std::ifstream::binary);
  if (!planet_file) {
    std::cerr << "can't open planet.json";
    close(sockfd);
    return 1;
  }
  Json::Value planets_info;
  planet_file >> planets_info;

  // window creation
  sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "cyan planet",
                          sf::Style::Fullscreen);
  // sf::RenderWindow
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

  sf::View camera = window.getDefaultView();
  float cameraSpeed = 5.0f;
  int scrollBorder = 10;

  float G = 100.0f;
  float timeStep = 0.016f;

  // trajectories vector
  std::vector<std::vector<sf::Vertex>> trajectories(planets.size());

  if (!ImGui::SFML::Init(window)) {
    std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    return 1;
  }

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(window, event);
      if (event.type == sf::Event::Closed)
        window.close();
    }

    static sf::Clock deltaClock;
    ImGui::SFML::Update(window, deltaClock.restart());

    ImGui::Begin("Simulation menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::SliderFloat("Gravitation", &G, 0.0f, 1000.0f, "%.1f");
      ImGui::SliderFloat("Time step", &timeStep, 0.001f, 0.1f, "%.3f");
    }

    ImGui::Separator();
    ImGui::Text("Camera:");
    ImGui::SliderFloat("Speed", &cameraSpeed, 1.0f, 50.0f);
    if (ImGui::Button("Reser view")) {
      camera = window.getDefaultView();
    }

    ImGui::Separator();
    ImGui::Text("Planets: %d", (int)planets.size());

    static float newRadius = 10.0f;
    static float newMass = 100.0f;

    // Planet creation
    ImGui::Separator();
    ImGui::Text("New planet");
    ImGui::PushItemWidth(100.0f);

    ImGui::InputFloat("Radius", &newRadius, 0.5f, 1.0f, "%.1f");
    ImGui::SameLine();
    ImGui::InputFloat("Mass", &newMass, 10.0f, 50.0f, "%.0f");

    static float posX = 400.0f, posY = 300.0f;
    ImGui::InputFloat("Position X", &posX, 10.0f, 50.0f, "%.0f");
    ImGui::SameLine();
    ImGui::InputFloat("Position Y", &posY, 10.0f, 50.0f, "%.0f");

    static float color[3] = {1.0f, 1.0f, 1.0f};
    ImGui::ColorEdit3("Color", color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    ImGui::Text("Color");

    static float velX = 0.0f, velY = 0.0f;
    ImGui::InputFloat("Velocity X", &velX, 0.1f, 1.0f, "%.1f");
    ImGui::SameLine();
    ImGui::InputFloat("Velocity Y", &velY, 0.1f, 1.0f, "%.1f");

    ImGui::PopItemWidth();

    if (ImGui::Button("Create a planet")) {
      Planet p(newRadius, newMass);
      p.setPosition(sf::Vector2f(posX, posY));
      p.setVelocity(sf::Vector2f(velX, velY));

      p.setColor(static_cast<int>(color[0] * 255),
                 static_cast<int>(color[1] * 255),
                 static_cast<int>(color[2] * 255));

      std::lock_guard<std::mutex> lock(planets_mutex);
      planets.push_back(p);
      trajectories.push_back(std::vector<sf::Vertex>());
    }

    ImGui::Separator();
    ImGui::Text("Trajectories");
    if (ImGui::Button("Clear trajectories")) {
      for (auto &trail : trajectories) {
        trail.clear();
      }
    }
    ImGui::End();

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2u windowSize = window.getSize();

    // move camera
    if (mousePos.x < scrollBorder) {
      camera.move(-cameraSpeed, 0);
    }
    if (mousePos.x > windowSize.x - scrollBorder) {
      camera.move(cameraSpeed, 0);
    }
    if (mousePos.y < scrollBorder) {
      camera.move(0, -cameraSpeed);
    }
    if (mousePos.y > windowSize.y - scrollBorder) {
      camera.move(0, cameraSpeed);
    }

    {
      std::lock_guard<std::mutex> lock(planets_mutex);
      applyGravity(planets, G, timeStep);
      applyCollision(planets);
    }

    window.setView(camera);

    window.clear();
    {
      std::lock_guard<std::mutex> lock(planets_mutex);
      for (int i = 0; i < planets.size(); i++) {

        sf::Color trailColor = planets[i].getShape().getFillColor();
        trailColor.a = 200;

        trajectories[i].push_back(
            sf::Vertex(planets[i].getPosition(), trailColor));

        if (trajectories[i].size() > 1) {
          window.draw(&trajectories[i][0], trajectories[i].size(),
                      sf::LineStrip);
        }

        planets[i].draw(window);
      }
    }

    ImGui::SFML::Render(window);
    window.display();
  }
  ImGui::SFML::Shutdown();

  // client thread termination
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
