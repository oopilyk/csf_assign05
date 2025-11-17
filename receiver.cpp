#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // connect
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Failed to connect to server\n";
    return 1;
  }

  Message rlogin_msg(TAG_RLOGIN, username);
  if (!conn.send(rlogin_msg)) {
    std::cerr << "Failed to send rlogin message\n";
    return 1;
  }

  // Receive response to rlogin
  Message response;
  if (!conn.receive(response)) {
    std::cerr << "Failed to receive rlogin response\n";
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << "\n";
    return 1;
  }

  if (response.tag != TAG_OK) {
    std::cerr << "Unexpected response to rlogin\n";
    return 1;
  }

  Message join_msg(TAG_JOIN, room_name);
  if (!conn.send(join_msg)) {
    std::cerr << "Failed to send join message\n";
    return 1;
  }

  if (!conn.receive(response)) {
    std::cerr << "Failed to receive join response\n";
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << "\n";
    return 1;
  }

  if (response.tag != TAG_OK) {
    std::cerr << "Unexpected response to join\n";
    return 1;
  }

  while (true) {
    Message msg;
    if (!conn.receive(msg)) {
      // connection was closed
      break;
    }

    if (msg.tag == TAG_DELIVERY) {
      size_t first_colon = msg.data.find(':');
      if (first_colon != std::string::npos) {
        size_t second_colon = msg.data.find(':', first_colon + 1);
        if (second_colon != std::string::npos) {
          std::string sender = msg.data.substr(first_colon + 1, second_colon - first_colon - 1);
          std::string message_text = msg.data.substr(second_colon + 1);
          std::cout << sender << ": " << message_text << "\n";
        }
      }
    }
  }

  return 0;
}
