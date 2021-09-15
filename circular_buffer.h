#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <vector>
#include <atomic>
#include <utility>
#include <memory>

template <typename T>
class CircularBuffer;

/**
 * CircularBufferProducer can push() on CircularBuffer.
 * Note: this is NOT thread-safe.
 * before push, user have to check if queue is filled, by fill()
 */ 
template <typename T>
class CircularBufferProducer {
public:
  explicit CircularBufferProducer(std::shared_ptr<CircularBuffer<T>> buffer) {
    while (buffer->has_producer.exchange(true, std::memory_order_acquire));
    buf = buffer;
  }
  ~CircularBufferProducer() {
    buf->has_producer.store(false);
  }
  bool fill() {
    return size()+1 == buf->size;
  }
  int64_t size() {
    int64_t head, tail;
    head = buf->head.load();
    tail = buf->tail.load();
    if (tail >= head) {
      return tail-head;
    }
    return tail+buf->size-head;
  }
  void push(T item) {
    int64_t tail;
    tail = buf->tail.load();
    buf->data[tail] = std::move(item);
    buf->tail.store((tail+1)%buf->size);
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
    while (buffer->has_consumer.exchange(true, std::memory_order_acquire));
    buf = buffer;
  }
  ~CircularBufferConsumer() {
    buf->has_consumer.store(false);
  }
  bool empty() {
    return size() == 0;
  }
  int64_t size() {
    int64_t head, tail;
    head = buf->head.load();
    tail = buf->tail.load();
    if (tail >= head) {
      return tail-head;
    }
    return tail+buf->size-head;
  }
  T* front() {
    int64_t head, tail;
    head = buf->head.load();
    tail = buf->tail.load();
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
    buf->head.store((head+1)%buf->size);
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
    CircularBuffer(int64_t queue_size) :
      size(queue_size+1),
      data(queue_size+1),
      head(0),
      tail(0),
      has_producer(false),
      has_consumer(false) {}
  private:
    const int64_t size;
    std::vector<T> data;
    std::atomic<int64_t> head, tail;
    std::atomic_bool has_producer, has_consumer;
};

#endif