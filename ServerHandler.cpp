/**
 * file: ServerHandler.cpp
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: Class the handle server functions when used with thread pooling.
 */

#include <stdio.h>
#include <string.h>
#include <memory>
#include <errno.h>
#include <iostream>
#include <deque>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "MultiTask.h"
#include "ServerHandler.h"
#include "SocketHandler.h"
#include "ChatRoom.h"


  ServerHandler::ServerHandler (int port, int max_clients, MultiTask* thread_pool) : port(port), max_clients(max_clients), thread_pool(thread_pool) {
    chat_room = new ChatRoom;
    opt = 1;
    tv.tv_usec = 50;
    tv.tv_sec = 0;
    if( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      thread_pool->print(std::cerr, "Master Socket Creation Failed", "\n");
      exit(EXIT_FAILURE);
    }

    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
      thread_pool->print(std::cerr, "Setting Socket Opt Failed", "\n");
      exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    if ( bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
      thread_pool->print(std::cerr, "Binding of Socket Failed", "\n");
      exit(EXIT_FAILURE);
    }

    thread_pool->print(std::cout, "Listener on port: ", port, "\n");
  }

  ServerHandler::~ServerHandler () {}

  void ServerHandler::start() {
    thread_pool->start();
    if (listen(master_socket, 3) < 0) {
      thread_pool->print(std::cerr, "Listening Failed", "\n");
    }

    addrlen = sizeof(address);
    thread_pool->print(std::cout, "Waiting for connection ...", "\n");

    while(true) {
      FD_ZERO(&readfds);

      FD_SET(master_socket, &readfds);
      max_sd = master_socket;
      {
        std::shared_lock<std::shared_mutex> l(client_lock);
        for(std::shared_ptr<SocketHandler> const& sock: client_sockets){
          sd=sock->socket;

          if(sd > 0) {
            FD_SET( sd, &readfds);

            if(sd > max_sd) {
              max_sd = sd;
            }
          }
        }
      }

      activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

      if ((activity < 0) && (errno!=EINTR)) {
        thread_pool->print(std::cerr, "Select Error ", strerror(errno), "\n");
      }

      if (FD_ISSET(master_socket, &readfds)) {
        thread_pool->queue_work(ServerHandler::create_socket, this, thread_pool);
      }

      for(std::list<std::shared_ptr<SocketHandler>>::iterator socket = client_sockets.begin(); socket != client_sockets.end(); ++socket) {
        sd = (*socket)->socket;

        if ((*socket)->socket==-1 && (*socket)->working <= 0 && (*socket)->checks <= 0) {
          client_sockets.erase(socket);
          break;
        }

        if(FD_ISSET(sd, &readfds)) {
          if((*socket)->working <= 0 && (*socket)->checks <= 0) {
            thread_pool->queue_work(SocketHandler::check_activity, *socket, this, thread_pool);
          }
        }
      }
    }
  }

  void ServerHandler::create_socket(ServerHandler* server_handler, MultiTask* thread_pool) {
    int new_socket;
    if ((new_socket = accept(server_handler->master_socket, (struct sockaddr *)&server_handler->address, (socklen_t*)&server_handler->addrlen)) < 0) {
      thread_pool->print(std::cerr, "New Socket Creation Failed", "\n");
      exit(EXIT_FAILURE);
    }

    thread_pool->print(std::cout, "New connection, socked fd is ", new_socket, " ip is ", inet_ntoa(server_handler->address.sin_addr), " port: ", ntohs(server_handler->address.sin_port), "\n");

    {
      std::lock_guard<std::shared_mutex> l(server_handler->client_lock);
      if(server_handler->client_sockets.size() < server_handler->max_clients) {
        std::string ip(inet_ntoa(server_handler->address.sin_addr));
        server_handler->client_sockets.emplace_back(std::make_shared<SocketHandler>(new_socket, ip));
        ChatRoom::register_client(server_handler->chat_room, server_handler->client_sockets.back()->client);
        thread_pool->queue_work(ChatRoom::register_client, server_handler->chat_room, server_handler->client_sockets.back()->client);
        thread_pool->print(std::cout, "Added new socket", "\n");
      }
      thread_pool->print(std::cout, "Number of sockets: ", server_handler->client_sockets.size(), "\n");
    }
  }
