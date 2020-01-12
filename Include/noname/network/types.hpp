#pragma once

namespace noname_core {
	namespace network {
		enum class PacketType : int {
			Ethernet,
			ARP,
			RARP,
			IP,
			TCP,
			UDP,
			HTTP,
			UNKNOWN
		};
	}
}