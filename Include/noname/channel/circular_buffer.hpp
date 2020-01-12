#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>

namespace noname_core {
	namespace channel {

		template<typename T, std::size_t buffer_size>
		class circular_buffer
		{
			typedef T& reference;
			typedef const T& const_reference;
			typedef T* pointer;
			typedef const T* const_pointer;
			typedef decltype(buffer_size) size_type;

			std::shared_ptr<T[]> buffer;
			pointer _front;
			pointer _end;
			size_t _size;

			void increment(pointer& p) const 
			{
				if (p + 1 == &buffer[0] + buffer_size)
					p = &buffer[0];
				else
					++p;
			}
		public:
			circular_buffer()
				: buffer(new T[buffer_size])
				, _front(&buffer[0])
				, _end(&buffer[0])
				, _size(0) { }

			~circular_buffer() 
			{
				_front = nullptr;
				_end = nullptr;
			}

			circular_buffer(const circular_buffer& other)
			{
				buffer = other.buffer;
				_front = other._front;
				_end = other._end;
				_size = other._size;
			}

			circular_buffer operator= (const circular_buffer& other)
			{
				buffer = other.buffer;
				_front = other._front;
				_end = other._end;
				_size = other._size;
			}

			bool empty() const
			{
				return _size <= 0;
			}

			bool full() const
			{
				return _size == buffer_size;
			}

			T& front()
			{
				if (empty())
					throw std::out_of_range("container is empty");
				return *_front;
			}

			const T& front() const
			{
				if (empty())
					throw std::out_of_range("container is empty");
				return *_front;
			}

			T& back()
			{
				if (empty())
					throw std::out_of_range("container is empty");
				return *_end;
			}

			const T& back() const
			{
				if (empty())
					throw std::out_of_range("container is empty");
				return *_end;
			}

			size_type size() const
			{
				return _size;
			}

			void push_back(T item)
			{
				if (full())
					throw std::out_of_range("container is full");
				*_end = item;
				increment(_end);
				++_size;
			}

			template<typename ... Args>
			void emplace_back(Args&& ... args)
			{
				push_back(T(std::forward<Args>(args)...));
			}

			void pop()
			{
				if (empty())
					throw std::out_of_range("container is empty");
				--_size;
				increment(_front);
			}

			T pop_front()
			{
				if (empty())
					throw std::out_of_range("container is empty");
				--_size;
				T temp = *_front;
				increment(_front);
				return temp;
			}

			void push(T item)
			{
				push_back(item);
			}
		};
	}
}
