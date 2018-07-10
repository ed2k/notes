%module progpow
%include "stdint.i"  
%include "carrays.i"

/* Create some functions for working with "int *" */
%array_functions(uint8_t, uint8array);

%{

/* Put header files here or function declarations like below */
extern int get_block_progpow_hash_from_mix(uint8_t*,uint8_t*,uint64_t, uint8_t*);
%}

extern int get_block_progpow_hash_from_mix(uint8_t*,uint8_t*,uint64_t, uint8_t*);

