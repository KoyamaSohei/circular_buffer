/*
MIT License

Copyright (c) 2021 KoyamaSohei

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

template <typename T>
class CircularBuffer;

/**
 * CircularBufferProducer can push() on CircularBuffer.
 * Note: this is NOT thread-safe.
 * before push, user have to check if queue is filled, by filled()
 */
template <typename T>
class CircularBufferProducer {
public:
  explicit CircularBufferProducer(std::shared_ptr<CircularBuffer<T>> buffer) {
    while (buffer->has_producer.exchange(true, std::memory_order_acquire))
      ;
    buf = buffer;
  }
  ~CircularBufferProducer() {
    buf->has_producer.store(false, std::memory_order_release);
  }
  bool filled() { return size() + 1 == buf->size; }
  int64_t size() {
    int64_t head, tail;
    head = buf->head.load(std::memory_order_acquire);
    tail = buf->tail.load();
    if (tail >= head) { return tail - head; }
    return tail + buf->size - head;
  }
  void push(T item) {
    int64_t tail;
    tail = buf->tail.load();
    buf->data[tail] = std::move(item);
    buf->tail.store((tail + 1) % buf->size, std::memory_order_release);
  }

private:
  std::shared_ptr<CircularBuffer<T>> buf;
};

/**
 * CircularBufferConsumer can pop(), and front() on CircularBuffer.
 * Note: this is NOT thread-safe.
 * before pop, user have to check if queue is empty, by empty()
 * if queue is empty, front() returns NULL.
 */
template <typename T>
class CircularBufferConsumer {
public:
  explicit CircularBufferConsumer(std::shared_ptr<CircularBuffer<T>> buffer) {
    while (buffer->has_consumer.exchange(true, std::memory_order_acquire))
      ;
    buf = buffer;
  }
  ~CircularBufferConsumer() {
    buf->has_consumer.store(false, std::memory_order_release);
  }
  bool empty() { return size() == 0; }
  int64_t size() {
    int64_t head, tail;
    head = buf->head.load();
    tail = buf->tail.load(std::memory_order_acquire);
    if (tail >= head) { return tail - head; }
    return tail + buf->size - head;
  }
  T* front() {
    int64_t head, tail;
    head = buf->head.load();
    tail = buf->tail.load(std::memory_order_acquire);
    if (head == tail) {
      // empty
      return NULL;
    }
    return &buf->data[head];
  }
  T pop() {
    int64_t head;
    head = buf->head.load();
    T ret = std::move(buf->data[head]);
    buf->head.store((head + 1) % buf->size, std::memory_order_release);
    return ret;
  }

private:
  std::shared_ptr<CircularBuffer<T>> buf;
};

/**
 * Lock-free Circular Buffer for single-consumer and single-producer.
 */
template <typename T>
class CircularBuffer {
public:
  friend CircularBufferConsumer<T>;
  friend CircularBufferProducer<T>;
  CircularBuffer(int64_t queue_size)
    : size(queue_size + 1)
    , data(queue_size + 1)
    , head(0)
    , tail(0)
    , has_producer(false)
    , has_consumer(false) {}

private:
  const int64_t size;
  std::vector<T> data;
  std::atomic<int64_t> head, tail;
  std::atomic_bool has_producer, has_consumer;
};

#endif