#pragma once
#include "Defines.h"
#include "Types.h"
#include "SpinLock.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>

namespace LambdaEngine
{
	class LAMBDA_API Thread
	{
		friend class EngineLoop;

	public:
		~Thread();

		void Wait();
		void Notify();

	private:
		Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

		void Run();

	public:
		static void Sleep(int32 milliseconds);
		static Thread* Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

	private:
		static void Join();

	private:
		std::thread m_Thread;
		std::function<void()> m_Func;
		std::function<void()> m_FuncOnFinished;
		std::condition_variable m_Condition;
		std::mutex m_Mutex;
		std::atomic_bool m_ShouldYeild;

	private:
		static SpinLock s_Lock;
		static std::vector<Thread*> s_ThreadsToJoin;
	};
}
