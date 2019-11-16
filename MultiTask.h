/**
 * file: MultiTask.h
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: header file for the MultiTask class.
 */

#ifndef MULTITASK
#define MULTITASK

#include <deque>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

class MultiTask {
private:
  std::shared_mutex init_lock;
  std::shared_mutex flush_lock;
  std::mutex print_lock;
  std::vector<std::future<void>> worker_threads;
  std::vector<std::deque<std::function<void()>>> work_queues;
  std::deque<std::shared_mutex> queue_locks;
  std::map<std::future<void>*, std::deque<std::function<void()>>*> worker_map;
  std::map<std::deque<std::function<void()>>*, std::shared_mutex*> queue_lock_map;
  int worker_count;
  bool finished;

  void worker(std::deque<std::function<void()>>* work_queue, int num);

  //sets the finish flag to true
  void finish();
  //flushes the contents of each of the queues
  void flush_queues();

public:
  MultiTask (int count);

  virtual ~MultiTask ();

  void start();

  void shutdown();

  template <typename... Args>
  void print(std::ostream& out, Args&&... args)
  {
      std::lock_guard<std::mutex> l(print_lock);
      ((out << std::forward<Args>(args)), ...);
  }

  template <class Fn, class... Args>
  void queue_work(Fn&& function, Args&&... args){
    //bind task to argument to create function
    std::function<void()> task(std::bind(function, args ...));
    int min = -1;
    //create pointer to queue with smallest number of work
    std::deque<std::function<void()>>* min_queue;
    //determine which queue current task should be placed into
    for(int i = 0; i < worker_count; i++) {
      std::shared_lock<std::shared_mutex> queue_lock(*(queue_lock_map.at(&(work_queues.at(i)))));
      if(work_queues.at(i).size() < min || min == -1) {
        min_queue = &(work_queues.at(i));
        min = work_queues.at(i).size();
      }
    }
    //add the task to the selected queue
    {
      std::lock_guard<std::shared_mutex> lock(*(queue_lock_map.at(min_queue)));
      min_queue->push_back(task);
    }
  }
};
#endif
