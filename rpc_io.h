
#pragma once

#include "rpc_error.h"


class CRpcIo
{
 protected:
  void Dump( const char *msg, const void *buffer, unsigned int size );
 public:
  virtual ~CRpcIo() {}
  virtual void Write(const void *buffer, unsigned int size) = 0;
  virtual void Flush() = 0;
  virtual void Clear() = 0;
  virtual void Read(void *buffer, unsigned int size) = 0;
  virtual void Close() = 0;
};

// usb.h implements CRpcIo as CUSB

// Null ?

class CRpcIoNull : public CRpcIo
{
 public:
  void Write(const void *buffer, unsigned int size) { throw CRpcError(CRpcError::WRITE_ERROR); }
  void Flush() {}
  void Clear() {}
  void Read( void *buffer, unsigned int size ) { throw CRpcError(CRpcError::READ_ERROR); } // real Read is in usb.h
  void Close() {}

};
