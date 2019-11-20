

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

#include "PacketHandler.h"
#include "MultiTask.h"
#include "ClientStub.h"

ClientStub::ClientStub(int port, MultiTask* thread_pool) : packet_id(0), port(port), thread_pool(thread_pool) {
  tv.tv_usec = 50;
  tv.tv_sec = 0;
  if( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    thread_pool->print(std::cerr, "Socket Creation Failed", "\n");
    exit(EXIT_FAILURE);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons( port );

  if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0)
    {
        thread_pool->print(std::cerr, "Invalid address Address not supported", "\n");
        exit(EXIT_FAILURE);
    }

  thread_pool->print(std::cout, "Ready to connect on port: ", port, "\n");
}

ClientStub::~ClientStub()  { }

void ClientStub::start() {
  thread_pool->start();
  thread_pool->queue_work(ClientStub::check_input, this, thread_pool);
  if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        thread_pool->print(std::cerr, "Connection Failed", "\n");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(server_address);

    while(true) {
      FD_ZERO(&readfds);
      FD_SET(server_socket, &readfds);

      activity = select(server_socket + 1, &readfds, NULL, NULL, &tv);

      if ((activity < 0) && (errno!=EINTR)) {
        thread_pool->print(std::cerr, "Select Error ", strerror(errno), "\n");
      }

      if(FD_ISSET(server_socket, &readfds)) {
        int read_size;
        std::vector<uint8_t> buffer(NETWORK_PACKET_SIZE);
        if( (read_size = read(server_socket, &buffer[0], NETWORK_PACKET_SIZE)) != 0) {
          thread_pool->queue_work(ClientStub::read_network_packet, buffer, this, thread_pool);
        }
      }
    }
}

void ClientStub::check_input(ClientStub* client, MultiTask* thread_pool) {
  std::string in;
  getline(std::cin, in);
  thread_pool->queue_work(ClientStub::create_control_packet, in, client, thread_pool);
  thread_pool->queue_work(ClientStub::check_input, client, thread_pool);
}

void ClientStub::create_control_packet(std::string user_input, ClientStub* client, MultiTask* thread_pool) {
  unsigned short id = 0xAAAA;
  unsigned long length = 2 + 4 + user_input.size();
  std::vector<uint8_t> packet(length);
  int index = 0;
  index = PacketHandler::insertByte(index, id, &packet);
  index = PacketHandler::insertByte(index, length, &packet);
  index = PacketHandler::insertByte(index, user_input, &packet);

  {
    std::lock_guard<std::shared_mutex> l(client->packet_lock);
    thread_pool->queue_work(ClientStub::send_network_packet, 0, client->packet_id++, packet, client, thread_pool);
  }

}

void ClientStub::send_network_packet(unsigned short data_position, unsigned short packet_id, std::vector<uint8_t> control_packet, ClientStub* client, MultiTask* thread_pool) {
  std::vector<uint8_t> network_packet(NETWORK_PACKET_SIZE);
  int index = 0;

  index = PacketHandler::insertByte(index, packet_id, &network_packet);

  unsigned long size = control_packet.size();
  index = PacketHandler::insertByte(index, size, &network_packet);

  index = PacketHandler::insertByte(index, data_position, &network_packet);

  unsigned short data_length = ( size - data_position < 512) ? size - data_position : 512 ;
  index = PacketHandler::insertByte(index, data_length, &network_packet);

  memcpy(&network_packet[index], &control_packet[data_position], data_length);


  send(client->server_socket, &network_packet[0], NETWORK_PACKET_SIZE, 0);

  data_position += 512;

  if(data_position < size) {
    thread_pool->queue_work(ClientStub::send_network_packet, data_position, packet_id, control_packet, client, thread_pool);
  }
}

void ClientStub::read_network_packet(std::vector<uint8_t> network_packet, ClientStub* client, MultiTask* thread_pool) {
  unsigned short packet_id = PacketHandler::toShort(0, &network_packet);
  unsigned long size = PacketHandler::toLong(2, &network_packet);
  unsigned short position = PacketHandler::toShort(6, &network_packet);
  unsigned short data_length = PacketHandler::toShort(8, &network_packet);


  if ( client->buffers.find(packet_id) == client->buffers.end() ) {
    std::vector<uint8_t> temp;
    client->buffers.insert( { packet_id, temp } );
    client->buffers.at(packet_id).resize(size);
  }

  memcpy(&(client->buffers.at(packet_id))[position], &network_packet[10], data_length);


  if(position + data_length >= size) {
    // queue up the client handler to process the completed control packet
    thread_pool->queue_work(ClientStub::process_packet, packet_id, client, thread_pool);
  }
}

void ClientStub::process_packet(unsigned short packet_id, ClientStub* client, MultiTask* thread_pool) {
  unsigned short id = PacketHandler::toShort(0, &client->buffers.at(packet_id));
  unsigned long packet_size = PacketHandler::toLong(2, &client->buffers.at(packet_id));
  unsigned long packet_info_size = PacketHandler::toShort(6, &client->buffers.at(packet_id));
  std::string packet_info = PacketHandler::toString(8, packet_info_size, &client->buffers.at(packet_id));
  std::string packet_message = PacketHandler::toString(8 + packet_info_size, packet_size - (8 + packet_info_size), &client->buffers.at(packet_id));
  thread_pool->print(std::cout, packet_info, "> ", packet_message, "\n");
  client->buffers.erase(packet_id);
}
