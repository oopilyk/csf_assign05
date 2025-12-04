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
  members.insert(user);
}

void Room::remove_member(User *user) {
  //removes User from the room
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  //sends a message to every (receiver) User in the room
  Guard guard(lock);

  for(User * user : members) {

    if(user->username != sender_username) {

      Message * msg = new Message(sender_username, message_text);

      user->mqueue.enqueue(msg);

    }

  }
  
}
