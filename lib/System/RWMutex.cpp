//===- RWMutex.cpp - Reader/Writer Mutual Exclusion Lock --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the llvm::sys::RWMutex class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Config/config.h"
#include "llvm/System/RWMutex.h"

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only TRULY operating system
//===          independent code.
//===----------------------------------------------------------------------===//

#if !defined(ENABLE_THREADS) || ENABLE_THREADS == 0
// Define all methods as no-ops if threading is explicitly disabled
namespace llvm {
using namespace sys;
RWMutex::RWMutex( bool recursive) { }
RWMutex::~RWMutex() { }
bool RWMutex::reader_acquire() { return true; }
bool RWMutex::reader_release() { return true; }
bool RWMutex::writer_acquire() { return true; }
bool RWMutex::writer_release() { return true; }
}
#else

#if defined(HAVE_PTHREAD_H) && defined(HAVE_PTHREAD_RWLOCK_INIT)

#include <cassert>
#include <pthread.h>
#include <stdlib.h>

namespace llvm {
using namespace sys;


// This variable is useful for situations where the pthread library has been
// compiled with weak linkage for its interface symbols. This allows the
// threading support to be turned off by simply not linking against -lpthread.
// In that situation, the value of pthread_mutex_init will be 0 and
// consequently pthread_enabled will be false. In such situations, all the
// pthread operations become no-ops and the functions all return false. If
// pthread_rwlock_init does have an address, then rwlock support is enabled.
// Note: all LLVM tools will link against -lpthread if its available since it
//       is configured into the LIBS variable.
// Note: this line of code generates a warning if pthread_rwlock_init is not
//       declared with weak linkage. It's safe to ignore the warning.
static const bool pthread_enabled = true;

// Construct a RWMutex using pthread calls
RWMutex::RWMutex()
  : data_(0)
{
  if (pthread_enabled)
  {
    // Declare the pthread_rwlock data structures
    pthread_rwlock_t* rwlock =
      static_cast<pthread_rwlock_t*>(malloc(sizeof(pthread_rwlock_t)));
    pthread_rwlockattr_t attr;

    // Initialize the rwlock attributes
    int errorcode = pthread_rwlockattr_init(&attr);
    assert(errorcode == 0);

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__)
    // Make it a process local rwlock
    errorcode = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
#endif

    // Initialize the rwlock
    errorcode = pthread_rwlock_init(rwlock, &attr);
    assert(errorcode == 0);

    // Destroy the attributes
    errorcode = pthread_rwlockattr_destroy(&attr);
    assert(errorcode == 0);

    // Assign the data member
    data_ = rwlock;
  }
}

// Destruct a RWMutex
RWMutex::~RWMutex()
{
  if (pthread_enabled)
  {
    pthread_rwlock_t* rwlock = static_cast<pthread_rwlock_t*>(data_);
    assert(rwlock != 0);
    pthread_rwlock_destroy(rwlock);
    free(rwlock);
  }
}

bool
RWMutex::reader_acquire()
{
  if (pthread_enabled)
  {
    pthread_rwlock_t* rwlock = static_cast<pthread_rwlock_t*>(data_);
    assert(rwlock != 0);

    int errorcode = pthread_rwlock_rdlock(rwlock);
    return errorcode == 0;
  }
  return false;
}

bool
RWMutex::reader_release()
{
  if (pthread_enabled)
  {
    pthread_rwlock_t* rwlock = static_cast<pthread_rwlock_t*>(data_);
    assert(rwlock != 0);

    int errorcode = pthread_rwlock_unlock(rwlock);
    return errorcode == 0;
  }
  return false;
}

bool
RWMutex::writer_acquire()
{
  if (pthread_enabled)
  {
    pthread_rwlock_t* rwlock = static_cast<pthread_rwlock_t*>(data_);
    assert(rwlock != 0);

    int errorcode = pthread_rwlock_wrlock(rwlock);
    return errorcode == 0;
  }
  return false;
}

bool
RWMutex::writer_release()
{
  if (pthread_enabled)
  {
    pthread_rwlock_t* rwlock = static_cast<pthread_rwlock_t*>(data_);
    assert(rwlock != 0);

    int errorcode = pthread_rwlock_unlock(rwlock);
    return errorcode == 0;
  }
  return false;
}

}

#elif defined(LLVM_ON_UNIX)
#include "Unix/RWMutex.inc"
#elif defined( LLVM_ON_WIN32)
#include "Win32/RWMutex.inc"
#else
#warning Neither LLVM_ON_UNIX nor LLVM_ON_WIN32 was set in System/Mutex.cpp
#endif
#endif

