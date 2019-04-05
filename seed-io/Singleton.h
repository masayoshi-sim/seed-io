#pragma once

#include <cstdio>
#include <mutex>
#include <memory>
#include <iostream>

using namespace std;

namespace seedio
{
	template<class T>
	class CSingleton
	{
	public:
		CSingleton() {};
		CSingleton(const CSingleton &) = delete;
		CSingleton<T> &operator=(const CSingleton &) = delete;

		static T& GetInstance()
		{
			//call_once(m_OnceFlag, []()
			call_once(CSingleton<T>::m_OnceFlag, []()
			{
				m_Instance.reset(new T);
			});

			return *(m_Instance.get());
		}

		static void Release()
		{
			if (m_Instance)
				m_Instance.release();
			/*
			call_once(CSingleton<T>::m_OnceFlag, []()
			{
				if (m_Instance && m_Instance.get())
				{
					m_Instance.reset();
				}
			});
			 */
		}

	protected:
		static unique_ptr<T> m_Instance;
		static once_flag m_OnceFlag;
	};

	template<class T> unique_ptr<T> CSingleton<T>::m_Instance = nullptr;
	template<class T> once_flag CSingleton<T>::m_OnceFlag;

#define GetInst(t) CSingleton<t>::GetInstance()
}