#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// pass data to worker threads
struct ClientInfo {
  Connection *conn;
  Server *server;
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {

void chat_with_sender(Connection *conn, Server *server, const std::string &username);
void chat_with_receiver(Connection *conn, Server *server, const std::string &username);

void *worker(void *arg) {
  pthread_detach(pthread_self());


  ClientInfo *info = static_cast<ClientInfo*>(arg);
  Connection *conn = info->conn;
  Server *server = info->server;

  // Read login message
  Message login_msg;
  if (!conn->receive(login_msg)) {
    if (conn->get_last_result() == Connection::INVALID_MSG) {
      conn->send(Message(TAG_ERR, "invalid message"));
    }
    delete conn;
    delete info;
    return nullptr;
  }

  // Check if sender or receiver
  if (login_msg.tag == TAG_SLOGIN) {
    conn->send(Message(TAG_OK, "welcome " + login_msg.data));
    chat_with_sender(conn, server, login_msg.data);
  } 
  else if (login_msg.tag == TAG_RLOGIN) {
    conn->send(Message(TAG_OK, "welcome " + login_msg.data));
    chat_with_receiver(conn, server, login_msg.data);
  } 
  else {
    // invalid
    conn->send(Message(TAG_ERR, "invalid login"));
  }

  delete conn;
  delete info;
  return nullptr;
}

void chat_with_sender(Connection *conn, Server *server, const std::string &username) {
  Room *current_room = nullptr;
  
  while (true) {
    Message msg;
    if (!conn->receive(msg)) {
      if (conn->get_last_result() == Connection::INVALID_MSG) {
        // Send error for invalid message and continue
        if (!conn->send(Message(TAG_ERR, "invalid message"))) {
          break;
        }
        continue;
      }
      break;
    }
    
    if (msg.tag == TAG_JOIN) {
      current_room = server->find_or_create_room(msg.data);
      if (!conn->send(Message(TAG_OK, "joined " + msg.data))) {
        break;
      }
    }
    else if (msg.tag == TAG_LEAVE) {
      if (!current_room) {
        if (!conn->send(Message(TAG_ERR, "not in a room"))) {
          break;
        }
      } else {
        current_room = nullptr;
        if (!conn->send(Message(TAG_OK, "left room"))) {
          break;
        }
      }
    }
    else if (msg.tag == TAG_SENDALL) {
      if (!current_room) {
        if (!conn->send(Message(TAG_ERR, "not in a room"))) {
          break;
        }
      } else {
        current_room->broadcast_message(username, msg.data);
        if (!conn->send(Message(TAG_OK, "message sent"))) {
          break;
        }
      }
    }
    else if (msg.tag == TAG_QUIT) {
      conn->send(Message(TAG_OK, "bye"));
      break;
    }
    else {
      if (!conn->send(Message(TAG_ERR, "invalid command"))) {
        break;
      }
    }
  }
}

void chat_with_receiver(Connection *conn, Server *server, const std::string &username) {
  User user(username);
  Room *current_room = nullptr;
  
  Message msg;
  if (!conn->receive(msg)) {
    // check if was an invalid message
    if (conn->get_last_result() == Connection::INVALID_MSG) {
      conn->send(Message(TAG_ERR, "invalid message"));
    }
    return;
  }
  
  if (msg.tag != TAG_JOIN) {
    conn->send(Message(TAG_ERR, "expected join"));
    return;
  }
  
  current_room = server->find_or_create_room(msg.data);
  current_room->add_member(&user);
  
  if (!conn->send(Message(TAG_OK, "joined " + msg.data))) {
    current_room->remove_member(&user);
    return;
  }
  
  while (true) {
    Message *delivery = user.mqueue.dequeue();
    
    if (delivery == nullptr) {
      continue;
    }
    
    if (!conn->send(*delivery)) {
      delete delivery;
      break;
    }
    
    delete delivery;
  }
  
  if (current_room) {
    current_room->remove_member(&user);
  }
}

}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  std::string port_str = std::to_string(m_port);
  m_ssock = open_listenfd(port_str.c_str());
  
  if (m_ssock < 0) {
    return false;
  }
  
  return true;
}

void Server::handle_client_requests() {
  while (true) {
    int client_fd = Accept(m_ssock, nullptr, nullptr);
    
    if (client_fd < 0) {
      continue;
    }
    
    Connection *conn = new Connection(client_fd);
    
    ClientInfo *info = new ClientInfo;
    info->conn = conn;
    info->server = this;
    
    pthread_t thread;
    if (pthread_create(&thread, nullptr, worker, info) != 0) {
      delete conn;
      delete info;
    }
  }
}

Room *Server::find_or_create_room(const std::string &room_name) {
  Guard guard(m_lock);
  
  RoomMap::iterator it = m_rooms.find(room_name);
  if (it != m_rooms.end()) {
    return it->second;
  }
  
  Room *new_room = new Room(room_name);
  m_rooms[room_name] = new_room;
  return new_room;
}
