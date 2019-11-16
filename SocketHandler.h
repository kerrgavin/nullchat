/**
 * file: SocketHandler.h
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: header file for the SocketHandler class.
 */

#ifndef SOCKETHANDLER
#define SOCKETHANDLER

#include<iostream>
#include<shared_mutex>
#include<mutex>
#include<vector>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "SocketHandler.h"
#include "ServerHandler.h"
#include "ClientHandler.h"
#include "MultiTask.h"

#define NETWORK_PACKET_SIZE 522

class ServerHandler;
class ClientHandler;
class SocketHandler {
private:

public:
  std::map<short, std::vector<uint8_t>> buffers;
  std::shared_mutex socket_lock;
  std::shared_mutex work_lock;
  std::shared_mutex close_lock;
  int checks;
  int working;
  int socket;
  ClientHandler* client;

  SocketHandler ();
  SocketHandler (int socket, std::string);
  SocketHandler (const SocketHandler& sock);

  virtual ~SocketHandler ();

  static void check_activity(std::shared_ptr<SocketHandler> socket_handler, ServerHandler* server_handler, MultiTask* thread_pool);

  static void close_socket(std::shared_ptr<SocketHandler> socket_handler, ServerHandler* server_handler, MultiTask* thread_pool);

  static void send_network_packet(unsigned short data_position, unsigned short packet_id, std::shared_ptr<SocketHandler> socket_handler, ClientHandler* client_handler, MultiTask* thread_pool);

  static void read_network_packet(std::vector<uint8_t> network_packet, std::shared_ptr<SocketHandler> socket_handler, MultiTask* thread_pool);
};
#endif
