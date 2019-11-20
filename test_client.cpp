#include <iostream>
#include <string>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <mutex>
#include "MultiTask.h"
#include "ClientStub.h"

int main(int argc, char const *argv[]) {
  std::cout << "Starting program" << std::endl;
  MultiTask m(2);
  ClientStub c(8888, &m);
  c.start();
  return 0;
}
