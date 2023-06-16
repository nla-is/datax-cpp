//
// Created by Giuseppe Coviello on 9/21/15.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <list>

namespace datax::sdk::implementation {
class QueueClosedException : public std::exception {
 public:
  virtual ~QueueClosedException() = default;
  const char *what() const noexcept override {
    return "Queue closed";
  }
};

template<typename T>
class Queue {
 public:
  Queue() : capacity_(10) {
  }

  explicit Queue(unsigned max_size) : capacity_(max_size) {
  }

  void push(const T &element) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_)
      throw QueueClosedException();
    while (data_.size() == capacity_) full_.wait(lock);
    data_.push_back(element);
    empty_.notify_one();
  }

  T front() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_ && data_.empty())
      throw QueueClosedException();
    while (data_.empty()) empty_.wait(lock);
    auto element = data_.front();
    return element;
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_ && data_.empty())
      throw QueueClosedException();
    while (data_.empty()) empty_.wait(lock);
    auto element = data_.front();
    data_.erase(data_.begin());
    full_.notify_one();
    return element;
  }

  bool tryPush(const T &element, uint64_t timeout = 1) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_)
      throw QueueClosedException();
    auto success = full_.wait_for(lock, std::chrono::milliseconds(timeout), [this]() {
      return data_.size() < capacity_;
    });
    if (success) {
      data_.push_back(element);
      empty_.notify_one();
    }
    return success;
  }

  bool tryPop(T *element, uint64_t timeout = 1) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_ && data_.empty())
      throw QueueClosedException();
    auto success = empty_.wait_for(lock, std::chrono::milliseconds(timeout), [this]() {
      return !data_.empty();
    });
    if (success) {
      *element = std::move(data_.front());
      data_.erase(data_.begin());
      full_.notify_one();
    }
    return success;
  }

  bool empty() {
    std::unique_lock<std::mutex> lock(mutex_);
    return data_.empty();
  }

  void drain(T *last) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_ && data_.empty())
      throw QueueClosedException();
    while (data_.empty()) empty_.wait(lock);
    *last = data_.back();
    data_.clear();
  }

  T drain() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_ && data_.empty())
      throw QueueClosedException();
    while (data_.empty()) empty_.wait(lock);
    auto last = data_.back();
    data_.clear();
    return last;
  }

  uint32_t size() const {
    return data_.size();
  }

  void close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
  }

  bool closed() {
    std::unique_lock<std::mutex> lock(mutex_);
    return data_.empty() && closed_;
  }

  uint32_t capacity() const {
    return capacity_;
  }

 private:
  unsigned capacity_;

  std::list<T> data_;
  std::mutex mutex_;
  std::condition_variable full_;
  std::condition_variable empty_;

  bool closed_ = false;
};
}
