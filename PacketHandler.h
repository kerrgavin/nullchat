#ifndef PACKETHANDLER
#define PACKETHANDLER

#include<string>
#include<vector>

typedef struct {
  std::string ip;
  unsigned short id;
  unsigned long length;
  std::string message;
} parsed_packet;

class PacketHandler {
private:
  /* data */

public:
  PacketHandler ();
  virtual ~PacketHandler ();

  static int insertByte(int index, unsigned short val, std::vector<uint8_t>* packet);

  static int insertByte(int index, unsigned long val, std::vector<uint8_t>* packet);

  static int insertByte(int index, std::string val, std::vector<uint8_t>* packet);

  static unsigned short toShort(int index, std::vector<uint8_t>* packet);

  static unsigned long toLong(int index, std::vector<uint8_t>* packet);

  static void parse_packet(std::vector<uint8_t>* packet, parsed_packet* parsed);

  static void gen_packet(parsed_packet* packet_info, std::vector<uint8_t>* packet);
};
#endif
