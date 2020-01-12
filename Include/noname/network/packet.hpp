#pragma once

#include "network.hpp"
#include "types.hpp"
#include "ethernet.hpp"
#include "arp.hpp"
#include "ipv4.hpp"
#include "tcp.hpp"

namespace noname_core {
	
	namespace {
		template <typename T, typename ... Types>
		inline auto get_size(const T& a, Types ... args)
		{
			auto size = sizeof a + (sizeof args + ...);
			return size;
		}

		template <typename T>
		inline void process_raw_data(std::vector<uint8_t>& raw_data, T data, int& index)
		{
			std::copy(
				reinterpret_cast<uint8_t*>(&data),
				reinterpret_cast<uint8_t*>(&data) + sizeof data,
				raw_data.data() + index);
			index += sizeof data;
		}
	}
	template <typename T, typename ... Types>
	class packet {
	public:
		packet(T header, Types ... headers)
			: data(header, headers ...)
		{
			int index = 0;
			auto size = get_size(header, headers ...);
			raw_data.reserve(size);

			process_raw_data(raw_data, header, index),
				(process_raw_data(raw_data, headers, index), ...);
		}

		packet(const packet& p) = default;

		inline std::vector<uint8_t>& get_raw_data() const
		{
			return raw_data;
		}

		inline std::tuple<T, Types ...>& get_data() const
		{
			return data;
		}

	protected:

	private:
		std::vector<uint8_t> raw_data;
		std::tuple<T, Types ...> data;
		enum { length = 1 + sizeof...(Types) };
	};

	template <typename T, typename ... Types>
	inline auto make_packet(T header, Types ... headers)
	{
		return packet<T, Types ...>(header, headers ...);
	}

}