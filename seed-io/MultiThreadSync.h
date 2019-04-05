#pragma once

#include "CriticalSectionSync.h"

#define UNLOCK 0
#define LOCK 1

namespace seedio
{
	template<typename T>
	class CMultiThreadSync
	{
		friend class CThreadSync;

	public:
		class CThreadSync
		{
		public:
			explicit CThreadSync()
			{
				this->Lock();
				//this->m_bySync = LOCK;
			}

			virtual ~CThreadSync()
			{
				/*
				if (m_bySync)
				{
					this->UnLock();
					this->m_bySync = UNLOCK;
				}
				*/
				this->UnLock();
			}

			inline void Lock()
			{
				T::m_CriticalSection.Enter();
			}

			inline void UnLock()
			{
				T::m_CriticalSection.Leave();
			}
	
		};

	protected:
		virtual ~CMultiThreadSync(void) {};

	private:
		static CCriticalSection m_CriticalSection;
		//static unsigned char m_bySync;
	};

	template<typename T>
	CCriticalSection CMultiThreadSync<T>::m_CriticalSection;
};
