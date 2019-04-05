#pragma once

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace seedio
{
	class CCriticalSection
	{
	public:
		CCriticalSection()
		{
#ifdef _WIN32
			InitializeCriticalSection(&m_Sync);
#else
			pthread_mutex_init(&m_Sync, NULL);
#endif
		}

		virtual ~CCriticalSection()
		{
#ifdef _WIN32
			DeleteCriticalSection(&m_Sync);
#else
			pthread_mutex_destroy(&m_Sync);
#endif
		}

		inline void Enter()
		{
#ifdef _WIN32
			EnterCriticalSection(&m_Sync);
#else
			pthread_mutex_lock(&m_Sync);
#endif
		}

		inline void Leave()
		{
#ifdef _WIN32
			LeaveCriticalSection(&m_Sync);
#else
			pthread_mutex_unlock(&m_Sync);
#endif
		}

	private:
#ifdef _WIN32
		CRITICAL_SECTION m_Sync;
#else
		pthread_mutex_t m_Sync;
#endif
	};
};
