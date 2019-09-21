# circular_buffer

Circular buffer in simple c++ to push/pop chunk of bytes, or directly pod msgs.

# Example

```
  const char* msg1 = "john"; 
  size_t len1 = strlen(msg1) + 1;

  const char* msg2 = "Peter & Eva"; 
  size_t len2 = strlen(msg2) + 1;

  // Push two messages
  CircularBufferMsgs cmsgs;
  cmsgs.create(64);
  cmsgs.pushMsg(msg1, len1);
  cmsgs.pushMsg(msg2, len2);
  assert(cmsgs.numMsgs() == 2);
  
  // Pop the messages, each popMsg returns the size of the stored message
  char buf[80] = {0}; 
  size_t szbuf = sizeof(buf);
  size_t sz_msg1 = cmsgs.popMsg(buf, szbuf);

  assert(sz_msg1 == len1);
  assert(strcmp(msg1, buf) == 0);
  assert(cmsgs.numMsgs() == 1);

  memset(buf, 0x00, szbuf);
  size_t sz_msg2 = cmsgs.popMsg(buf, szbuf);
  assert(sz_msg2 == len2);
  assert(strcmp(msg2, buf) == 0);
  assert(cmsgs.empty() == 0);

  cmsgs.destroy();

```