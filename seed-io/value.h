#pragma once

#ifdef _WIN32
#include <windows.h>
#include <WinDef.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <string>
#include <sys/types.h>
#include <WS2tcpip.h>
#include <wchar.h>

/*
#include "Typedef.h"
#include "Define.h"
#include "PktHeader.h"
*/

using namespace std;
//-------------------------------------------------------------------//
//문자열 변환
inline void StrConvA2T(CHAR *src, TCHAR *dest, size_t destLen) 
{
#ifdef  UNICODE                     // r_winnt
	if (destLen < 1) {
		return;
	}
	MultiByteToWideChar(CP_ACP, 0, src, -1, dest, (int)destLen - 1);
#endif
}

inline void StrConvT2A(TCHAR *src, CHAR *dest, size_t destLen) 
{
#ifdef  UNICODE                     // r_winnt
	if (destLen < 1) {
		return;
	}
	WideCharToMultiByte(CP_ACP, 0, src, -1, dest, (int)destLen, NULL, FALSE);
#endif
}

inline void StrConvA2W(CHAR *src, WCHAR *dest, size_t destLen) 
{
	if (destLen < 1) {
		return;
	}
	MultiByteToWideChar(CP_ACP, 0, src, -1, dest, (int)destLen - 1);
}
inline void StrConvW2A(WCHAR *src, CHAR *dest, size_t destLen) 
{
	if (destLen < 1) {
		return;
	}
	WideCharToMultiByte(CP_ACP, 0, src, -1, dest, (int)destLen, NULL, FALSE);
}
//-------------------------------------------------------------------//
// delete object
#undef	SAFE_DELETE
#define SAFE_DELETE(obj)						\
{												\
	if ((obj)) delete(obj);		    			\
    (obj) = nullptr;									\
}
// delete object array
#undef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(arr)					\
{												\
	if ((arr)) delete [] (arr);		    		\
    (arr) = nullptr;									\
}
