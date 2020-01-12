#include <iostream>
#include <map>
#include <algorithm>
#include <thread>
#include <future>

#include <pcap.h>

#include "noname/network/network.hpp"
#include "noname/channel/channel.hpp"
#include "noname/concurrent/concurrent_unordered_map.hpp"

struct packet_and_bytes {
	int packet;
	int bytes;
};

struct send_data {
	packet_and_bytes tx, rx;
};

struct Packet {
	struct pcap_pkthdr* header;
	const uint8_t* data;
};

void setup_map(
	std::map<std::pair<std::string, std::string>, send_data>& ret,
	std::string src,
	std::string des,
	int bytes
)
{
	auto key = std::make_pair(src, des);
	auto reverse_key = std::make_pair(des, src);

	if (ret.count(key) > 0) {
		ret[key].tx.bytes += bytes;
		ret[key].tx.packet += 1;
	}
	else if (ret.count(reverse_key) > 0) {
		ret[reverse_key].rx.bytes += bytes;
		ret[reverse_key].rx.packet += 1;
	}
	else {
		ret[key] = send_data{};
		ret[key].tx.bytes += bytes;
		ret[key].tx.packet += 1;
	}
}

int get_stats(
	noname_core::concurrent::concurrent_unordered_map<std::pair<std::string, std::string>, send_data>& ret_mac,
	noname_core::concurrent::concurrent_unordered_map<std::pair<std::string, std::string>, send_data>& ret_ip,
	noname_core::concurrent::concurrent_unordered_map<std::pair<std::string, std::string>, send_data>& ret_port,
	noname_core::channel::channel<Packet, 10>& input_chan
	)
{
	std::map<std::pair<std::string, std::string>, send_data> mac_stat, ip_stat, port_stat;

	while (1) {
		Packet packet;
		input_chan >> packet;

		if (packet.data == nullptr && packet.header == nullptr)
			break;

		noname_core::network::ethernet_header ether(packet.data);
		setup_map(mac_stat, ether.get_source().to_string(), ether.get_destination().to_string(), packet.header->caplen);

		if (ether.get_next_packet_type() != noname_core::network::PacketType::IP)
			continue;

		noname_core::network::ip_header ip(packet.data + sizeof ether);
		setup_map(ip_stat, ip.get_src_ip().to_string(), ip.get_des_ip().to_string(), packet.header->caplen);

		if (ip.get_next_packet_type() != noname_core::network::PacketType::TCP)
			continue;

		noname_core::network::tcp_header port(packet.data + sizeof ether + sizeof ip);
		setup_map(
			port_stat, 
			std::to_string(noname_core::network::bswap16(port.get_src_port())), 
			std::to_string(noname_core::network::bswap16(port.get_des_port())), 
			packet.header->caplen
		);
	}
	return 0;
}

void print_data(noname_core::concurrent::concurrent_unordered_map<std::pair<std::string, std::string>, send_data>& ret)
{
	std::cout << "A\t" << "B\t" << "A->B packet\t" << "A->B bytes\t" << "B->A packet\t" << "B->A bytes" << std::endl;

	for (auto& i : ret)
	{
		std::cout << i.first.first << "  ->  " << i.first.second << " :\t"
			<< i.second.rx.packet << "\t" << i.second.rx.bytes << "\t" << i.second.tx.packet << "\t" << i.second.tx.bytes << std::endl;
	}
	std::cout << std::endl;
}

int main()
{
	pcap_t* handle;
	char errbuf[PCAP_ERRBUF_SIZE];

	int res;
	int count = 0;
	int total_bytes = 0;
	int i = 0;

	struct pcap_pkthdr* header;
	const u_char* packet;

	noname_core::channel::channel<Packet, 10> chan[4];
	noname_core::concurrent::concurrent_unordered_map<std::pair<std::string, std::string>, send_data> ret_mac, ret_ip, ret_port;

	handle = pcap_open_offline("test.pcap", errbuf);
	if (handle == nullptr) return -1;

	std::vector<std::future<int>> threadpool;
	threadpool.reserve(4);

	for (int i = 0; i < 4; ++i)
		threadpool[i] = std::async(std::launch::async, get_stats, std::ref(ret_mac), std::ref(ret_ip), std::ref(ret_port), std::ref(chan[i]));
	
	do {
		res = pcap_next_ex(handle, &header, &packet);
		if (res == 0) continue;
		if (res == -1 || res == -2) break;

		chan[i] << Packet{ header, packet };

		i = (i + 1) % 4;

	} while (1);

	for (int i = 0; i < 4; ++i)
		chan[i].close();

	for (int i = 0; i < 4; ++i)
		auto ret = threadpool[i].get();

	print_data(ret_mac);
	print_data(ret_ip);
	print_data(ret_port);

	return 0;
}