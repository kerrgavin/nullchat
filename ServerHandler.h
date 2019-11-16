/**
 * file: ServerHandler.h
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: header file for the ServerHandler class.
 */

#ifndef SERVERHANDLER
#define SERVERHANDLER

#include <errno.h>
#include <unistd.h>
#include <memory>
#include <mutex>
#include <list>
#include <shared_mutex>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "MultiTask.h"
#include "ChatRoom.h"
#include "SocketHandler.h"

class SocketHandler;
class ChatRoom;

class ServerHandler {
private:
  int opt;
  int port;
  int master_socket , max_clients , activity, valread , sd;
	int max_sd;
  std::list<std::shared_ptr<SocketHandler>> client_sockets;
  fd_set readfds;
  MultiTask* thread_pool;
  //char *message = "ECHO Daemon v1.0 \r\n";
  char buffer[1025];
  struct timeval tv;


public:
  struct sockaddr_in address;
  int addrlen;
  std::shared_mutex client_lock;
  ChatRoom* chat_room;

  ServerHandler (int port, int max_clients, MultiTask* thread_pool);

  virtual ~ServerHandler ();

  void start();

  static void create_socket(ServerHandler* server_handler, MultiTask* thread_pool);

};
#endif
