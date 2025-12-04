#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
  //initializes the mutex
  pthread_mutex_init(&lock, nullptr);
}

Room::~Room() {
  //destroys the mutex
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  //adds User to the room
  Guard guard(lock);
  members.insert(user);
}

void Room::remove_member(User *user) {
  //removes User from the room
  Guard guard(lock);
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  //sends a message to every (receiver) User in the room
  Guard guard(lock);

  // delivery:room:sender:message_text
  std::string payload = room_name + ":" + sender_username + ":" + message_text;

  for(User * user : members) {
    Message * msg = new Message(TAG_DELIVERY, payload);
    user->mqueue.enqueue(msg);
  }
  
}
