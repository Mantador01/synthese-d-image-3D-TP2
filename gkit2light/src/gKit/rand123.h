
//! \file rand123.h

#ifndef _RAND123_H
#define _RAND123_H

#include <cstdint>


// philox 2x64 6 rounds
// rewritten from https://github.com/DEShawResearch/random123/blob/main/include/Random123/philox.h
struct Philox
{
    Philox( ) : ctr(), key() { seed64(0xcf492074862957a3); }
    Philox( const uint64_t s ) : ctr(), key() { seed64(s); }
    
    Philox( const unsigned hi, const unsigned lo= 0x40ba6a95 ) : ctr(), key() { seed(hi, lo); }
    void seed( const unsigned hi, const unsigned lo= 0x40ba6a95 ) { seed64( uint64_t(hi) << 32 | uint64_t(lo) );}
    
    void seed64( const uint64_t s ) 
    { 
        ctr= { { 0, 0x838ca1328d07d3ba } }; 
        key= { { s } }; 
    }
    
    Philox& index( const uint64_t i ) 
    { 
        ctr= { { i, 0x838ca1328d07d3ba } }; 
        return *this;
    }
    
    unsigned sample( ) 
    { 
        ctr.v[0]++;
        return philox2x64<6>(ctr, key).v[0];        // 6 rounds
        //~ return philox2x64<10>(ctr, key).v[0];   // 10 rounds
    }
    
    unsigned sample_range( const unsigned range )
    {
        // Efficiently Generating a Number in a Range
        // cf http://www.pcg-random.org/posts/bounded-rands.html
        unsigned divisor= ((-range) / range) + 1; // (2^32) / range
        if(divisor == 0) return 0;
        
        while(true)
        {
            unsigned x= sample() / divisor;
            if(x < range) return x;
        }
    }
    
    // c++ interface
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return ~unsigned(0); }
    typedef unsigned result_type;
    
protected:    
    struct array2x64 { uint64_t v[2]; };
    struct array1x64 { uint64_t v[1]; };
    
    array1x64 bumpkey( array1x64 key ) 
    { 
        key.v[0] += 0x9E3779B97F4A7C15UL; 
        return key; 
    }
    
    uint64_t mulhilo64( uint64_t a, uint64_t b, uint64_t* hip )
    { 
        __uint128_t product = __uint128_t(a) * __uint128_t(b); 
        *hip= uint64_t(product >> 64); 
        return uint64_t(product); 
    }
    
    array2x64 round( array2x64 ctr, array1x64 key )
    { 
        uint64_t hi; 
        uint64_t lo = mulhilo64(0xD2B74407B1CE6E93UL, ctr.v[0], &hi); 
        
        return { { hi^key.v[0]^ctr.v[1], lo } }; 
    }
    
    template< int R >
    array2x64 philox2x64( array2x64 ctr, array1x64 key )
    { 
        if(R>0) { ctr = round(ctr, key); } 
        if(R>1) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>2) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>3) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>4) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>5) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>6) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>7) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>8) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>9) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>10) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>11) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>12) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>13) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>14) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>15) { key = bumpkey(key); ctr = round(ctr, key); } 
        
        return ctr; 
    }
    
    array2x64 ctr;
    array1x64 key;
};


// philox 2x32 6 rounds
// rewritten from https://github.com/DEShawResearch/random123/blob/main/include/Random123/philox.h
struct Philox32
{
    Philox32( ) : n(), key()  { seed64(0xcf492074862957a3); }
    Philox32( const uint64_t s ) : n(), key() { seed(s); }
    Philox32( const unsigned hi, const unsigned lo= 0x40ba6a95 ) : n(), key() { seed(hi, lo); }
    
    void seed( const unsigned hi, const unsigned lo= 0x40ba6a95 ) { seed64(uint64_t(hi) << 32 | uint64_t(lo)); }
    void seed64( const uint64_t s ) 
    { 
        n= 0;
        key= { { unsigned(s >> 32) } }; 
    }
    
    Philox32& index( const uint64_t i ) 
    { 
        n= i;
        return *this;
    }
    
    unsigned sample( ) 
    { 
        n++;
        array2x32 ctr= { { unsigned(n >>32), unsigned(n) } }; 
        return philox2x32<6>(ctr, key).v[0];        // 6 rounds
        //~ return philox2x32<10>(ctr, key).v[0];   // 10 rounds
    }
    
    unsigned sample_range( const unsigned range )
    {
        // Efficiently Generating a Number in a Range
        // cf http://www.pcg-random.org/posts/bounded-rands.html
        unsigned divisor= ((-range) / range) + 1; // (2^32) / range
        if(divisor == 0) return 0;
        
        while(true)
        {
            unsigned x= sample() / divisor;
            if(x < range) return x;
        }
    }
    
    // c++ interface
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return ~unsigned(0); }
    typedef unsigned result_type;
    
protected:    
    struct array2x32 { unsigned v[2]; };
    struct array1x32 { unsigned v[1]; };
    
    array1x32 bumpkey( array1x32 key ) 
    { 
        key.v[0] += 0x9E3779B9; 
        return key; 
    }
    
    unsigned mulhilo32( unsigned a, unsigned b, unsigned* hip )
    {
        uint64_t product = uint64_t(a) * uint64_t(b); 
        *hip= unsigned(product >> 32); 
        return unsigned(product); 
    }

    array2x32 round( array2x32 ctr, array1x32 key )
    { 
        uint32_t hi, lo= mulhilo32(0xd256d193, ctr.v[0], &hi); 
        
        return { { hi^key.v[0]^ctr.v[1], lo } }; 
    }    
    
    template< int R >
    array2x32 philox2x32( array2x32 ctr, array1x32 key )
    { 
        if(R>0) { ctr = round(ctr, key); } 
        if(R>1) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>2) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>3) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>4) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>5) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>6) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>7) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>8) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>9) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>10) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>11) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>12) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>13) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>14) { key = bumpkey(key); ctr = round(ctr, key); } 
        if(R>15) { key = bumpkey(key); ctr = round(ctr, key); } 
        
        return ctr; 
    }
    
    uint64_t n;
    array1x32 key;
};


// threefry 2x32 13 rounds
// rewritten from https://github.com/DEShawResearch/random123/blob/main/include/Random123/threefry.h
struct Threefry
{
    Threefry( ) : n(), key() { seed64(0xcf492074862957a3); }
    Threefry( const uint64_t s ) : n(), key() { seed64(s); }
    Threefry( const unsigned hi, const unsigned lo= 0xacac820c ) : n(), key() { seed(hi, lo); }
    
    void seed( const unsigned hi, const unsigned lo= 0xacac820c ) { seed64( uint64_t(hi) << 32 | uint64_t(lo) ); }
    void seed64( const uint64_t s ) 
    { 
        n= 0;
        key= { unsigned(s >> 32), unsigned(s) }; 
    }
    
    Threefry& index( const uint64_t  i ) 
    { 
        n= i;
        return *this;
    }
    
    unsigned sample( ) 
    { 
        n++;
        array2x32 ctr= { { unsigned(n >>32), unsigned(n) } }; 
        return threefry2x32<13>(ctr, key).v[0];
    }
    
    unsigned sample_range( const unsigned range )
    {
        // Efficiently Generating a Number in a Range
        // cf http://www.pcg-random.org/posts/bounded-rands.html
        unsigned divisor= ((-range) / range) + 1; // (2^32) / range
        if(divisor == 0) return 0;

        while(true)
        {
            unsigned x= sample() / divisor;
            if(x < range) return x;
        }
    }
    
    // c++ interface
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return ~unsigned(0); }
    typedef unsigned result_type;

    struct array2x32 { unsigned v[2]; };
    
private:
    static constexpr unsigned R_32x2_0_0= 13;
    static constexpr unsigned R_32x2_1_0= 15;
    static constexpr unsigned R_32x2_2_0= 26;
    static constexpr unsigned R_32x2_3_0= 6;
    static constexpr unsigned R_32x2_4_0= 17;
    static constexpr unsigned R_32x2_5_0= 29;
    static constexpr unsigned R_32x2_6_0= 16;
    static constexpr unsigned R_32x2_7_0= 24;
    
    unsigned rotl32( const unsigned x, const unsigned n ) { return (x << (n & 31)) | (x >> ((32 - n) & 31)); }
    
    template<int R >
    array2x32 threefry2x32( array2x32 ctr, array2x32 k )
    { 
        array2x32 x; 
        unsigned ks[3]= { 0, 0, 0x1BD11BDA };
        for(int i= 0; i < 2; i++)
        { 
            ks[i] = k.v[i]; 
            x.v[i] = ctr.v[i]; 
            ks[2] ^= k.v[i]; 
        }
        
        x.v[0] += ks[0]; 
        x.v[1] += ks[1]; 
        if(R>0) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_0_0); x.v[1] ^= x.v[0]; } 
        if(R>1) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_1_0); x.v[1] ^= x.v[0]; } 
        if(R>2) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_2_0); x.v[1] ^= x.v[0]; } 
        if(R>3) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_3_0); x.v[1] ^= x.v[0]; } 
        if(R>3) { x.v[0] += ks[1]; x.v[1] += ks[2]; x.v[1] += 1; } 
        if(R>4) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_4_0); x.v[1] ^= x.v[0]; } 
        if(R>5) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_5_0); x.v[1] ^= x.v[0]; } 
        if(R>6) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_6_0); x.v[1] ^= x.v[0]; } 
        if(R>7) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_7_0); x.v[1] ^= x.v[0]; } 
        if(R>7) { x.v[0] += ks[2]; x.v[1] += ks[0]; x.v[1] += 2; } 
        if(R>8) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_0_0); x.v[1] ^= x.v[0]; } 
        if(R>9) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_1_0); x.v[1] ^= x.v[0]; } 
        if(R>10) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_2_0); x.v[1] ^= x.v[0]; } 
        if(R>11) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_3_0); x.v[1] ^= x.v[0]; } 
        if(R>11) { x.v[0] += ks[0]; x.v[1] += ks[1]; x.v[1] += 3; } 
        if(R>12) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_4_0); x.v[1] ^= x.v[0]; } 
        if(R>13) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_5_0); x.v[1] ^= x.v[0]; } 
        if(R>14) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_6_0); x.v[1] ^= x.v[0]; } 
        if(R>15) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_7_0); x.v[1] ^= x.v[0]; } 
        if(R>15) { x.v[0] += ks[1]; x.v[1] += ks[2]; x.v[1] += 4; } 
        if(R>16) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_0_0); x.v[1] ^= x.v[0]; } 
        if(R>17) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_1_0); x.v[1] ^= x.v[0]; } 
        if(R>18) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_2_0); x.v[1] ^= x.v[0]; } 
        if(R>19) { x.v[0] += x.v[1]; x.v[1] = rotl32(x.v[1], R_32x2_3_0); x.v[1] ^= x.v[0]; } 
        if(R>19) { x.v[0] += ks[2]; x.v[1] += ks[0]; x.v[1] += 5; } 
        
        return x; 
    }
    
    uint64_t n;
    array2x32 key;
};


#ifdef __AES__
#include <x86intrin.h>

// ars 1x128 5 rounds
// rewritten from https://github.com/DEShawResearch/random123/blob/main/include/Random123/ars.h
// utilise les instructions AES, compiler avec g++ -maes ou g++ -march=native 
struct Ars
{
    Ars( ) : ctr(), key() { seed64(0xcf492074862957a3); }
    Ars( const uint64_t s ) : ctr(), key() { seed64(s); }
    Ars( const unsigned hi, const unsigned lo= 0xacac820c ) : ctr(), key() { seed(hi, lo); }
    
    void seed( const unsigned hi, const unsigned lo= 0xacac820c ) { seed64( uint64_t(hi) << 32 | uint64_t(lo) ); }
    void seed64( const uint64_t  s ) 
    {
        ctr= { 0, 0x838ca1328d07d3ba }; 
        key= { s, 0xcf492074862957a3 }; 
    }
    
    Ars& index( const uint64_t  i ) 
    { 
        ctr= { i, 0x838ca1328d07d3ba }; 
        return *this;
    }
    
    unsigned sample( ) 
    { 
        array1x128 one= { 1, 0 };
        ctr= _mm_add_epi64(ctr, one);
        
        array1x128 out= ars1x128<5>(ctr, key);
        return _mm_extract_epi32(out.v, 3);
    }
    
    unsigned sample_range( const unsigned range )
    {
        // Efficiently Generating a Number in a Range
        // cf http://www.pcg-random.org/posts/bounded-rands.html
        unsigned divisor= ((-range) / range) + 1; // (2^32) / range
        if(divisor == 0) return 0;
        
        while(true)
        {
            unsigned x= sample() / divisor;
            if(x < range) return x;
        }
    }
    
    // c++ interface
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return ~unsigned(0); }
    typedef unsigned result_type;
    
protected:
    struct array1x128 
    { 
        __m128i v; 
        
        array1x128( ) = default;
        array1x128( const uint64_t hi, const uint64_t lo ) { v= _mm_set_epi64x(hi, lo); }
        array1x128( const __m128i x ) { v= x; }
        operator __m128i( ) const { return v; }
    };
    
    template< int R >
    array1x128 ars1x128( array1x128 ctr, array1x128 key ) 
    {
        __m128i kweyl = _mm_set_epi64x(0xBB67AE8584CAA73BUL, 0x9E3779B97F4A7C15UL);
        __m128i k = key.v;
        __m128i v = _mm_xor_si128(ctr.v, k);
        
        if(R>1) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>2) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>3) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>4) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>5) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>6) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>7) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>8) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        if(R>9) { k = _mm_add_epi64(k, kweyl); v = _mm_aesenc_si128(v, k); }
        
        k = _mm_add_epi64(k, kweyl);
        v = _mm_aesenclast_si128(v, k);
        return v;
    }
    
    array1x128 ctr;
    array1x128 key;
};
#else
struct Ars {};  // compiler avec g++ -maes 
#endif


#endif
