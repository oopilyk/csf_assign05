#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  Connection conn;

  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Failed to connect to server\n";
    return 1;
  }

  Message slogin_msg(TAG_SLOGIN, username);
  if (!conn.send(slogin_msg)) {
    std::cerr << "Failed to send rlogin message\n";
    return 1;
  }

  Message response;
  if (!conn.receive(response)) {
    std::cerr << "Failed to receive slogin response\n";
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << "\n";
    return 1;
  }

  if (response.tag != TAG_OK) {
    std::cerr << "Unexpected response to slogin\n";
    return 1;
  }

  //       server as appropriate
  std::string input;
  while (true) {

    std::cout << "> ";

    if (!std::getline(std::cin, input)) {
      std::cout << "\nEOF detected, exiting.\n";
      break;
    }

    if (input.size() <= 0) continue;

    trim(input);
    if (input[0]=='/') {
      std::string command;
      std::string arg;

      size_t space = input.find(' ');
      if (space == std::string::npos) {
        command = input.substr(1);
      } else {
        command = input.substr(1, space-1);
        arg = input.substr(space+1);
      }

      if (command == "join") {
        Message join(TAG_JOIN, arg);
        conn.send(join);
        conn.receive(response);

        if (response.tag == TAG_ERR) {
          std::cerr << response.data << "\n";
        }
        continue;
      }

      if (command == "leave") {
        Message leave(TAG_LEAVE, "");
        conn.send(leave);
        conn.receive(response);

        if (response.tag == TAG_ERR) {
          std::cerr << response.data << "\n";
        }
        continue;
      }

      if (command == "quit") {
        Message quit(TAG_QUIT, "");
        conn.send(quit);
        conn.receive(response);

        if (response.tag == TAG_ERR) {
          std::cerr << response.data << "\n";
        }
        return 0;
      }

      std::cerr << "'" << command << "' is not a valid command\n";
      continue;

    } 

    Message sendall(TAG_SENDALL, input);
    conn.send(sendall);
    conn.receive(response);

    if (response.tag == TAG_ERR) {
      std::cerr << response.data << "\n";
    }
  }

  return 0;
}
