#ifndef CLIENTHANDLER
#define CLIENTHANDLER

#include <memory>
#include <mutex>
#include <shared_mutex>
#include "SocketHandler.h"
#include "PacketHandler.h"
#include "ChatRoom.h"
#include "MultiTask.h"

class SocketHandler;
class ChatRoom;

class ClientHandler {
private:

public:
  std::string ip;
  ChatRoom* chat_room;
  std::shared_mutex socket_lock;
  std::shared_ptr<SocketHandler> socket_handler;
  std::map<unsigned short, std::vector<uint8_t>> outbound_packets;
  unsigned short current_packet_id;

  ClientHandler ();
  virtual ~ClientHandler ();

  unsigned short find_next_packet_id();

  static void remove_socket(std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler);

  static void process_packet(unsigned short packet_num, std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler, MultiTask* thread_pool);

  static void construct_packet(parsed_packet* packet_info, ClientHandler* client_handler, MultiTask* thread_pool);

  static void act(parsed_packet* packet_info, ClientHandler* client_handler, MultiTask* thread_pool);
};

#endif
