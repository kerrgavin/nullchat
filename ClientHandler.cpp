
#include <climits>
#include <memory>
#include "SocketHandler.h"
#include "ClientHandler.h"
#include "PacketHandler.h"
#include "ChatRoom.h"
#include "MultiTask.h"

  ClientHandler::ClientHandler () : current_packet_id(0) {}
  ClientHandler::~ClientHandler () {}

  // void process_packet(std::vector<uint8_t> packet, ClientHandler* client_handler, MultiTask* thread_pool) {
  //   parsed_packet* packet_info = new parsed_packet;
  //   parse_packet(packet, packet_info);
  //   packet_info->ip = client_handler->ip;
  //   thread_pool->queue_work(ClientHandler::act, packet_info, client_handler, thread_pool);
  // }

  unsigned short ClientHandler::find_next_packet_id() {
    unsigned short next_packet_id = current_packet_id + 1;
    while(true) {
      if(outbound_packets.find(next_packet_id) == outbound_packets.end()) {
        return next_packet_id;
      }
      if(next_packet_id == current_packet_id) {
        return current_packet_id;
      }
      next_packet_id++;
    }
  }

  void ClientHandler::remove_socket(std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler) {
    std::lock_guard<std::shared_mutex> l(client_handler->socket_lock);
    client_handler->socket_handler.reset();
  }

  void ClientHandler::process_packet(unsigned short packet_num, std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler, MultiTask* thread_pool) {
    thread_pool->print(std::cout, "Processing packet: ", packet_num, "\n");
    parsed_packet* packet_info = new parsed_packet;
    PacketHandler::parse_packet(&socket_handler->buffers.at(packet_num), packet_info);
    thread_pool->print(std::cout, "Processing complete\n");
    socket_handler->buffers.erase(packet_num);
    packet_info->ip = client_handler->ip;
    thread_pool->print(std::cout, "Packet Struct:\n",
                        "IP: ", packet_info->ip,
                        "\nID: ", packet_info->id,
                        "\nLength: ", packet_info->length,
                        "\nMessage: ", packet_info->message, "\n");
    thread_pool->queue_work(ClientHandler::act, packet_info, client_handler, thread_pool);
  }

  void ClientHandler::construct_packet(parsed_packet* packet_info, ClientHandler* client_handler, MultiTask* thread_pool) {
    thread_pool->print(std::cout, "Constructing packet\n");
    std::vector<uint8_t> temp;
    unsigned short packet_id = client_handler->find_next_packet_id();
    thread_pool->print(std::cout, "Outbound packet id: ", packet_id, "\n");
    if(packet_id != client_handler->current_packet_id) {
      client_handler->outbound_packets.insert( {packet_id, temp} );
      client_handler->current_packet_id = packet_id;
      thread_pool->print(std::cout, "Generating outbound packet\n");
      PacketHandler::gen_packet(packet_info, &client_handler->outbound_packets.at(packet_id));
      thread_pool->print(std::cout, "Queueing socket handler to send network packet\n");
      thread_pool->queue_work(SocketHandler::send_network_packet, 0, packet_id, client_handler->socket_handler, client_handler, thread_pool);
    } else {
      thread_pool->queue_work(ClientHandler::construct_packet, packet_info, client_handler, thread_pool);
    }

  }

  void ClientHandler::act(parsed_packet* packet_info, ClientHandler* client_handler, MultiTask* thread_pool) {
    thread_pool->print(std::cout, "Acting on packet info\n");
    thread_pool->queue_work(ChatRoom::send_message, packet_info, client_handler->chat_room, client_handler, thread_pool);
  }
