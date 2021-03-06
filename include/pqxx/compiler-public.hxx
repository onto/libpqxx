/** Compiler deficiency workarounds for libpqxx clients.
 *
 * Copyright (c) 2002-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_COMPILER_PUBLIC
#define PQXX_H_COMPILER_PUBLIC

#ifdef _MSC_VER

/* Work around a particularly pernicious and deliberate bug in Visual C++:
 * min() and max() are defined as macros, which can have some very nasty
 * consequences.  This compiler bug can be switched off by defining NOMINMAX.
 *
 * We don't like making choices for the user and defining environmental macros
 * of our own accord, but in this case it's the only way to compile without
 * incurring a significant risk of bugs--and there doesn't appear to be any
 * downside.  One wonders why this compiler wart is being maintained at all,
 * since the introduction of inline functions back in the 20th century.
 */
#if defined(min) || defined(max)
#error "Oops: min() and/or max() are defined as preprocessor macros.\
  Define NOMINMAX macro before including any system headers!"
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Suppress vtables on abstract classes.
#define PQXX_NOVTABLE __declspec(novtable)

#endif	// _MSC_VER


// Workarounds & definitions that need to be included even in library's headers
#include "pqxx/config-public-compiler.h"

// Workarounds for SUN Workshop 6
#if defined(__SUNPRO_CC)
#define PQXX_PRIVATE __hidden
#endif	// __SUNPRO_CC


#if defined(__GNUC__) && defined(PQXX_HAVE_GCC_CONST)
#define PQXX_CONST __attribute__ ((const))
#else
#define PQXX_CONST
#endif

#if defined(PQXX_HAVE_DEPRECATED)
#define PQXX_DEPRECATED [[deprecated]]
#elif defined(__GNUC__) && defined(PQXX_HAVE_GCC_DEPRECATED)
#define PQXX_DEPRECATED __attribute__ ((deprecated))
#else
#define PQXX_DEPRECATED
#endif

#if defined(__GNUC__) && defined(PQXX_HAVE_GCC_PURE)
#define PQXX_PURE __attribute__ ((pure))
#else
#define PQXX_PURE
#endif


// Workarounds for Windows
#ifdef _WIN32

/* For now, export DLL symbols if _DLL is defined.  This is done automatically
 * by the compiler when linking to the dynamic version of the runtime library,
 * according to "gzh"
 */
// TODO: Define custom macro to govern how libpqxx will be linked to client
#if !defined(PQXX_LIBEXPORT) && defined(PQXX_SHARED)
#define PQXX_LIBEXPORT __declspec(dllimport)
#endif	// !PQXX_LIBEXPORT && PQXX_SHARED


// Workarounds for Microsoft Visual C++
#ifdef _MSC_VER

// Automatically link with the appropriate libpq (static or dynamic, debug or
// release).  The default is to use the release DLL.  Define PQXX_PQ_STATIC to
// link to a static version of libpq, and _DEBUG to link to a debug version.
// The two may be combined.
#if defined(PQXX_AUTOLINK)
#if defined(PQXX_PQ_STATIC)
#ifdef _DEBUG
#pragma comment(lib, "libpqd")
#else
#pragma comment(lib, "libpq")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "libpqddll")
#else
#pragma comment(lib, "libpqdll")
#endif
#endif
#endif

// If we're not compiling libpqxx itself, automatically link with the correct
// libpqxx library.  To link with the libpqxx DLL, define PQXX_SHARED; the
// default is to link with the static library.  This is also the recommended
// practice.
// Note that the preprocessor macro PQXX_INTERNAL is used to detect whether we
// are compiling the libpqxx library itself. When you compile the library
// yourself using your own project file, make sure to include this define.
#if defined(PQXX_AUTOLINK) && !defined(PQXX_INTERNAL)
  #ifdef PQXX_SHARED
    #ifdef _DEBUG
      #pragma comment(lib, "libpqxxD")
    #else
      #pragma comment(lib, "libpqxx")
    #endif
  #else // !PQXX_SHARED
    #ifdef _DEBUG
      #pragma comment(lib, "libpqxx_staticD")
    #else
      #pragma comment(lib, "libpqxx_static")
    #endif
  #endif
#endif

#endif	// _MSC_VER
#endif	// _WIN32


#ifndef PQXX_LIBEXPORT
#define PQXX_LIBEXPORT
#endif

#ifndef PQXX_PRIVATE
#define PQXX_PRIVATE
#endif

#ifndef PQXX_NOVTABLE
#define PQXX_NOVTABLE
#endif

#endif
