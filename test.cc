#include <atomic>
#include <utility>
#include <iostream>
#include <gtest/gtest.h>
#include <thread>
#include "circular_buffer.h"

namespace {

class CircularBufferIntTest : public ::testing::Test {
protected:
  std::shared_ptr<CircularBuffer<int>> buffer;
  CircularBufferIntTest() {
    int64_t queue_size = 100;
    buffer = std::make_shared<CircularBuffer<int>>(queue_size);
  }
};

TEST_F(CircularBufferIntTest, EMPTY) {
  CircularBufferConsumer<int> consumer(buffer);
  ASSERT_EQ(consumer.empty(), true);
}

TEST_F(CircularBufferIntTest, PUSH) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  producer.push(10);
  ASSERT_EQ(consumer.empty(), false);
  ASSERT_EQ(consumer.size(), 1);
  ASSERT_EQ(producer.filled(), false);
  ASSERT_EQ(producer.size(), 1);
}

TEST_F(CircularBufferIntTest, FRONT) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  producer.push(10);
  int* front = consumer.front();
  ASSERT_EQ((*front), 10);
}

TEST_F(CircularBufferIntTest, POP) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  producer.push(10);
  ASSERT_EQ(consumer.pop(), 10);
  ASSERT_EQ(consumer.empty(), true);
  ASSERT_EQ(consumer.size(), 0);
  ASSERT_EQ(producer.filled(), false);
  ASSERT_EQ(producer.size(), 0);
}

TEST_F(CircularBufferIntTest, FILL) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  for (int k=0; k<100; k++) {
    producer.push(k);
  }
  ASSERT_EQ(consumer.empty(), false);
  ASSERT_EQ(consumer.size(), 100);
  ASSERT_EQ(producer.filled(), true);
  ASSERT_EQ(producer.size(), 100);

  ASSERT_EQ(consumer.pop(), 0);
  ASSERT_EQ(consumer.empty(), false);
  ASSERT_EQ(consumer.size(), 99);
  ASSERT_EQ(producer.filled(), false);
  ASSERT_EQ(producer.size(), 99);
}

TEST_F(CircularBufferIntTest, CYCLE) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  for (int k=0; k<100000; k++) {
    producer.push(k);
    ASSERT_EQ(consumer.empty(), false);
    ASSERT_EQ(consumer.size(), 1);
    ASSERT_EQ(producer.filled(), false);
    ASSERT_EQ(producer.size(), 1);
    consumer.pop();
    ASSERT_EQ(consumer.empty(), true);
    ASSERT_EQ(consumer.size(), 0);
    ASSERT_EQ(producer.filled(), false);
    ASSERT_EQ(producer.size(), 0);
  }
}

TEST_F(CircularBufferIntTest, CYCLE_2) {
  CircularBufferProducer<int> producer(buffer);
  CircularBufferConsumer<int> consumer(buffer);
  for (int k=0; k<99; k++) {
    producer.push(k);
  }
  for (int k=0; k<100000; k++) {
    producer.push(k);
    ASSERT_EQ(consumer.empty(), false);
    ASSERT_EQ(consumer.size(), 100);
    ASSERT_EQ(producer.filled(), true);
    ASSERT_EQ(producer.size(), 100);
    consumer.pop();
    ASSERT_EQ(consumer.empty(), false);
    ASSERT_EQ(consumer.size(), 99);
    ASSERT_EQ(producer.filled(), false);
    ASSERT_EQ(producer.size(), 99);
  }
}

TEST_F(CircularBufferIntTest, MULTITHREAD) {
  std::thread t([&]() {
    CircularBufferProducer<int> producer(buffer);
    for (int k=0; k<100; k++) {
      producer.push(k);
    }
  });
  CircularBufferConsumer<int> consumer(buffer);
  for (int k=0; k<100; k++) {
    while (consumer.empty());
    ASSERT_EQ(consumer.pop(), k);
  }
  t.join();
}

TEST_F(CircularBufferIntTest, MULTITHREAD_2) {
  std::thread t([&]() {
    CircularBufferProducer<int> producer(buffer);
    for (int k=0; k<100000; k++) {
      while (producer.filled());
      producer.push(k);
    }
  });
  CircularBufferConsumer<int> consumer(buffer);
  for (int k=0; k<100000; k++) {
    while (consumer.empty());
    ASSERT_EQ(consumer.pop(), k);
  }
  t.join();
  ASSERT_EQ(consumer.empty(), true);
  ASSERT_EQ(consumer.size(), 0);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}