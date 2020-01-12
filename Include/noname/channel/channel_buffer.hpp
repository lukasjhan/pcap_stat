#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "circular_buffer.hpp"

#include <concurrent_queue.h>

namespace noname_core {
	namespace channel {

		template<typename T, std::size_t buffer_size = 0>
		class channel_buffer
		{
		private:
			circular_buffer<T, buffer_size> buffer;
			std::mutex buffer_lock;
			std::condition_variable input_wait;
			std::condition_variable output_wait;
			std::atomic_bool is_closed;

		public:
			channel_buffer() : is_closed(false) { }
			~channel_buffer() = default;
			channel_buffer(const channel_buffer& other)
			{
				buffer = other.buffer;
			}

			channel_buffer operator= (const channel_buffer& other)
			{
				buffer = other.buffer;
			}

			// Wait for the next value
			T get_next()
			{
				std::unique_lock<std::mutex> ulock(buffer_lock);
				if (buffer.empty())
				{
					if (is_closed) return{}; //if closed we always return the default initialisation of T
						
					input_wait.wait(ulock, [&]() {return !buffer.empty() || is_closed; });
					if (buffer.empty() && is_closed) // when we close the channel and there was more waiting then available value 
						return{};
				}

				T temp;
				std::swap(temp, buffer.front());
				buffer.pop();
				output_wait.notify_one();

				return temp;
			}

			//Should use std::optional but MSVC and Clang doesn't support it yet :( #C++17
			std::unique_ptr<T> try_get_next()
			{
				if (is_closed) //if closed we always return the default initialisation of T
					return std::make_unique<T>(T{});
				std::unique_lock<std::mutex> ulock(buffer_lock);
				if (buffer.empty())
				{
					return nullptr;
				}
				std::unique_ptr<T> temp = std::make_unique<T>(buffer.front());
				buffer.pop();
				output_wait.notify_one();
				return std::move(temp);
			}

			void insert(T in)
			{
				if (!is_closed)
				{
					{
						std::unique_lock<std::mutex> lock(buffer_lock);
						if (buffer.full())
						{
							output_wait.wait(lock, [&]() {return !buffer.full() || is_closed; });
							if (is_closed) // if channel was closed end all awaiting inputs (cannot send to a closed channel)
							{
								return;
							}
						}
						buffer.push(in);
					}
					input_wait.notify_one();
				}
			}

			void close()
			{
				is_closed = true;
				input_wait.notify_one();
				output_wait.notify_all();
			}

			bool status()
			{
				return is_closed;
			}

		};
	}
}
