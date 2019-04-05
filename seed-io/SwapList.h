#pragma once

#include <list>
#include "MultiThreadSync.h"

namespace seedio
{
	template<typename T>
	class CSwapList
	{
	public:
		CSwapList()
		{
			m_multiList = &m_list1;
			m_singleList = &m_list2;
		};

		~CSwapList() 
		{
		};

		inline void Push(const T& t)
		{
			m_CriticalSection.Enter();
			//m_multiList->push_back(t);
			m_multiList->emplace_back(t);
			m_CriticalSection.Leave();
		}

		inline bool Pop(T& t)
		{
			if (m_singleList->empty())
				Swap();

			if (m_singleList->empty())
				return false;

			t = *m_singleList->begin();
			m_singleList->pop_front();
			return true;
		}

		inline void Swap()
		{
			m_CriticalSection.Enter();

			if (m_multiList == &m_list1)
			{
				m_multiList = &m_list2;
				m_singleList = &m_list1;
			}
			else
			{
				m_multiList = &m_list1;
				m_singleList = &m_list2;
			}

			m_CriticalSection.Leave();
		}

		inline std::list<T>* GetSingleList()
		{
			if (m_singleList->empty())
				Swap();

			return m_singleList;
		}

	private:
		std::list<T> m_list1;
		std::list<T> m_list2;

		// 워커스레드 push 리스트
		std::list<T>* m_multiList;

		// 로직스레드 pop 리스트
		std::list<T>* m_singleList;

		CCriticalSection m_CriticalSection;
	};
}
