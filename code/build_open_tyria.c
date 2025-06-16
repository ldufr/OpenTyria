#ifndef STRICT
# define STRICT
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_WARNINGS


#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX   0

#ifdef _WIN32
# pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
# pragma warning(disable: 4214) // nonstandard extension used: bit field types other than int
# pragma comment(lib, "Ws2_32.lib")
# include <Windows.h>
# include <Winsock2.h>
# include <Ws2tcpip.h>
# include <bcrypt.h>
# include <winternl.h>
# undef PLATFORM_WINDOWS
# define PLATFORM_WINDOWS 1
#else // _WIN32
# include <unistd.h>
# include <fcntl.h>
# include <pthread.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <errno.h>
# undef PLATFORM_LINUX
# define PLATFORM_LINUX 1
#endif

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mbedtls/bignum.h>
#include <mbedtls/chacha20.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "sqlite3.h"

// arc4 was ported from mbedtls when they removed the implementation.
#include "arc4.h"
#include "arc4.c"

#include "macros.h"
#include "helper.c"
#include "int.h"

#include "endian.h"
#include "GmUuid.h"
#include "array.h"
#include "slice.h"
#include "errors.h"

#include "stream.h"
#include "iocp.h"
#include "sys.h"
#include "logs.h"
#include "random.h"
#include "network.h"

#include "DbSchema.h"

#include "GmDefs.h"
#include "GmErrors.h"
#include "GmIdTable.h"
#include "GmColors.h"
#include "GmMath.h"

#include "GmMaps.h"
#include "GmTitles.h"
#include "GmAgentProperties.h"
#include "GmAttributes.h"

#include "opcodes.h"
#include "msgdefs.h"
#include "GameMsg.h"

#include "GmChar.h"
#include "GmMapData.h"
#include "GmChat.h"
#include "GmFriend.h"
#include "GmMap.h"
#include "GmInventory.h"
#include "GmPlayer.h"
#include "GmAgent.h"
#include "GmItem.h"
#include "GmParty.h"
#include "GmText.h"

#include "GmTitleConstData.h"
#include "GmDefaultArmors.h"

#include "Db.h"

#include "msgpack.h"
#include "proto.h"

#include "GameSrv.h"

#include "AuthMsg.h"
#include "AuthSrv.h"

#if !defined(COMPILE_TESTS)
#include "main.c"
#endif

#include "array.c"
#include "AuthSrv.c"
#include "Db.c"
#include "GameSrv.c"
#include "GmAgent.c"
#include "GmChar.c"
#include "GmChat.c"
#include "GmDefaultArmors.c"
#include "GmInventory.c"
#include "GmItem.c"
#include "GmMapData.c"
#include "GmPlayer.c"
#include "GmParty.c"
#include "GmTitleConstData.c"
#include "GmText.c"
#include "iocp.c"
#include "int.c"
#include "logs.c"
#include "msgdefs.c"
#include "msgpack.c"
#include "network.c"
#include "random.c"
#include "stream.c"

#if PLATFORM_WINDOWS
#include "win32/iocp_win32.c"
#include "win32/sys_win32.c"
#elif PLATFORM_LINUX
#include "linux/iocp_linux.c"
#include "linux/sys_linux.c"
#else
#error "Unknown platform"
#endif

#if defined(COMPILE_TESTS)
#include "tests.c"

#endif
