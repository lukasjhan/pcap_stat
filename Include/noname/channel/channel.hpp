#pragma once
#include <queue>
#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <limits>
#include <iterator>

#include "channel_buffer.hpp"
#include "circular_buffer.hpp"

namespace noname_core {
	namespace channel {

		template<typename T, std::size_t buffer_size> class channel;
		template<typename T, std::size_t buffer_size> class ichannel;
		template<typename T, std::size_t buffer_size> class ochannel;

		template<typename T, std::size_t buffer_size = 1>
		class ichannel
		{
		protected:
			std::shared_ptr<channel_buffer<T, buffer_size>> buffer;
			ichannel(std::shared_ptr<channel_buffer<T, buffer_size>> _buffer) : buffer(_buffer) {}
		public:
			ichannel() = default;
			ichannel(const ichannel<T, buffer_size>& ch) = default;
			ichannel(ichannel<T, buffer_size>&& ch) { std::swap(buffer, ch.buffer); }

			void close()
			{
				buffer->close();
			}

			std::shared_ptr<channel_buffer<T, buffer_size>> get_buffer() const { return buffer; }

			friend ichannel<T, buffer_size>& operator<< (ichannel<T, buffer_size>& ch, T& obj)
			{
				ch.buffer->insert(obj);
				return ch;
			}

			friend ichannel<T, buffer_size>& operator<< (ichannel<T, buffer_size>& ch, const T& obj)
			{
				ch.buffer->insert(obj);
				return ch;
			}

			friend std::istream& operator>> (std::istream& is, ichannel<T, buffer_size>& ch)
			{
				T temp;
				is >> temp;
				ch << temp;
				return is;
			}
		};

		template <typename T, std::size_t buffer_size = 0>
		class ochannel_iterator : public std::iterator<std::input_iterator_tag, T>
		{
			std::shared_ptr<channel_buffer<T, buffer_size>> buffer;
			T val;
		public:
			ochannel_iterator(std::shared_ptr<channel_buffer<T, buffer_size>> _buffer, bool is_end = false) : buffer(_buffer)
			{
				if (!is_end) operator++();
			}

			ochannel_iterator(const ochannel_iterator&) = default;

			T& operator*()
			{
				return val;
			}
			ochannel_iterator& operator++()
			{
				val = buffer->get_next();
				return *this;
			}
			ochannel_iterator operator++(int)
			{
				ochannel_iterator tmp(*this);
				operator++();
				return tmp;
			}
			inline bool operator==(const ochannel_iterator& rhs) const { return buffer->status(); }
			inline bool operator!=(const ochannel_iterator& rhs) const { return !operator==(rhs); }
		};

		template <typename T, std::size_t buffer_size = 0>
		class ochannel
		{
		protected:
			std::shared_ptr<channel_buffer<T, buffer_size>> buffer;
			ochannel(std::shared_ptr<channel_buffer<T, buffer_size>> _buffer) : buffer(_buffer) {}
		public:
			ochannel() = default;
			ochannel(const ochannel<T, buffer_size>& ch) = default;
			ochannel(ochannel<T, buffer_size>&& ch) { swap(buffer, ch.buffer); };

			std::shared_ptr<channel_buffer<T, buffer_size>> get_buffer() const { return buffer; }

			friend ochannel<T, buffer_size>& operator>> (ochannel<T, buffer_size>& ch, T& obj)
			{
				obj = ch.buffer->get_next();
				return ch;
			}

			friend std::ostream& operator<< (std::ostream& os, ochannel<T, buffer_size>& ch)
			{
				T temp;
				ch >> temp;
				os << temp;
				return os;
			}

			using ochannel_end_iterator = ochannel_iterator<T, buffer_size>;

			ochannel_iterator<T, buffer_size> begin()
			{
				return ochannel_iterator<T, buffer_size>{ buffer };
			}
			ochannel_end_iterator end()
			{
				return { buffer , true };
			}
		};

		template<typename T, std::size_t buffer_size = 1>
		class channel : public ichannel<T, buffer_size>, public ochannel<T, buffer_size>
		{
			friend class channel::ichannel;
			friend class channel::ochannel;

		public:
			channel()
			{
				channel::ichannel::buffer = channel::ochannel::buffer = std::make_shared<channel_buffer<T, buffer_size>>();
			}

			~channel() = default;

			channel(const channel& other)
			{
				channel::ichannel::buffer = channel::ochannel::buffer = other.get_buffer();
			}

			channel operator= (const channel& other)
			{
				channel::ichannel::buffer = channel::ochannel::buffer = other.get_buffer();
			}

			void close()
			{
				channel::ichannel::close();
			}

			std::shared_ptr<channel_buffer<T, buffer_size>> get_buffer() const
			{
				return channel::ichannel::buffer;
			}
			
			friend ichannel<T, buffer_size>& operator<< (channel<T, buffer_size>& ch, T& obj)
			{
				return static_cast<ichannel<T, buffer_size>&>(ch) << obj;
			}

			friend ichannel<T, buffer_size>& operator<< (channel<T, buffer_size>& ch, const T& obj)
			{
				return static_cast<ichannel<T, buffer_size>&>(ch) << obj;
			}

			friend ochannel<T, buffer_size>& operator>> (channel<T, buffer_size>& ch, T& obj)
			{
				return static_cast<ochannel<T, buffer_size>&>(ch) >> obj;
			}

			template <std::size_t obuffer_size, std::size_t ibuffer_size>
			friend ochannel<T, buffer_size>& operator>> (ochannel<T, obuffer_size>& out, ichannel<T, ibuffer_size>& in)
			{
				T temp;
				out >> temp;
				in << temp;
				return out;
			}

			friend std::ostream& operator<< (std::ostream& os, channel<T, buffer_size>& ch)
			{
				return os << static_cast<ochannel<T, buffer_size>&>(ch);
			}
			friend std::istream& operator>> (std::istream& is, channel<T, buffer_size>& ch)
			{
				return is >> static_cast<ichannel<T, buffer_size>&>(ch);
			}
		};

		template <typename T, std::size_t buffer_size>
		void channel_close(channel<T, buffer_size> ch)
		{
			ch.close();
		}

		/*
		TODO: fix copy problem.

		template <typename T, std::size_t buffer_size = 1>
		channel<T, buffer_size>&& make_channel()
		{
			return channel<T, buffer_size>();
		}
		*/
	}
}