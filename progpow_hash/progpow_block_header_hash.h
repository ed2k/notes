#ifndef _PROGPOW_BLOCK_HEADER_HASH_ 
#define _PROGPOW_BLOCK_HEADER_HASH_ 
extern "C" {

int
get_block_progpow_hash(uint8_t *header, uint8_t *mix, uint64_t nonce, uint8_t *out);

}
#endif 
