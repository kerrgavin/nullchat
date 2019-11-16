/**
 * file: SocketHandler.cpp
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: Wrapper class to handle the management of sockets when used with
                thread pooling.
 */

#include<iostream>
#include<shared_mutex>
#include<mutex>
#include<map>
#include <memory>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "SocketHandler.h"
#include "ServerHandler.h"
#include "PacketHandler.h"
#include "MultiTask.h"

  SocketHandler::SocketHandler ()  : socket(0), working(0), checks(0) {
    client = new ClientHandler;
    client->ip = "";
  }
  SocketHandler::SocketHandler (int socket, std::string ip) : socket(socket), working(0), checks(0) {
    client = new ClientHandler;
    client->ip = ip;
  }
  SocketHandler::SocketHandler (const SocketHandler& sock) {
    socket = sock.socket;
    working = sock.working;
    client = sock.client;
  }

  SocketHandler::~SocketHandler () {}

  //static function that is queued to allow the socket to determine how to handle incoming data, whether to read or close
  void SocketHandler::check_activity(std::shared_ptr<SocketHandler> socket_handler, ServerHandler* server_handler, MultiTask* thread_pool) {
    //add one to the checks counter for the SocketHandler object
    socket_handler->checks++;
    //lock down the work_lock so that there is not conflict when deciding if socket should queue work
    {
      std::lock_guard<std::shared_mutex> l(socket_handler->work_lock);
      //if not already working then queue up work
      if(socket_handler->working <= 0) {
        int read_size;
        //add one to the working counter for the SocketHandler object
        socket_handler->working++;
        std::vector<uint8_t> buffer(NETWORK_PACKET_SIZE);
        if((read_size = read(socket_handler->socket, &buffer[0], NETWORK_PACKET_SIZE)) == 0) {
          thread_pool->queue_work(close_socket, socket_handler, server_handler, thread_pool);
        } else {
          long size = (buffer.at(2) << 24) | (buffer.at(3) << 16) | (buffer.at(4) << 8) | (buffer.at(4));
          buffer.resize(size);
          thread_pool->queue_work(SocketHandler::read_network_packet, buffer, socket_handler, thread_pool);
          //thread_pool->queue_work(SocketHandler::read_data, size - 6, buffer, socket_handler, thread_pool);
        }
      }
    }
    //subtract one from the checks counter
    socket_handler->checks--;
  }

  void SocketHandler::read_network_packet(std::vector<uint8_t> network_packet, std::shared_ptr<SocketHandler> socket_handler, MultiTask* thread_pool) {
    unsigned short packet_id = PacketHandler::toShort(0, &network_packet);
    unsigned long size = PacketHandler::toLong(2, &network_packet);
    unsigned short position = PacketHandler::toShort(6, &network_packet);
    unsigned short data_length = PacketHandler::toShort(8, &network_packet);

    if ( socket_handler->buffers.find(packet_id) != socket_handler->buffers.end() ) {
      std::vector<uint8_t> temp;
      socket_handler->buffers.insert( { packet_id, temp } );
      socket_handler->buffers.at(packet_id).resize(size);
    }

    memcpy(&(socket_handler->buffers.at(packet_id))[position], &network_packet[10], data_length);

    if(position + data_length >= size) {
      // queue up the client handler to process the completed control packet
    }
  }

  // void SocketHandler::send_data(int send_size, std::vector<uint8_t>* packet, SocketHandler* socket_handler, MultiTask* thread_pool) {
  //   int send_val = 0;
  //   send_val = (socket_handler->socket, &packet[packet.size() - send_size], (send_size < 512) ? send_size : 512 );
  //   if(read_size <= 0){}
  // }

  void SocketHandler::send_network_packet(unsigned short data_position, unsigned short packet_id, std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler, MultiTask* thread_pool) {
    std::vector<uint8_t> network_packet(NETWORK_PACKET_SIZE);
    int index = 0;

    index = PacketHandler::insertByte(index, packet_id, &network_packet);

    unsigned long size = client_handler->outbound_packets.at(packet_id).size();
    index = PacketHandler::insertByte(index, size, &network_packet);

    index = PacketHandler::insertByte(index, data_position, &network_packet);

    unsigned short data_length = ( size - data_position < 512) ? size - data_position : 512 ;
    index = PacketHandler::insertByte(index, data_length, &network_packet);

    memcpy(&network_packet[index], &(client_handler->outbound_packets.at(packet_id))[data_position], data_length);

    send(socket_handler->socket, &network_packet[0], NETWORK_PACKET_SIZE, 0);

    data_position += 512;

    if(data_position < size) {
      thread_pool->queue_work(SocketHandler::send_network_packet, data_position, packet_id, socket_handler, client_handler, thread_pool);
    } else {
      client_handler->outbound_packets.erase(packet_id);
    }

  }

  //static function that is queued to close the socket possessed by the SocketHandler object
  void SocketHandler::close_socket(std::shared_ptr<SocketHandler> socket_handler, ServerHandler* server_handler, MultiTask* thread_pool) {
    std::lock_guard<std::shared_mutex> loc(server_handler->client_lock);
    std::lock_guard<std::shared_mutex> l(socket_handler->close_lock);
    sockaddr_in sa = {0};
    getpeername(socket_handler->socket, (struct sockaddr*)&sa, (socklen_t*)&server_handler->addrlen);
    thread_pool->print(std::cout, "Host disconnected, ip ", inet_ntoa(sa.sin_addr), " port ", ntohs(sa.sin_port), "\n");
    close(socket_handler->socket);
    socket_handler->socket = -1;
    std::lock_guard<std::shared_mutex> lo(socket_handler->work_lock);
    socket_handler->working--;
  }

  // void SocketHandler::read_data(int read_size, std::vector<uint8_t> buffer, SocketHandler* socket_handler, MultiTask* thread_pool) {
  //   int read_val = 0;
  //   read_val = read(socket_handler->socket, &buffer[buffer.size() - read_size], (read_size < 512) ? read_size : 512 );
  //   read_size -= read_val;
  //   if (read_size <= 0) {
  //     //call the proper ClientHandler function here
  //     thread_pool->queue_work(ClientHandler::act, buffer, &socket_handler->client_handler, thread_pool);
  //     std::lock_guard<std::shared_mutex> l(socket_handler->work_lock);
  //     socket_handler->working--;
  //   } else {
  //     thread_pool->queue_work(SocketHandler::read_data, read_size, buffer, socket_handler, thread_pool);
  //   }
  // }
