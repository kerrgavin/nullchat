
#include <mutex>
#include <shared_mutex>
#include "ChatRoom.h"
#include "PacketHandler.h"
#include "ClientHandler.h"
#include "MultiTask.h"

  ChatRoom::ChatRoom (){}
  ChatRoom::~ChatRoom (){}

  void ChatRoom::register_client(ChatRoom* chat_room, ClientHandler* client) {
    std::lock_guard<std::shared_mutex> l(chat_room->client_lock);
    chat_room->clients.push_back(client);
    client->chat_room = chat_room;
  }

  void ChatRoom::send_message(parsed_packet* packet_info, ChatRoom* chat_room, ClientHandler* client_handler, MultiTask* thread_pool) {
    std::shared_lock<std::shared_mutex> l(chat_room->client_lock);
    for(ClientHandler* client: chat_room->clients){
      if(client != client_handler) {
        thread_pool->queue_work(ClientHandler::construct_packet, packet_info, client, thread_pool);
      }
    }
  }
