#include<string>
#include<vector>
#include "PacketHandler.h"

PacketHandler::PacketHandler (){}
PacketHandler::~PacketHandler (){}

int PacketHandler::insertByte(int index, unsigned short val, std::vector<uint8_t>* packet) {
  packet->at(index++) = (val >> 8) & 0xff;
  packet->at(index++) = val & 0xff;
  return index;
}

int PacketHandler::insertByte(int index, unsigned long val, std::vector<uint8_t>* packet) {
  packet->at(index++) = (val >> 24) & 0xff;
  packet->at(index++) = (val >> 16) & 0xff;
  packet->at(index++) = (val >> 8) & 0xff;
  packet->at(index++) = val & 0xff;
  return index;
}

int PacketHandler::insertByte(int index, std::string val, std::vector<uint8_t>* packet) {
  for(uint8_t byte: val) {
    packet->at(index++) = byte;
  }
  return index;
}

unsigned short PacketHandler::toShort(int index, std::vector<uint8_t>* packet) {
  unsigned short value = (packet->at(index) << 8) | (packet->at(index + 1) << 8);
  return value;
}

unsigned long PacketHandler::toLong(int index, std::vector<uint8_t>* packet) {
  unsigned long value = (packet->at(index) << 24) | (packet->at(index + 1) << 16) | (packet->at(index + 2) << 8) | (packet->at(index + 3));
  return value;
}

void PacketHandler::parse_packet(std::vector<uint8_t>* packet, parsed_packet* parsed) {
  parsed->id = PacketHandler::toShort(0, packet);
  parsed->length = PacketHandler::toLong(2, packet);
  parsed->message.assign(packet->begin() + 6, packet->end());
}

void PacketHandler::gen_packet(parsed_packet* packet_info, std::vector<uint8_t>* packet) {
  packet->resize(2 + 4 + 2 + packet_info->ip.size() + packet_info->message.size());
  int index = 0;
  index = PacketHandler::insertByte(index, packet_info->id, packet);
  index = PacketHandler::insertByte(index, (unsigned long)packet->size(), packet);
  index = PacketHandler::insertByte(index, (unsigned short)packet_info->ip.size(), packet);
  index = PacketHandler::insertByte(index, packet_info->ip, packet);
  index = PacketHandler::insertByte(index, packet_info->message, packet);
}
