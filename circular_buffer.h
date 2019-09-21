#ifndef INC_JABA_CIRCULAR_BUFFER_H_
#define INC_JABA_CIRCULAR_BUFFER_H_

#include <cassert>
#include <cstdint>      // uintXX_t
#include <cstring>      // memcpy

namespace jaba {

  // Circular buffer allocated dinamically: create & destroy
  // push & pop chunks of bytes. 
  class CircularBuffer {

    char* buffer = nullptr;
    size_t allocated_size = 0;

  protected:

    size_t head = 0;          // First byte to be used next time some pushes
    size_t tail = 0;          // First byte to be read
    size_t free_space = 0;    // Remaining free space

  public:

    void create(size_t new_size) {
      assert(allocated_size == 0);
      assert(buffer == nullptr);
      buffer = new char[new_size];
      allocated_size = new_size;
      free_space = allocated_size;
      head = tail = 0;
    }

    void destroy() {
      delete[] buffer;
      allocated_size = free_space = 0;
      clear();
    }

    struct CheckPoint {
      size_t head;
      size_t tail;
      size_t free_space;
    };

    CheckPoint saveCheckPoint() const {
      return CheckPoint{ head, tail, free_space };
    }

    void loadCheckPoint(CheckPoint check_point) {
      tail = check_point.tail;
      head = check_point.head;
      free_space = check_point.free_space;
    }

    void clear() {
      head = tail = 0;
    }

    bool isValid() const {
      return buffer && allocated_size > 0;
    }

    bool empty() const {
      return free_space == allocated_size;
    }

    size_t capacity() const {
      return allocated_size;
    }

    size_t bytesUsed() const {
      return allocated_size - free_space;
    }

    size_t bytesFree() const {
      return free_space;
    }

    bool push(const void* data, size_t data_size) {

      if (data_size > free_space)
        return false;

      assert(data);
      assert(data_size > 0);
      assert(data_size < allocated_size);
      free_space -= data_size;

      size_t bytes_to_wrap = allocated_size - head;
      if (bytes_to_wrap >= data_size) {
        memcpy(buffer + head, data, data_size);
        head += data_size;
      }
      else {
        // Need to split the data in two pieces
        memcpy(buffer + head, data, bytes_to_wrap);
        head = data_size - bytes_to_wrap;
        memcpy(buffer, (char*)data + bytes_to_wrap, head);
      }
      return true;
    }

    bool pop(void* data, size_t data_size) {
      size_t used_space = bytesUsed();
      if (used_space < data_size)
        return false;

      size_t bytes_to_wrap = allocated_size - tail;
      if (bytes_to_wrap >= data_size) {
        memcpy(data, buffer + tail, data_size);
        tail += data_size;
      }
      else {
        // Need to split the data in two pieces
        memcpy(data, buffer + tail, bytes_to_wrap);
        tail = data_size - bytes_to_wrap;
        memcpy((char*)data + bytes_to_wrap, buffer, tail);
      }

      free_space += data_size;
      return true;
    }

  };

  // -----------------------------------------------------------------
  // Each msg is prefixed with a marker indicating the size of the msg
  // Depending on the max size of the msgs you expect to store in 
  // the circular buffer, you can reduce the size of this marker
  template< typename msg_separatr_size_t = uint16_t>
  class CircularBufferMsgsT : protected CircularBuffer {

    size_t nmsgs = 0;

  public:

    typedef msg_separatr_size_t msg_size_t;

    // Expose some members from the base class
    void create(size_t new_size) {
      CircularBuffer::create(new_size);
    }
    void destroy() {
      CircularBuffer::destroy();
    }

    size_t empty() const { return CircularBuffer::empty(); }
    size_t bytesUsed() const { return CircularBuffer::bytesUsed(); }
    size_t numMsgs() const { return nmsgs; }

    bool pushMsg(const void* data, size_t data_size) {
      if (data_size + sizeof(msg_size_t) > free_space)
        return false;
      msg_size_t msg_size = (msg_size_t)data_size;
      bool is_ok = push(&msg_size, sizeof(msg_size_t));
      assert(is_ok);
      is_ok &= push(data, data_size);
      assert(is_ok);
      ++nmsgs;
      return is_ok;
    }

    size_t popMsg(void* data, size_t max_data_size) {

      size_t used_space = bytesUsed();
      if (used_space < sizeof(msg_size_t))
        return false;

      auto check_point = saveCheckPoint();
      msg_size_t msg_size;
      bool is_ok = pop(&msg_size, sizeof(msg_size_t));
      assert(is_ok);

      // If the msg does not fit in the buffer given by the user... roll back
      if (msg_size > max_data_size) {
        loadCheckPoint(check_point);
        return false;
      }

      is_ok = pop(data, msg_size);
      assert(is_ok);

      --nmsgs;
      assert(nmsgs > 0 || empty());

      // Just to try to reduce the number of wraps
      if (!nmsgs)
        clear();

      return (size_t)msg_size;
    }

  };

  // Each msg can have max length 16 bits
  using CircularBufferMsgs = CircularBufferMsgsT<uint16_t>;

}

#endif
