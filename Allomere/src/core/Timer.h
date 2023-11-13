#pragma once
#include "core/Log.h"

#include <chrono>
#include <ostream>

namespace Allomere
{

	struct TimeSlice
	{

		std::chrono::duration<long long, std::nano> Time{};

		inline long long nanoseconds() const
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Time).count();
		}
		inline long long microseconds() const
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(Time).count();
		}
		inline long long milliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(Time).count();
		}
		inline long long seconds() const
		{
			return std::chrono::duration_cast<std::chrono::seconds>(Time).count();
		}
		inline int minutes() const
		{
			return std::chrono::duration_cast<std::chrono::minutes>(Time).count();
		}
		inline int hours() const
		{
			return std::chrono::duration_cast<std::chrono::hours>(Time).count();
		}

		inline std::string toString() const
		{
			std::stringstream ss;
			ss << milliseconds() << " milliseconds"
			   << " | " << microseconds() << " microseconds";
			return ss.str();
		}

		inline void toString(std::ostream &os) const
		{
			os << milliseconds() << " milliseconds"
			   << " | " << microseconds() << " microseconds";
		}

		TimeSlice operator+(const TimeSlice &e) const
		{
			return {Time + e.Time};
		}

		TimeSlice &operator+=(const TimeSlice &e)
		{
			Time += e.Time;
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_integral<T>::value || std::is_floating_point<T>::value, int> = 0>
		TimeSlice operator/(const T val) const
		{
			return {Time / val};
		}

		friend std::ostream &operator<<(std::ostream &os, const TimeSlice &e)
		{
			// Default is milliseconds
			e.toString(os);
			return os;
		}
	};

	namespace Timer
	{

		// only works with void functions
		template <typename T, typename... Args>
		TimeSlice Duration(T func, Args &&...args)
		{
			auto t1 = std::chrono::high_resolution_clock::now();

			func(std::forward<Args>(args)...);

			auto t2 = std::chrono::high_resolution_clock::now();

			TimeSlice slice{t2 - t1};

			return slice;
		}

		template <typename T>
		TimeSlice Duration(T func)
		{
			auto t1 = std::chrono::high_resolution_clock::now();

			func();

			auto t2 = std::chrono::high_resolution_clock::now();

			TimeSlice slice{t2 - t1};

			return slice;
		}

		class Stopwatch
		{
		public:
			Stopwatch() = default;

			TimeSlice Lap()
			{
				auto now = std::chrono::high_resolution_clock::now();
				TimeSlice slice{std::chrono::duration_cast<std::chrono::nanoseconds>(now - mLastLap)};
				mLastLap = now;
				mLaps.push_back(slice);

				return slice;
			}

			TimeSlice Start()
			{
				mStartTime = std::chrono::high_resolution_clock::now();
				mLastLap = mStartTime;
				mStarted = true;
				return TimeSlice{};
			}

			TimeSlice FromStart()
			{
				return TimeSlice{ std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - mStartTime) };
			}

			TimeSlice Stop()
			{
				mStarted = false;
				return FromStart();
			}

			bool isStarted() const { return mStarted; }

			const std::vector<TimeSlice>& laps() const { return mLaps; }

		private:
			std::chrono::steady_clock::time_point mStartTime{std::chrono::high_resolution_clock::now()};
			std::chrono::steady_clock::time_point mLastLap{mStartTime};
			std::vector<TimeSlice> mLaps;
			bool mStarted{ false };
		};
	}
}
