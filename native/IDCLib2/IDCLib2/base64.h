//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#ifndef BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A
#define BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);
int base64_decode_to_buff(char* encoded_string, unsigned char* outBuff);
int base64_decode_to_buff_len(char* encoded_string, size_t max_in_len, unsigned char* outBuff);

#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */
