#ifndef CHATROOM
#define CHATROOM

#include<shared_mutex>
#include "PacketHandler.h"
#include "ClientHandler.h"
#include "MultiTask.h"

class ClientHandler;
//class ServerHandler;

class ChatRoom {
private:
  /* data */

public:
  std::list<ClientHandler*> clients;
  std::shared_mutex client_lock;

  ChatRoom ();
  virtual ~ChatRoom ();

  static void register_client(ChatRoom* chat_room, ClientHandler* client);

  static void send_message(parsed_packet* packet_info, ChatRoom* chat_room, ClientHandler* client_handler, MultiTask* thread_pool);
};
#endif
