#pragma once

#include "header.hpp"
#include "types.hpp"

#include <optional>

#ifdef _WIN32
#include <WinSock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#endif // _WIN32

#ifdef __linux__
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#endif // __linux__

namespace noname_core {
	namespace network {
#pragma pack(push, 1)
		struct mac_address final {
			static constexpr auto LEN = 6;

			uint8_t address[LEN] = { 0 };

			mac_address();
			mac_address(const uint8_t* data);
			mac_address(const mac_address& m);

			std::string to_string(const char delimiter) const;

			mac_address  operator+(const mac_address& m) const = delete;
			mac_address  operator-(const mac_address& m) const = delete;
			mac_address  operator*(const mac_address& m) const = delete;
			mac_address  operator/(const mac_address& m) const = delete;
			mac_address  operator%(const mac_address& m) const = delete;
			mac_address& operator=(const mac_address& m);
			bool		 operator==(const mac_address& m) const;
			bool		 operator!=(const mac_address& m) const;

			friend std::ostream& operator<<(std::ostream& os, const mac_address& m);
		};

		struct ethernet_header final : public header<ethernet_header> {
			static constexpr auto ETHER_TYPE_IP = 0x0800;
			static constexpr auto ETHER_TYPE_ARP = 0x0806;
			static constexpr auto ETHER_TYPE_RARP = 0x0835;

		private:
			mac_address destination;
			mac_address source;
			uint16_t	ether_type;

		public:
			ethernet_header();
			ethernet_header(const uint8_t* data);
			ethernet_header(const ethernet_header& e);

			void		set_destination(mac_address destination);
			void		set_source(mac_address source);
			void		set_ether_type(uint16_t ether_type);
			mac_address get_destination() const;
			mac_address get_source() const;
			uint16_t	get_ether_type() const;

			std::string to_string() const;
			PacketType	get_next_packet_type() const;

			ethernet_header  operator+(const ethernet_header& e) const = delete;
			ethernet_header  operator-(const ethernet_header& e) const = delete;
			ethernet_header  operator*(const ethernet_header& e) const = delete;
			ethernet_header  operator/(const ethernet_header& e) const = delete;
			ethernet_header  operator%(const ethernet_header& e) const = delete;
			ethernet_header& operator=(const ethernet_header& e);
			bool			 operator==(const ethernet_header& e) const;
			bool			 operator!=(const ethernet_header& e) const;

			friend std::ostream& operator<<(std::ostream& os, const ethernet_header& e);
		};
#pragma pack(pop)

		inline mac_address::mac_address() : address{ 0 } {  }

		inline mac_address::mac_address(const uint8_t* data) : address{ 0 }
		{
			for (int i = 0; i < LEN; ++i)
				address[i] = data[i];
		}

		inline mac_address::mac_address(const mac_address& m) : address{ 0 }
		{
			for (int i = 0; i < LEN; ++i)
				address[i] = m.address[i];
		}

		inline std::string mac_address::to_string(const char delimiter = ':') const
		{
			std::ostringstream ss;

			ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
				<< static_cast<int>(address[0]) << delimiter
				<< static_cast<int>(address[1]) << delimiter
				<< static_cast<int>(address[2]) << delimiter
				<< static_cast<int>(address[3]) << delimiter
				<< static_cast<int>(address[4]) << delimiter
				<< static_cast<int>(address[5]);

			return ss.str();
		}

		inline mac_address& mac_address::operator=(const mac_address& m)
		{
			for (int i = 0; i < LEN; ++i)
				this->address[i] = m.address[i];
			return *this;
		}

		inline bool mac_address::operator==(const mac_address& m) const
		{
			for (int i = 0; i < LEN; ++i)
				if (address[i] != m.address[i]) return false;
			return true;
		}

		inline bool mac_address::operator!=(const mac_address& m) const
		{
			return !(*this == m);
		}

		inline std::ostream& operator<<(std::ostream& os, const mac_address& m)
		{
			os << m.to_string();
			return os;
		}

		inline ethernet_header::ethernet_header()
			: destination()
			, source()
			, ether_type(0) { }

		inline ethernet_header::ethernet_header(const uint8_t* data)
			: destination(data)
			, source(data + mac_address::LEN)
			, ether_type(*(data + 2 * mac_address::LEN)) { }

		inline ethernet_header::ethernet_header(const ethernet_header& e)
			: destination(e.destination)
			, source(e.source)
			, ether_type(e.ether_type) { }

		inline void ethernet_header::set_destination(mac_address destination) { this->destination = destination; }
		inline void ethernet_header::set_source(mac_address source) { this->source = source; }
		inline void ethernet_header::set_ether_type(uint16_t ether_type) { this->ether_type = ether_type; }

		inline mac_address ethernet_header::get_destination() const { return destination; }
		inline mac_address ethernet_header::get_source() const { return source; }
		inline uint16_t ethernet_header::get_ether_type() const { return ether_type; }

		inline std::string ethernet_header::to_string() const
		{
			std::ostringstream ss;

			ss << "Destination mac address: " << destination << std::endl
				<< "Source mac address: " << source << std::endl;

			return ss.str();
		}

		inline PacketType ethernet_header::get_next_packet_type() const
		{
			switch (ether_type)
			{
			case ETHER_TYPE_IP:
				return PacketType::IP;
			case ETHER_TYPE_ARP:
				return PacketType::ARP;
			case ETHER_TYPE_RARP:
				return PacketType::RARP;
			default:
				break;
			}
			return PacketType::UNKNOWN;
		}

		inline ethernet_header& ethernet_header::operator=(const ethernet_header& e)
		{
			destination = e.destination;
			source = e.source;
			ether_type = e.ether_type;
			return *this;
		}

		inline bool ethernet_header::operator==(const ethernet_header& e) const
		{
			return destination == e.destination
				&& source == e.source
				&& ether_type == e.ether_type;
		}

		inline bool ethernet_header::operator!=(const ethernet_header& e) const
		{
			return !(*this == e);
		}

		inline std::ostream& operator<<(std::ostream& os, const ethernet_header& e)
		{
			os << e.to_string();
			return os;
		}
	} // namespace network
} // namespace noname_core