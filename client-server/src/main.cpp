#include "client-server.hpp"
#include "engine.hpp"
#include "json/json.h"
#include <SFML/System/Vector2.hpp>
#include <X11/X.h>
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
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

#define MAXLINE 1024
#define JSON_USE_STRING_VIEW 0

#ifdef __linux__
int flags = MSG_CONFIRM;
#elif __APPLE__
int flags = 0; // macOS doesn't support MSG_CONFIRM
#else
int flags = 0;
#endif

struct ConnectionConfig {
  bool start;
  bool isServer;
  std::string ip;
  int port;
};

ConnectionConfig showLauncher() {
  sf::RenderWindow launcher(sf::VideoMode(400, 300), "Planet Sim Launcher");
  launcher.setFramerateLimit(60);

  if (!ImGui::SFML::Init(launcher))
    return {false, false, "", 0};

  sf::Clock clock;
  ConnectionConfig config = {false, false, "127.0.0.1", 8080};

  static char ipBuffer[64] = "127.0.0.1";
  static char portBuffer[16] = "8080";

  static int selectedMode = 0;

  while (launcher.isOpen()) {
    sf::Event event;
    while (launcher.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(launcher, event);
      if (event.type == sf::Event::Closed) {
        launcher.close();
        return {false, false, "", 0};
      }
    }

    ImGui::SFML::Update(launcher, clock.restart());
    launcher.clear(sf::Color(40, 40, 50));

    // launcher's main window
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(360, 260));

    ImGui::Begin("Planet Simulation Launcher", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Select mode:");
    ImGui::RadioButton("Host", &selectedMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Client", &selectedMode, 1);

    ImGui::Separator();

    if (selectedMode == 0) {
      // host mode
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Host Configuration");

      ImGui::Text("Send to IP:");
      ImGui::InputText("##HostIP", ipBuffer, sizeof(ipBuffer));
      ImGui::SameLine();
      if (ImGui::SmallButton("255")) {
        strcpy(ipBuffer, "255.255.255.255");
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("127")) {
        strcpy(ipBuffer, "127.0.0.1");
      }

      ImGui::Text("Port to send on:");
      ImGui::InputText("##HostPort", portBuffer, sizeof(portBuffer));

      ImGui::Spacing();
      ImGui::Text("Clients should listen on the same port.");

      if (ImGui::Button("Start Host", ImVec2(150, 40))) {
        std::string portStr = portBuffer;
        std::stringstream ss(portStr);
        int port;
        if (ss >> port && port > 0 && port < 65536) {
          config.start = true;
          config.isServer = true;
          config.ip = ipBuffer;
          config.port = port;
          launcher.close();
        } else {
          ImGui::OpenPopup("Invalid Port");
        }
      }
    } else {
      // client mode
      ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f),
                         "Client Configuration");

      ImGui::Text("Listen on IP:");
      ImGui::InputText("##ClientIP", ipBuffer, sizeof(ipBuffer));
      ImGui::SameLine();
      if (ImGui::SmallButton("0.0.0.0")) {
        strcpy(ipBuffer, "0.0.0.0");
      }

      ImGui::Text("Port to listen on:");
      ImGui::InputText("##ClientPort", portBuffer, sizeof(portBuffer));

      ImGui::Spacing();
      ImGui::Text("Client will listen for incoming coordinates.");
      ImGui::Text("Make sure this matches the host's settings.");

      if (ImGui::Button("Start Client", ImVec2(150, 40))) {
        std::string portStr = portBuffer;
        std::stringstream ss(portStr);
        int port;
        if (ss >> port && port > 0 && port < 65536) {
          config.start = true;
          config.isServer = false;
          config.ip = ipBuffer; // Listen IP
          config.port = port;
          launcher.close();
        } else {
          ImGui::OpenPopup("Invalid Port");
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(150, 40))) {
      launcher.close();
      return {false, false, "", 0};
    }

    if (ImGui::BeginPopupModal("Invalid Port", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Please enter a valid port number (1-65535)");
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::End();

    ImGui::SFML::Render(launcher);
    launcher.display();
  }

  ImGui::SFML::Shutdown();
  return config;
}

int main() {
  ConnectionConfig config = showLauncher();

  if (!config.start) {
    return 0;
  }

  std::mutex planets_mutex;

#if 1
  int server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0) {
    perror("server socket creation failed");
    exit(EXIT_FAILURE);
  }

  int broadcast_enable = 1;
  setsockopt(server_sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
             sizeof(broadcast_enable));

  int client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_sockfd < 0) {
    perror("client socket creation failed");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(config.port);
  client_addr.sin_addr.s_addr = INADDR_ANY;

  int opt = 1;
  setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(client_sockfd, (const struct sockaddr *)&client_addr,
           sizeof(client_addr)) < 0) {
    perror("client bind failed");
    close(client_sockfd);
    close(server_sockfd);
    return 1;
  }

  struct timeval tv;
  tv.tv_sec = 1; // 1 second timeout
  tv.tv_usec = 0;
  setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  char buffer[MAXLINE];
#endif

  // engine part
  std::vector<Planet> planets;

  // plantet.json parsing
  if (config.isServer == true) {
    std::ifstream planet_file("assets/planets.json", std::ifstream::binary);
    if (!planet_file) {
      std::cerr << "can't open planet.json";
      close(server_sockfd);
      close(client_sockfd);
      return 1;
    }
    Json::Value planets_info;
    planet_file >> planets_info;

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
  }

  // window creation
  sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "2d-engine",
                          sf::Style::Fullscreen);
  //
  // sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "cyan planet");

  window.setFramerateLimit(60);

  float G = 100.0f;
  float timeStep = 0.016f;
  std::vector<std::vector<sf::Vertex>> trajectories;

  std::thread client_receive_thread(client_receive, client_sockfd, &planets,
                                    &planets_mutex, &trajectories);
  std::thread server_send_thread;

  if (config.isServer == true) {
    server_send_thread =
        std::thread(server_send_broadcast, server_sockfd, &planets,
                    &planets_mutex, config.port, config.ip, &G, &timeStep);
  }

  sf::View camera = window.getDefaultView();
  float cameraSpeed = 5.0f;
  int scrollBorder = 10;

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
    if (config.isServer == 1) {
      static sf::Clock deltaClock;
      ImGui::SFML::Update(window, deltaClock.restart());

      ImGui::Begin("Simulation (host)", nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize);

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
      ImGui::Text("Create...");
      ImGui::PushItemWidth(100.0f);

      ImGui::Text("Radius");
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
    } else {
      static sf::Clock deltaClock;
      ImGui::SFML::Update(window, deltaClock.restart());

      ImGui::Begin("Simulation (client)", nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Separator();
      ImGui::Text("Camera:");
      ImGui::SliderFloat("Speed", &cameraSpeed, 1.0f, 50.0f);
      if (ImGui::Button("Reser view")) {
        camera = window.getDefaultView();
      }
      ImGui::Separator();
      ImGui::Text("Planets: %d", (int)planets.size());

      ImGui::Separator();
      ImGui::Text("Trajectories");
      if (ImGui::Button("Clear trajectories")) {
        for (auto &trail : trajectories) {
          trail.clear();
        }
      }
      ImGui::End();
    }
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2u windowSize = window.getSize();

    // move camera
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      camera.move(0, -cameraSpeed);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      camera.move(0, cameraSpeed);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      camera.move(-cameraSpeed, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      camera.move(cameraSpeed, 0);
    }

    window.setView(camera);

    window.clear();

    {
      std::lock_guard<std::mutex> lock(planets_mutex);

      if (trajectories.size() < planets.size()) {
        for (size_t i = trajectories.size(); i < planets.size(); i++) {
          trajectories.push_back(std::vector<sf::Vertex>());
        }
      } else if (trajectories.size() > planets.size()) {
        trajectories.resize(planets.size());
      }

      for (size_t i = 0; i < planets.size(); i++) {
        if (i < trajectories.size()) {
          sf::Color trailColor = planets[i].getShape().getFillColor();
          trailColor.a = 200;
          trajectories[i].push_back(
              sf::Vertex(planets[i].getPosition(), trailColor));

          const size_t max_trajectory_points = 1000;
          if (trajectories[i].size() > max_trajectory_points) {
            trajectories[i].erase(trajectories[i].begin());
          }

          if (trajectories[i].size() > 1) {
            window.draw(&trajectories[i][0], trajectories[i].size(),
                        sf::LineStrip);
          }
        }

        planets[i].draw(window);
      }
    }
    ImGui::SFML::Render(window);
    window.display();
  }
  ImGui::SFML::Shutdown();

  // thread termination
  clientRunning = false;

  if (client_receive_thread.joinable()) {
    client_receive_thread.join();
  }

  if (server_send_thread.joinable()) {
    server_send_thread.join();
  }

  close(server_sockfd);
  close(client_sockfd);

  return 0;
}
