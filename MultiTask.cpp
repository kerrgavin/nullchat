/**
 * file: MultiTask.cpp
 * author: Gavin Kerr
 * contact: gvnkerr97@aol.com
 * description: Code used to manage thread pools for asynchronus coding.
 */

#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "MultiTask.h"

  void MultiTask::worker(std::deque<std::function<void()>>* work_queue, int num) {
    //the thread is locked until all threads are created
    std::shared_lock<std::shared_mutex> start_lock(init_lock);

    //continuous loop to process work
    while(!finished) {
      std::shared_lock<std::shared_mutex> shared_flush_lock(flush_lock);
      //empty task to hold work to be run
      std::function<void()> work;
      int queue_length = 0;

      //check self owned queue for work
      if(!finished){
        std::lock_guard<std::shared_mutex> self_queue_lock(*(queue_lock_map.at(work_queue)));
        queue_length = work_queue->size();
        //if there is work then select it for processing
        if (queue_length > 0) {
          work = work_queue->front();
          work_queue->pop_front();
        }
      }

      //if self owned queue is empty, then check the other queues and steal work
      if(queue_length <= 0 && !finished) {
        for(int i = 0; i < worker_count; i++) {
          if (&(work_queues.at(i)) != work_queue && !finished) {
            //lock down queue in question
            std::lock_guard<std::shared_mutex> other_queue_lock(*(queue_lock_map.at(&(work_queues.at(i)))));
            //if there is available work in the queue then steal it
            if(work_queues.at(i).size() > 0 && !finished) {
              work = work_queues.at(i).front();
              work_queues.at(i).pop_front();
              break;
            }
          }
        }
      }

      //process the current work
      if(!finished && work) {
        work();
      }
      //if the finished flag is raise, end the thread
      if(finished) {
        return;
      }
    }
  }

  //sets the finish flag to true
  void MultiTask::finish() {
    finished = true;
  }

  //flushes the contents of each of the queues
  void MultiTask::flush_queues() {
    std::lock_guard<std::shared_mutex> lock(flush_lock);
    for(int i = 0; i < worker_count; i++) {
      work_queues.at(i).clear();
    }
  }

  MultiTask::MultiTask (int count): worker_count(count) {
    finished = false;
  }

  MultiTask::~MultiTask () {
    worker_map.clear();
    queue_lock_map.clear();
    worker_threads.clear();
    work_queues.clear();
  }

  void MultiTask::start() {
    //lock the worker threads from beginning work until the initialization process has finished
    std::lock_guard<std::shared_mutex> lock(init_lock);
    //set up the queues to hold onto waiting work
    for(int i = 0; i < worker_count; i++) {
      work_queues.push_back(std::deque<std::function<void()>>(0));
    }

    //begin threads that will process work from queues
    for(int i = 0; i < worker_count; i++) {
      worker_threads.push_back(
        std::async(std::launch::async, [this, i]{ worker(&(work_queues.at(i)), i); })
      );
    }
    //create collection of mutex locks to protect reading/writing of worker queues

    queue_locks.resize(worker_count);

    //map the threads to their respective queues
    for(int i = 0; i < worker_count; i++) {
      worker_map.insert(
        {
          &(worker_threads.at(i)),
          &(work_queues.at(i))
        }
      );
    }
    //map queues to their respective locks
    for(int i = 0; i < worker_count; i++) {
      queue_lock_map.insert(
        {
          &(work_queues.at(i)),
          &(queue_locks.at(i))
        }
      );
    }
    std::cout << "Starting Work" << std::endl;
  }

  void MultiTask::shutdown() {
    finish();
    flush_queues();
  }
