#include <stdint.h>
#include <string.h>
//#include <iostream>

#include "progpow_block_header_hash.h"

/*
typedef uint8_t unsigned char;
typedef uint32_t unsigned int;
typedef uint64_t unsigned long long;
*/

union ethash_hash256
{
    uint64_t words[4];
    uint32_t hwords[8];
    uint8_t bytes[32];
};

uint32_t inline bswap32(uint32_t s)
{
    return ((s&0xff) <<24 | (s&0xff00) <<8 | (s&0xff0000) >> 8 |
            (s&0xff000000) >> 24);
}

/****************keccakf800***************************/
 /* Implementation based on:
        https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.c
        converted from 64->32 bit words*/
const uint32_t prog_keccakf_rndc[24] = {
        0x00000001, 0x00008082, 0x0000808a, 0x80008000, 0x0000808b, 0x80000001,
        0x80008081, 0x00008009, 0x0000008a, 0x00000088, 0x80008009, 0x8000000a,
        0x8000808b, 0x0000008b, 0x00008089, 0x00008003, 0x00008002, 0x00000080,
        0x0000800a, 0x8000000a, 0x80008081, 0x00008080, 0x80000001, 0x80008008
};

#define ROTL(x,n,w) (((x) << (n)) | ((x) >> ((w) - (n))))
#define ROTL32(x,n) ROTL(x,n,32)        /* 32 bits word */

void keccak_f800_round(uint32_t st[25], const int r)
{

#if 0
        const uint32_t keccakf_rotc[24] = {
                1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
                27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
        };
#endif 
    const uint32_t keccakf_rotc[24] = {
        1,  3,  6,  10, 15, 21, 28, 4, 13, 23, 2,  14,
        27, 9, 24, 8,  25, 11, 30, 18, 7, 29, 20, 12 
    };

        const uint32_t keccakf_piln[24] = {
                10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
                15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
        };

        uint32_t t, bc[5];
        /* Theta*/
        uint32_t i = 0, j = 0;
        for (i = 0; i < 5; i++)
                bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        i = 0;
        for (i = 0; i < 5; i++) {
                t = bc[(i + 4) % 5] ^ ROTL32(bc[(i + 1) % 5], 1);
                j = 0;
                for (j = 0; j < 25; j += 5)
                        st[j + i] ^= t;
        }

        /*Rho Pi*/
        i = 0;
        t = st[1];
        for (i = 0; i < 24; i++) {
                uint32_t jj = keccakf_piln[i];
                bc[0] = st[jj];
                st[jj] = ROTL32(t, keccakf_rotc[i]);
                t = bc[0];
        }

        /* Chi*/
        j = 0;
        for (j = 0; j < 25; j += 5) {
                i = 0;
                for (i = 0; i < 5; i++)
                        bc[i] = st[j + i];
                i = 0;
                for (i = 0; i < 5; i++)
                        st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota*/
        st[0] ^= prog_keccakf_rndc[r];
}

uint64_t keccak_f800(const ethash_hash256& header, const uint64_t seed, const uint32_t *result)
{
    uint32_t st[25];

    for (int i = 0; i < 25; i++)
        st[i] = 0;
    for (int i = 0; i < 8; i++)
        st[i] = header.hwords[i];
    st[8] = (uint32_t)seed;
    st[9] = (uint32_t)(seed >> 32);
    st[10] = result[0];
    st[11] = result[1];
    st[12] = result[2];
    st[13] = result[3];

    for (int r = 0; r < 21; r++) {
        keccak_f800_round(st, r);
    }
    // last round can be simplified due to partial output
    keccak_f800_round(st, 21);

    return (uint64_t)st[0] << 32 | st[1];//should be return (uint64_t)st[0] << 32 | st[1];
}

void keccak_f800(uint32_t* out, const ethash_hash256 header, const uint64_t seed, const uint32_t *result)
{
    uint32_t st[25];

    for (int i = 0; i < 25; i++)
        st[i] = 0;
    for (int i = 0; i < 8; i++)
        st[i] = header.hwords[i];
    st[8] = (uint32_t)seed;
    st[9] = (uint32_t)(seed >> 32);
    st[10] = result[0];
    st[11] = result[1];
    st[12] = result[2];
    st[13] = result[3];

    for (int r = 0; r < 21; r++) {
        keccak_f800_round(st, r);
    }
    // last round can be simplified due to partial output
    keccak_f800_round(st, 21);

    for (int i = 0; i < 8; ++i)
        out[i] = st[i];
}

ethash_hash256 ethash_keccak256(const ethash_hash256 header, const uint64_t seed, const uint32_t *result)
{
    ethash_hash256 hash;
    keccak_f800(hash.hwords, header, seed, result);
    return hash;
}

/**********keccakf1600***********************/
/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#include <stdint.h>

static uint64_t rol(uint64_t x, unsigned s)
{
    return (x << s) | (x >> (64 - s));
}

static const uint64_t round_constants[24] = {
    0x0000000000000001,
    0x0000000000008082,
    0x800000000000808a,
    0x8000000080008000,
    0x000000000000808b,
    0x0000000080000001,
    0x8000000080008081,
    0x8000000000008009,
    0x000000000000008a,
    0x0000000000000088,
    0x0000000080008009,
    0x000000008000000a,
    0x000000008000808b,
    0x800000000000008b,
    0x8000000000008089,
    0x8000000000008003,
    0x8000000000008002,
    0x8000000000000080,
    0x000000000000800a,
    0x800000008000000a,
    0x8000000080008081,
    0x8000000000008080,
    0x0000000080000001,
    0x8000000080008008,
};

void ethash_keccakf1600(uint64_t state[25])
{
    /* The implementation based on the "simple" implementation by Ronny Van Keer. */

    int round;

    uint64_t Aba, Abe, Abi, Abo, Abu;
    uint64_t Aga, Age, Agi, Ago, Agu;
    uint64_t Aka, Ake, Aki, Ako, Aku;
    uint64_t Ama, Ame, Ami, Amo, Amu;
    uint64_t Asa, Ase, Asi, Aso, Asu;

    uint64_t Eba, Ebe, Ebi, Ebo, Ebu;
    uint64_t Ega, Ege, Egi, Ego, Egu;
    uint64_t Eka, Eke, Eki, Eko, Eku;
    uint64_t Ema, Eme, Emi, Emo, Emu;
    uint64_t Esa, Ese, Esi, Eso, Esu;

    uint64_t Ba, Be, Bi, Bo, Bu;

    uint64_t Da, De, Di, Do, Du;

    Aba = state[0];
    Abe = state[1];
    Abi = state[2];
    Abo = state[3];
    Abu = state[4];
    Aga = state[5];
    Age = state[6];
    Agi = state[7];
    Ago = state[8];
    Agu = state[9];
    Aka = state[10];
    Ake = state[11];
    Aki = state[12];
    Ako = state[13];
    Aku = state[14];
    Ama = state[15];
    Ame = state[16];
    Ami = state[17];
    Amo = state[18];
    Amu = state[19];
    Asa = state[20];
    Ase = state[21];
    Asi = state[22];
    Aso = state[23];
    Asu = state[24];

    for (round = 0; round < 24; round += 2)
    {
        /* Round (round + 0): Axx -> Exx */

        Ba = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
        Be = Abe ^ Age ^ Ake ^ Ame ^ Ase;
        Bi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
        Bo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
        Bu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

        Da = Bu ^ rol(Be, 1);
        De = Ba ^ rol(Bi, 1);
        Di = Be ^ rol(Bo, 1);
        Do = Bi ^ rol(Bu, 1);
        Du = Bo ^ rol(Ba, 1);

        Ba = Aba ^ Da;
        Be = rol(Age ^ De, 44);
        Bi = rol(Aki ^ Di, 43);
        Bo = rol(Amo ^ Do, 21);
        Bu = rol(Asu ^ Du, 14);
        Eba = Ba ^ (~Be & Bi) ^ round_constants[round];
        Ebe = Be ^ (~Bi & Bo);
        Ebi = Bi ^ (~Bo & Bu);
        Ebo = Bo ^ (~Bu & Ba);
        Ebu = Bu ^ (~Ba & Be);

        Ba = rol(Abo ^ Do, 28);
        Be = rol(Agu ^ Du, 20);
        Bi = rol(Aka ^ Da, 3);
        Bo = rol(Ame ^ De, 45);
        Bu = rol(Asi ^ Di, 61);
        Ega = Ba ^ (~Be & Bi);
        Ege = Be ^ (~Bi & Bo);
        Egi = Bi ^ (~Bo & Bu);
        Ego = Bo ^ (~Bu & Ba);
        Egu = Bu ^ (~Ba & Be);

        Ba = rol(Abe ^ De, 1);
        Be = rol(Agi ^ Di, 6);
        Bi = rol(Ako ^ Do, 25);
        Bo = rol(Amu ^ Du, 8);
        Bu = rol(Asa ^ Da, 18);
        Eka = Ba ^ (~Be & Bi);
        Eke = Be ^ (~Bi & Bo);
        Eki = Bi ^ (~Bo & Bu);
        Eko = Bo ^ (~Bu & Ba);
        Eku = Bu ^ (~Ba & Be);

        Ba = rol(Abu ^ Du, 27);
        Be = rol(Aga ^ Da, 36);
        Bi = rol(Ake ^ De, 10);
        Bo = rol(Ami ^ Di, 15);
        Bu = rol(Aso ^ Do, 56);
        Ema = Ba ^ (~Be & Bi);
        Eme = Be ^ (~Bi & Bo);
        Emi = Bi ^ (~Bo & Bu);
        Emo = Bo ^ (~Bu & Ba);
        Emu = Bu ^ (~Ba & Be);

        Ba = rol(Abi ^ Di, 62);
        Be = rol(Ago ^ Do, 55);
        Bi = rol(Aku ^ Du, 39);
        Bo = rol(Ama ^ Da, 41);
        Bu = rol(Ase ^ De, 2);
        Esa = Ba ^ (~Be & Bi);
        Ese = Be ^ (~Bi & Bo);
        Esi = Bi ^ (~Bo & Bu);
        Eso = Bo ^ (~Bu & Ba);
        Esu = Bu ^ (~Ba & Be);


        /* Round (round + 1): Exx -> Axx */

        Ba = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
        Be = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
        Bi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
        Bo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
        Bu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

        Da = Bu ^ rol(Be, 1);
        De = Ba ^ rol(Bi, 1);
        Di = Be ^ rol(Bo, 1);
        Do = Bi ^ rol(Bu, 1);
        Du = Bo ^ rol(Ba, 1);

        Ba = Eba ^ Da;
        Be = rol(Ege ^ De, 44);
        Bi = rol(Eki ^ Di, 43);
        Bo = rol(Emo ^ Do, 21);
        Bu = rol(Esu ^ Du, 14);
        Aba = Ba ^ (~Be & Bi) ^ round_constants[round + 1];
        Abe = Be ^ (~Bi & Bo);
        Abi = Bi ^ (~Bo & Bu);
        Abo = Bo ^ (~Bu & Ba);
        Abu = Bu ^ (~Ba & Be);

        Ba = rol(Ebo ^ Do, 28);
        Be = rol(Egu ^ Du, 20);
        Bi = rol(Eka ^ Da, 3);
        Bo = rol(Eme ^ De, 45);
        Bu = rol(Esi ^ Di, 61);
        Aga = Ba ^ (~Be & Bi);
        Age = Be ^ (~Bi & Bo);
        Agi = Bi ^ (~Bo & Bu);
        Ago = Bo ^ (~Bu & Ba);
        Agu = Bu ^ (~Ba & Be);

        Ba = rol(Ebe ^ De, 1);
        Be = rol(Egi ^ Di, 6);
        Bi = rol(Eko ^ Do, 25);
        Bo = rol(Emu ^ Du, 8);
        Bu = rol(Esa ^ Da, 18);
        Aka = Ba ^ (~Be & Bi);
        Ake = Be ^ (~Bi & Bo);
        Aki = Bi ^ (~Bo & Bu);
        Ako = Bo ^ (~Bu & Ba);
        Aku = Bu ^ (~Ba & Be);

        Ba = rol(Ebu ^ Du, 27);
        Be = rol(Ega ^ Da, 36);
        Bi = rol(Eke ^ De, 10);
        Bo = rol(Emi ^ Di, 15);
        Bu = rol(Eso ^ Do, 56);
        Ama = Ba ^ (~Be & Bi);
        Ame = Be ^ (~Bi & Bo);
        Ami = Bi ^ (~Bo & Bu);
        Amo = Bo ^ (~Bu & Ba);
        Amu = Bu ^ (~Ba & Be);

        Ba = rol(Ebi ^ Di, 62);
        Be = rol(Ego ^ Do, 55);
        Bi = rol(Eku ^ Du, 39);
        Bo = rol(Ema ^ Da, 41);
        Bu = rol(Ese ^ De, 2);
        Asa = Ba ^ (~Be & Bi);
        Ase = Be ^ (~Bi & Bo);
        Asi = Bi ^ (~Bo & Bu);
        Aso = Bo ^ (~Bu & Ba);
        Asu = Bu ^ (~Ba & Be);
    }

    state[0] = Aba;
    state[1] = Abe;
    state[2] = Abi;
    state[3] = Abo;
    state[4] = Abu;
    state[5] = Aga;
    state[6] = Age;
    state[7] = Agi;
    state[8] = Ago;
    state[9] = Agu;
    state[10] = Aka;
    state[11] = Ake;
    state[12] = Aki;
    state[13] = Ako;
    state[14] = Aku;
    state[15] = Ama;
    state[16] = Ame;
    state[17] = Ami;
    state[18] = Amo;
    state[19] = Amu;
    state[20] = Asa;
    state[21] = Ase;
    state[22] = Asi;
    state[23] = Aso;
    state[24] = Asu;
}

/*********************keccak*******************/

#define to_le64(X) X
/** Loads 64-bit integer from given memory location as little-endian number. */
static inline uint64_t load_le(const uint8_t* data)
{
    /* memcpy is the best way of expressing the intention. Every compiler will
       optimize is to single load instruction if the target architecture
       supports unaligned memory access (GCC and clang even in O0).
       This is great trick because we are violating C/C++ memory alignment
       restrictions with no performance penalty. */
    uint64_t word;
    memcpy(&word, data, sizeof(word));
    return to_le64(word);
}

static inline void keccak(
    uint64_t* out, size_t bits, const uint8_t* data, size_t size)
{
    static const size_t word_size = sizeof(uint64_t);
    const size_t hash_size = bits / 8;
    const size_t block_size = (1600 - bits * 2) / 8;

    size_t i;
    uint64_t* state_iter;
    uint64_t last_word = 0;
    uint8_t* last_word_iter = (uint8_t*)&last_word;

    uint64_t state[25] = {0};

    while (size >= block_size)
    {
        for (i = 0; i < (block_size / word_size); ++i)
        {
            state[i] ^= load_le(data);
            data += word_size;
        }

        ethash_keccakf1600(state);

        size -= block_size;
    }

    state_iter = state;

    while (size >= word_size)
    {
        *state_iter ^= load_le(data);
        ++state_iter;
        data += word_size;
        size -= word_size;
    }

    while (size > 0)
    {
        *last_word_iter = *data;
        ++last_word_iter;
        ++data;
        --size;
    }
    *last_word_iter = 0x01;
    *state_iter ^= to_le64(last_word);

    state[(block_size / word_size) - 1] ^= 0x8000000000000000;

    ethash_keccakf1600(state);

    for (i = 0; i < (hash_size / word_size); ++i)
        out[i] = to_le64(state[i]);
}

union ethash_hash256 ethash_keccak256(const uint8_t* data, size_t size)
{
    union ethash_hash256 hash;
    keccak(hash.words, 256, data, size);
    return hash;
}
/*********************progpow hash *******************/
//return big endian of hash string
ethash_hash256 verify_final_progpow_hash(const ethash_hash256& header_hash, 
                        const ethash_hash256& mix_hash, uint64_t nonce)
{
    uint32_t result[4]; 
    for (int i = 0; i < 4; i++)
        result[i] = 0;
    uint64_t seed = keccak_f800(header_hash, nonce, result);
    ethash_hash256 h = ethash_keccak256(header_hash, seed, &mix_hash.hwords[0]);
    ethash_hash256 final_hash;
    //XXX keccak_f800 using st[0] << 32 | st[1], st[0], st[1] are little endian,
    //here convert it back to big endian
    for(int i = 0; i < 8; i ++) 
        final_hash.hwords[i] = bswap32(h.hwords[i]);

    return (final_hash);
}


/**************API********************************/
/**************************************************
 * header is 140 bytes with zerod nonce
 * mix is 32 bytes, nonce is 64 bytes
 * return with hash uint8_t array
 * hash is big endian, nonce is little endian.
***************************************************/
int 
get_block_progpow_hash(uint8_t *header, uint8_t *mix, uint64_t nonce, uint8_t *out)
{
    memset(&header[108], 0, 32); 
    ethash_hash256 header_hash = ethash_keccak256(header, 140);

    ethash_hash256 mix_hash;
    memcpy(mix_hash.bytes, mix, 32);

    ethash_hash256 final_hash = verify_final_progpow_hash (header_hash, 
                                            mix_hash, nonce);   

    memcpy(out, final_hash.bytes, 32);
    return 0;
}

/****
[2018-06-30 09:59:54] GPU #0: start=31300181 end=315a9f6d range=002a9dec
[2018-06-30 09:59:54] GPU #0: Target 1881cffffff.
[2018-06-30 10:00:08] GPU #0: found count = 2,  buffer->count = 2
[2018-06-30 10:00:08] GPU #0: found result 0: nonces=313dfc18, mixes=5883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b5aa6bc951d5a16f8
[2018-06-30 10:00:08] GPU #0: before verify, nonce = 313dfc18, header_hash =5d5378ed7576a89388fad85a628d1895483bf98bdb29b2fc9ad17db7263823f1, m_current_header=5d5378ed7576a89388fad85a628d1895483bf98bdb29b2fc9ad17db7263823f1, mix=5883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b
[2018-06-30 10:00:08] GPU #0: calc_hash: 00000148b6715b3a32a7b36e5c820fcd4660acb72947ab9ebe4949134688066e

[2018-06-30 10:00:09] GPU #0: progpow hash results. final_hash 00000148b6715b3a32a7b36e5c820fcd4660acb72947ab9ebe4949134688066e, mix_hash 5883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b5aa6bc951d5a16f8

[2018-06-30 10:00:09] GPU #0: verify_final_progpow passed!

[2018-06-30 10:00:10] GPU #0: verify passed!

yang@ubuntu:~/bitcoinnewyork_newfork/Bitcoinnyc/src$ ./btc2-cli -datadir=/data/bitcoin_data/ -debug=1 getblock 00000148b6715b3a32a7b36e5c820fcd4660acb72947ab9ebe4949134688066e 0
000000206812f2a87241851756cb79524ddb813a9f83bb4bbcdda51abe07daeba50000001280c5e80b3472eb0f63f14e0c2edebbbe0409fb30598ca0a5ee3674f9c7f875f3dd07000000000000000000000000000000000000000000000000000000000055b6375b1c88011e00000000000000000000000000000000000000000000000018fc3d3100000000fd40055883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b5aa6bc951d5a16f80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
***/

/*
uint8_t inline parse_digit(uint8_t d) 
{
    return d <= '9' ? (d - '0') : (d - 'a' + 10);
}

inline void to_array (const std::string& hex, uint8_t *out)
{
    for (size_t i = 1; i < hex.size(); i += 2) {

        int h = parse_digit(hex[i-1]);
        int l = parse_digit(hex[i]);
        out[i/2] = uint8_t((h<<4) | l);
    }
}


main() 
{
    
    uint8_t header[140];
    to_array (
    "000000206812f2a87241851756cb79524ddb813a9f83bb4bbcdda51abe07daeba50000001280c5e80b3472eb0f63f14e0c2edebbbe0409fb30598ca0a5ee3674f9c7f875f3dd07000000000000000000000000000000000000000000000000000000000055b6375b1c88011e00000000000000000000000000000000000000000000000018fc3d3100000000", header);

    uint8_t mix[32]; 
    to_array (
    "5883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b5aa6bc951d5a16f8",
    mix);

    uint64_t nonce = 0x313dfc18;

    uint8_t hash[32]; 
    get_block_progpow_hash(header, mix, nonce, hash);
    

    uint8_t expected[32];
    to_array (
    "00000148b6715b3a32a7b36e5c820fcd4660acb72947ab9ebe4949134688066e", 
    expected);

    if (memcmp(expected, hash, 32)) {
        std::cout << "hash calculation incorrect!!! "  << "\n";
    } else {
        std::cout << "hash calculation passed. "  << "\n";
    }

} 
*/
