#include <cassert>
#include <ctime>
#include "message.h"
#include "message_queue.h"

MessageQueue::MessageQueue() {
  //initializes the mutex and the semaphore
  pthread_mutex_init(&m_lock, nullptr);
  sem_init(&m_avail, 0, 0);
}

MessageQueue::~MessageQueue() {
  //destroys the mutex and the semaphore
  pthread_mutex_lock(&m_lock);

  while(!m_messages.empty()) {
    Message * m = m_messages.front();
    m_messages.pop_front();
    delete m;
  }

  pthread_mutex_unlock(&m_lock);

  pthread_mutex_destroy(&m_lock);
  sem_destroy(&m_avail);
}

void MessageQueue::enqueue(Message *msg) {
  //puts the specified message on the queue
  pthread_mutex_lock(&m_lock);

  m_messages.push_back(msg);

  pthread_mutex_unlock(&m_lock);

  sem_post(&m_avail);
}

Message *MessageQueue::dequeue() {
  if (sem_wait(&m_avail) == -1) {
      return nullptr;
  }

  pthread_mutex_lock(&m_lock);

  Message *msg = m_messages.front();
  m_messages.pop_front();

  pthread_mutex_unlock(&m_lock);

  return msg;
}
