#pragma once

#include <queue>
#include "MultiThreadSync.h"

namespace seedio
{
	template<typename T>
	class CSwapQueue
	{
	public:
		CSwapQueue()
		{
			m_multiQueue = &m_queue1;
			m_singleQueue = &m_queue2;
		}

		~CSwapQueue() 
		{
		};

		inline void Push(const T& t)
		{
			m_CriticalSection.Enter();
			m_multiQueue->push(t);
			m_CriticalSection.Leave();
		}

		inline bool Pop(T& t)
		{
			if (m_singleQueue->empty())
				Swap();

			if (m_singleQueue->empty())
				return false;

			t = m_singleQueue->front();
			m_singleQueue->pop();
		}

		inline const std::queue<T>* GetPacketQueue()
		{
			if (m_singleQueue->empty())
				Swap();

			if (m_singleQueue->empty())
				return nullptr;

			return m_singleQueue;
		}

		inline void Swap()
		{
			m_CriticalSection.Enter();

			if (m_multiQueue == &m_queue1)
			{
				m_multiQueue = &m_queue2;
				m_singleQueue = &m_queue1;
			}
			else
			{
				m_multiQueue = &m_queue1;
				m_singleQueue = &m_queue2;
			}

			m_CriticalSection.Leave();
		}

	private:
		std::queue<T> m_queue1;
		std::queue<T> m_queue2;

		// 워커스레드 push 큐
		std::queue<T>* m_multiQueue;

		// 로직스레드 pop 큐
		std::queue<T>* m_singleQueue;

		CCriticalSection m_CriticalSection;
	};
}
