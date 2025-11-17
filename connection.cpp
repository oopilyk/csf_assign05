#include <sstream>
#include <cctype>
#include <cassert>
#include <cstring>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
  std::string port_str = std::to_string(port);
  m_fd = open_clientfd(hostname.c_str(), port_str.c_str());
  if (m_fd < 0) {
    m_last_result = EOF_OR_ERROR;
    return;
  }
  rio_readinitb(&m_fdbuf, m_fd);
  m_last_result = SUCCESS;
}

Connection::~Connection() {
  if (is_open()) {
    close();
  }
}

bool Connection::is_open() const {
  return m_fd >= 0;
}

void Connection::close() {
  if (is_open()) {
    Close(m_fd);
    m_fd = -1;
  }
}

bool Connection::send(const Message &msg) {
  std::string formatted = msg.tag + ":" + msg.data + "\n";
  
  // message length check
  if (formatted.length() > Message::MAX_LEN) {
    m_last_result = INVALID_MSG;
    return false;
  }
  
  // send message
  ssize_t n = rio_writen(m_fd, formatted.c_str(), formatted.length());
  if (n != (ssize_t)formatted.length()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  
  m_last_result = SUCCESS;
  return true;
}

bool Connection::receive(Message &msg) {
  char buf[Message::MAX_LEN + 1];

  ssize_t n = rio_readlineb(&m_fdbuf, buf, Message::MAX_LEN + 1);
  
  if (n <= 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  std::string line(buf, n);
  while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
    line.pop_back();
  }
  
  // Parse the message
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string::npos) {
    m_last_result = INVALID_MSG;
    return false;
  }
  
  msg.tag = line.substr(0, colon_pos);
  msg.data = line.substr(colon_pos + 1);
  
  m_last_result = SUCCESS;
  return true;
}
