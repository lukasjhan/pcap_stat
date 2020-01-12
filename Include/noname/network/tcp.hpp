#pragma once

#include "header.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace noname_core {
	namespace network {
#pragma pack(push, 1)
		struct tcp_header final : public header<tcp_header> {
		private:
			uint16_t src_port;
			uint16_t des_port;

			uint32_t seq_num;
			uint32_t ack_num;

			uint16_t header_len_flags;
			uint16_t window_size;
			uint16_t check_sum;
			uint16_t urgent_ptr;

		public:
			tcp_header();
			tcp_header(const uint8_t* data);
			tcp_header(const tcp_header& t);

			uint16_t get_src_port() const;
			uint16_t get_des_port() const;
			uint32_t get_seq_num() const;
			uint32_t get_ack_num() const;
			uint8_t get_header_length() const;
			uint8_t get_reserved() const;
			uint8_t get_flags() const;
			uint16_t get_window_size() const;
			uint16_t get_check_sum() const;
			uint16_t get_urgent_prt() const;

			//setter

			std::string to_string() const;

			//operators
			tcp_header operator+(const tcp_header& t) const = delete;
			tcp_header operator-(const tcp_header& t) const = delete;
			tcp_header operator*(const tcp_header& t) const = delete;
			tcp_header operator/(const tcp_header& t) const = delete;
			tcp_header operator%(const tcp_header& t) const = delete;
			tcp_header& operator=(const tcp_header& t);
			bool operator==(const tcp_header& t) const;
			bool operator!=(const tcp_header& t) const;

			friend std::ostream& operator<<(std::ostream& os, const tcp_header& t);
		};
#pragma pack(pop)
		inline tcp_header::tcp_header()
			: src_port(0)
			, des_port(0)
			, seq_num(0)
			, ack_num(0)
			, header_len_flags(0)
			, window_size(0)
			, check_sum(0)
			, urgent_ptr(0) { }

		inline tcp_header::tcp_header(const uint8_t* data)
			: src_port(data[0])
			, des_port(data[2])
			, seq_num(data[4])
			, ack_num(data[8])
			, header_len_flags(data[12])
			, window_size(data[14])
			, check_sum(data[16])
			, urgent_ptr(data[18]) { }

		inline tcp_header::tcp_header(const tcp_header& t)
			: src_port(t.src_port)
			, des_port(t.des_port)
			, seq_num(t.seq_num)
			, ack_num(t.ack_num)
			, header_len_flags(t.header_len_flags)
			, window_size(t.window_size)
			, check_sum(t.check_sum)
			, urgent_ptr(t.urgent_ptr) { }

		inline uint16_t tcp_header::get_src_port() const { return src_port; }
		inline uint16_t tcp_header::get_des_port() const { return des_port; }
		inline uint32_t tcp_header::get_seq_num() const { return seq_num; }
		inline uint32_t tcp_header::get_ack_num() const { return ack_num; }
		inline uint8_t tcp_header::get_header_length() const { return header_len_flags >> 12; }
		inline uint8_t tcp_header::get_reserved() const { return (header_len_flags >> 6) & 0x3F; }
		inline uint8_t tcp_header::get_flags() const { return header_len_flags & 0x3F; }
		inline uint16_t tcp_header::get_window_size() const { return window_size; }
		inline uint16_t tcp_header::get_check_sum() const { return check_sum; }
		inline uint16_t tcp_header::get_urgent_prt() const { return urgent_ptr; }

		inline std::string tcp_header::to_string() const
		{
			std::ostringstream ss;

			ss << "src port: " << bswap16(src_port) << std::endl
				<< "des port: " << bswap16(des_port) << std::endl;
			return ss.str();
		}

		inline tcp_header& tcp_header::operator=(const tcp_header& t)
		{
			src_port = t.src_port;
			des_port = t.des_port;
			seq_num = t.seq_num;
			ack_num = t.ack_num;
			header_len_flags = t.header_len_flags;
			window_size = t.window_size;
			check_sum = t.check_sum;
			urgent_ptr = t.urgent_ptr;
			return *this;
		}

		inline bool tcp_header::operator==(const tcp_header& t) const
		{
			return src_port == t.src_port
				&& des_port == t.des_port
				&& seq_num == t.seq_num
				&& ack_num == t.ack_num
				&& header_len_flags == t.header_len_flags
				&& window_size == t.window_size
				&& check_sum == t.check_sum
				&& urgent_ptr == t.urgent_ptr;
		}

		inline bool tcp_header::operator!=(const tcp_header& t) const
		{
			return !(*this == t);
		}

		inline std::ostream& operator<<(std::ostream& os, const tcp_header& t)
		{
			os << t.to_string();
			return os;
		}
	}
}