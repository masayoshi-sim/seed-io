#include "stdafx.h"
#include "Thread.h"
#include <iostream>

namespace seedio
{
	CThread::CThread(void)
	{
#ifdef _WIN32
		m_hThread = INVALID_HANDLE_VALUE;
#else
		m_hThread = 0;
#endif
	}

	CThread::~CThread(void)
	{
#ifdef _WIN32
		if (m_hThread != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(m_hThread);
			m_hThread = INVALID_HANDLE_VALUE;
		}
#else
		m_hThread = 0;
#endif
	}

#ifdef _WIN32 // ������ ������ �Լ�
	// �ڵ� ���
	HANDLE CThread::GetThreadHandler() const
	{
		return m_hThread;
	}

	// ���� ����
	void CThread::Wait() const
	{
		Wait(INFINITE);
	}

	// �Ͻ� ����
	bool CThread::Wait(DWORD dwMuilliseconds) const
	{
		DWORD dwResult = ::WaitForSingleObject(m_hThread, dwMuilliseconds);

		if (dwResult == WAIT_TIMEOUT || dwResult == WAIT_ABANDONED)
			return false;
		return true;
	}

	// ������ �̸� ���̱�
	/*
	void CThread::SetThreadName(DWORD dwThreadID, const char* szThreadName)
	{
		THREADNAME_INFO info;
		memset(&info, 0, sizeof(info));
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
	}
	*/

	// ������ �Լ�
	unsigned int __stdcall CThread::ThreadFunction(void* pParam)
	{
		unsigned int uiRet = 0;
		CThread* pThread = (CThread*)pParam;

		if (pThread)
		{
			uiRet = pThread->Run();
		}
		return uiRet;
	}
#else // ������ ������ �Լ�
	// �ڵ� ���
	pthread_t CThread::GetThreadHandler() const
	{
		return m_hThread;
	}

	void* CThread::ThreadFunction(void* pParam)
	{
		CThread* pThread = (CThread*)pParam;
		pThread->Run();
		return NULL;
	}
#endif

	// ������ ����
	bool CThread::BeginThread()
	{
#ifdef _WIN32
		if (m_hThread != INVALID_HANDLE_VALUE)
			return false;

		unsigned int uiThreadID = 0;

		m_hThread = (HANDLE)::_beginthreadex(NULL, 0, ThreadFunction, (void*)this, 0, &uiThreadID);
		if (m_hThread == INVALID_HANDLE_VALUE)
			return false;

		//printf("%s threadid : %d\n", __FUNCDNAME__, uiThreadID);
		return true;
#else
		if (pthread_create(&m_hThread, NULL, ThreadFunction, (void*)this) == -1)
			return false;
		return true;
#endif
	}

	void CThread::EndThread(unsigned int uiRetVal) const
	{
#ifdef _WIN32
		::_endthreadex(uiRetVal);
#else
		pthread_exit(NULL);
#endif
	}
}
