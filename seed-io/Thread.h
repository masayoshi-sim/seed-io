#pragma once

#include "MultiThreadSync.h"

namespace seedio
{
	class CThread
	{
	public:
		CThread(void);
		virtual ~CThread(void);

		bool BeginThread();
		void EndThread(unsigned int uiRetVal) const;
#ifdef _WIN32
		HANDLE GetThreadHandler() const;
		void Wait() const;
		bool Wait(DWORD dwMuilliseconds) const;
		//void SetThreadName(DWORD dwThreadID, const char* szThreadName);
#else
		pthread_t GetThreadHandler() const;
#endif	
		virtual unsigned int Run() = 0;

	private:
#ifdef _WIN32
		/*
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType;
			LPCSTR szName;
			DWORD dwThreadID;
			DWORD dwFlags;
		}THREADNAME_INFO;
		*/
#endif

	protected:
#ifdef _WIN32
		HANDLE m_hThread;
		static unsigned int __stdcall ThreadFunction(void* pParam);
#else
		pthread_t m_hThread;
		static void* ThreadFunction(void* pParam);
#endif
	};
}
