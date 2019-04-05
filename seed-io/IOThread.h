#pragma once

#include "Thread.h"

namespace seedio
{
	class CIOThread :
		public CThread
	{
	public:
		CIOThread();
		virtual ~CIOThread();
		virtual unsigned int Run();
	};
}