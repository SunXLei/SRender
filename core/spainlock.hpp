#pragma once
#include <atomic>

class SpinLock {
public:
	SpinLock() : flag_(false)
	{}

	void lock()
	{
		bool expect = false;
		
		//检查flag和expect的值，若相等则返回true，且把flag置为true， 
		//若不相等则返回false，且把expect置为true(所以每次循环一定要将expect复原)
		while (!flag_.compare_exchange_weak(expect, true))
		{
			//这里一定要将expect复原; 执行失败时expect结果是未定的
			expect = false;
		}
	}

	void unlock()
	{
		flag_.store(false);
	}

private:
	std::atomic<bool> flag_;
};