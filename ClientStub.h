#ifndef CLIENTSTUB
#define CLIENTSTUB

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

#include "MultiTask.h"

#define NETWORK_PACKET_SIZE 522

class ClientStub {
private:
  int port;
  int server_socket , max_clients , activity, valread;
  fd_set readfds;
  MultiTask* thread_pool;
  //char *message = "ECHO Daemon v1.0 \r\n";
  char buffer[1025];
  struct timeval tv;
  unsigned short packet_id;
  std::shared_mutex packet_lock;

public:
  std::map<short, std::vector<uint8_t>> buffers;
  struct sockaddr_in server_address;
  int addrlen;
  std::shared_mutex client_lock;

  ClientStub (int port, MultiTask* thread_pool);
  virtual ~ClientStub ();

  void start();

  static void check_input(ClientStub* client, MultiTask* thread_pool);
  static void create_control_packet(std::string user_input, ClientStub* client, MultiTask* thread_pool);
  static void send_network_packet(unsigned short data_position, unsigned short packet_id, std::vector<uint8_t> control_packet, ClientStub* client, MultiTask* thread_pool);
  static void read_network_packet(std::vector<uint8_t> network_packet, ClientStub* client, MultiTask* thread_pool);
  static void process_packet(unsigned short packet_id, ClientStub* client, MultiTask* thread_pool);
};
#endif
