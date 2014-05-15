
#pragma once

#include "windows.h" // /usr/i686-pc-mingw32/sys-root/mingw/include/windows.h
#include "rpc.h"

class CPipeServer : public CRpcIo
{
  HANDLE hPipe;
 public:
 CPipeServer() : hPipe(INVALID_HANDLE_VALUE) {}
  ~CPipeServer() {}
  bool Create(const char *name);
  void WaitConnection();
  void Close();
  void Read( void *buffer, unsigned int size ) // rpc_io.Read
  {
    std::cout << "pipe.h ReadFile for size " << size << std::endl;
    DWORD n;
    if( !ReadFile( hPipe, buffer, size, &n, NULL ) ) { // WinAPI ?
      // /usr/i686-pc-mingw32/sys-root/mingw/include/winbase.h
      std::cout << "pipe.h ReadFile error for size " << size
		<< " but got " << n << std::endl;
      throw int(1);
    }
  }
  void Flush() {}
  void Clear() {}
  void Write(const void *buffer, unsigned int size)
  {
    DWORD n;
    if (!WriteFile(hPipe, buffer, size, &n, NULL)) throw int(2);
  }
};


class CPipeClient : public CRpcIo
{
  HANDLE hPipe;
 public:
 CPipeClient() : hPipe(INVALID_HANDLE_VALUE) {}
  ~CPipeClient() {}
  bool Open(const char *name);
  void Close();
  void Read(void *buffer, unsigned int size)
  {
    DWORD n;
    if( !ReadFile( hPipe, buffer, size, &n, NULL ) ) { // WinAPI ?
      std::cout << "pipe.h ReadFile error for size " << size
		<< " but got " << n << std::endl;
      throw int(1);
    }
  }
  void Flush() {}
  void Clear() {}
  void Write(const void *buffer, unsigned int size)
  { 
    DWORD n;
    if (!WriteFile(hPipe, buffer, size, &n, NULL)) throw int(2);
  }
};

