#pragma once

#include "header.hpp"
#include "types.hpp"
#include "utils.hpp"

#include <optional>
#include <iterator>

namespace noname_core {
	namespace network {
#pragma pack(push, 1)
		struct ip_address {
			static constexpr auto LEN = 4;

			uint8_t address[LEN];

			ip_address();
			ip_address(const uint8_t* data);
			ip_address(const ip_address& i);

			std::string to_string() const;

			ip_address operator+(const ip_address& i) const = delete;
			ip_address operator-(const ip_address& i) const = delete;
			ip_address operator*(const ip_address& i) const = delete;
			ip_address operator/(const ip_address& i) const = delete;
			ip_address operator%(const ip_address& i) const = delete;
			ip_address& operator=(const ip_address& i);
			bool operator==(const ip_address& i) const;
			bool operator!=(const ip_address& i) const;

			friend std::ostream& operator<<(std::ostream& os, const ip_address& i);
		};

		struct ip_header final : public header<ip_header> {
			static constexpr auto IP_PROTO_TCP = 6;
			static constexpr auto IP_PROTO_UDP = 17;

		private:
			uint8_t		header_length_and_version;
			uint8_t		ip_type_of_serivce;
			uint16_t	ip_length;
			uint16_t	ip_id;

			uint16_t	ip_flag_offset;
			uint8_t		ip_ttl;
			uint8_t		ip_proto;
			uint16_t	ip_check_sum;

			ip_address	src_ip_addr;
			ip_address	des_ip_addr;

		public:
			ip_header();
			ip_header(const uint8_t* data);
			ip_header(const ip_header& i);

			uint8_t get_version() const;
			uint8_t get_header_length() const;
			uint16_t get_length() const;
			uint16_t get_id() const;
			uint8_t get_flag() const;
			uint16_t get_frag_offset() const;
			uint8_t get_ttl() const;
			uint8_t get_proto() const;
			uint16_t get_checksum() const;
			ip_address get_src_ip() const;
			ip_address get_des_ip() const;

			//setter will be placed here

			std::string to_string() const;
			PacketType get_next_packet_type() const;

			ip_header	operator+(const ip_header& i) const = delete;
			ip_header	operator-(const ip_header& i) const = delete;
			ip_header	operator*(const ip_header& i) const = delete;
			ip_header	operator/(const ip_header& i) const = delete;
			ip_header	operator%(const ip_header& i) const = delete;
			ip_header& operator=(const ip_header& i);
			bool		operator==(const ip_header& i) const;
			bool		operator!=(const ip_header& i) const;

			friend std::ostream& operator<<(std::ostream& os, const ip_header& i);
		};
#pragma pack(pop)

		inline ip_address::ip_address() : address{ 0 } { }

		inline ip_address::ip_address(const uint8_t* data) : address{ 0 }
		{
			std::copy(address, address+3, data);
		}

		inline ip_address::ip_address(const ip_address& i) : address{ 0 }
		{
			std::copy(address, address+3, i.address);
		}

		inline std::string ip_address::to_string() const
		{
			std::ostringstream ss;
			ss << static_cast<int>(address[3]) << "."
				<< static_cast<int>(address[2]) << "."
				<< static_cast<int>(address[1]) << "."
				<< static_cast<int>(address[0]);
			return ss.str();
		}

		inline ip_address& ip_address::operator=(const ip_address& i)
		{
			for (int j = 0; j < LEN; ++j)
				address[j] = i.address[j];
			return *this;
		}

		inline bool ip_address::operator==(const ip_address& i) const
		{
			for (int j = 0; j < LEN; ++j)
				if (address[j] != i.address[j]) return false;
			return true;
		}

		inline bool ip_address::operator!=(const ip_address& i) const
		{
			return !(*this == i);
		}

		inline std::ostream& operator<<(std::ostream& os, const ip_address& i)
		{
			os << i.to_string();
			return os;
		}

		inline ip_header::ip_header()
			: header_length_and_version(0)
			, ip_type_of_serivce(0)
			, ip_length(0)
			, ip_id(0)
			, ip_flag_offset(0)
			, ip_ttl(0)
			, ip_proto(0)
			, ip_check_sum(0)
			, src_ip_addr(), des_ip_addr() { }

		inline ip_header::ip_header(const uint8_t* data)
			: header_length_and_version(data[0])
			, ip_type_of_serivce(data[1])
			, ip_length(data[2])
			, ip_id(data[4])
			, ip_flag_offset(data[6])
			, ip_ttl(data[8])
			, ip_proto(data[9])
			, ip_check_sum(data[10])
			, src_ip_addr(data + 12), des_ip_addr(data + 16) { }

		inline ip_header::ip_header(const ip_header& i)
			: header_length_and_version(i.header_length_and_version)
			, ip_type_of_serivce(i.ip_type_of_serivce)
			, ip_length(i.ip_length)
			, ip_id(i.ip_id)
			, ip_flag_offset(i.ip_flag_offset)
			, ip_ttl(i.ip_ttl)
			, ip_proto(i.ip_proto)
			, ip_check_sum(i.ip_check_sum)
			, src_ip_addr(i.src_ip_addr), des_ip_addr(i.des_ip_addr) { }

		inline uint8_t ip_header::get_version() const { return header_length_and_version >> 4; }
		inline uint8_t ip_header::get_header_length() const { return header_length_and_version & 0x0F; }
		inline uint16_t ip_header::get_length() const { return bswap16(ip_length); }
		inline uint16_t ip_header::get_id() const { return bswap16(ip_id); }
		inline uint8_t ip_header::get_flag() const { return ip_flag_offset >> 13; }
		inline uint16_t ip_header::get_frag_offset() const { return ip_flag_offset & 0x1FFF; }
		inline uint8_t ip_header::get_ttl() const { return ip_ttl; }
		inline uint8_t ip_header::get_proto() const { return ip_proto; }
		inline uint16_t ip_header::get_checksum() const { return bswap16(ip_check_sum); }
		inline ip_address ip_header::get_src_ip() const { return src_ip_addr; }
		inline ip_address ip_header::get_des_ip() const { return des_ip_addr; }

		inline std::string ip_header::to_string() const
		{
			std::ostringstream ss;

			ss << "src ip: " << src_ip_addr << std::endl
				<< "des ip" << des_ip_addr << std::endl;

			return ss.str();
		}

		inline PacketType ip_header::get_next_packet_type() const
		{
			switch (ip_proto)
			{
			case IP_PROTO_TCP:
				return PacketType::TCP;
			case IP_PROTO_UDP:
				return PacketType::UDP;

			default:
				break;
			}
			return PacketType::UNKNOWN;
		}

		inline ip_header& ip_header::operator=(const ip_header& i)
		{
			header_length_and_version = i.header_length_and_version;
			ip_type_of_serivce = i.ip_type_of_serivce;
			ip_length = i.ip_length;
			ip_id = i.ip_id;
			ip_flag_offset = i.ip_flag_offset;
			ip_ttl = i.ip_ttl;
			ip_proto = i.ip_proto;
			ip_check_sum = i.ip_check_sum;

			src_ip_addr = i.src_ip_addr;
			des_ip_addr = i.des_ip_addr;
			return *this;
		}

		inline bool ip_header::operator==(const ip_header& i) const
		{
			return header_length_and_version == i.header_length_and_version
				&& ip_type_of_serivce == i.ip_type_of_serivce
				&& ip_length == i.ip_length
				&& ip_id == i.ip_id
				&& ip_flag_offset == i.ip_flag_offset
				&& ip_ttl == i.ip_ttl
				&& ip_proto == i.ip_proto
				&& ip_check_sum == i.ip_check_sum

				&& src_ip_addr == i.src_ip_addr
				&& des_ip_addr == i.des_ip_addr;
		}

		inline bool ip_header::operator!=(const ip_header& i) const
		{
			return !(*this == i);
		}

		inline std::ostream& operator<<(std::ostream& os, const ip_header& i)
		{
			os << i.to_string();
			return os;
		}
	}
}