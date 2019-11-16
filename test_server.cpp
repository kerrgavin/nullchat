#include <iostream>
#include <string>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <mutex>
#include "MultiTask.h"
#include "ServerHandler.h"


int main(int argc, char const *argv[]) {
  std::cout << "Starting program" << std::endl;
  MultiTask m(5);
  ServerHandler s(8888, 30, &m);
  s.start();
  return 0;
}
