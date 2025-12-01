
//! \file crng.h

#ifndef _CRNG_H
#define _CRNG_H

#include <cstdint>


// counter based rng, 32 bits
// periode 2^32
struct CRNG32
{   
    CRNG32( ) : n(), key() { seed(123451); }
    CRNG32( const unsigned s ) : n(), key() { seed(s); }
    void seed( const unsigned s ) { n= 0; key= (s << 1) | 1; }
    
    CRNG32& index( const unsigned i ) { n= i; return *this;}
    unsigned sample( ){ return hash(++n * key); }
    
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
    
    // 2 rounds
    unsigned hash( unsigned x )
    {
        x ^= x >> 16;
        x *= 0x21f0aaad;
        x ^= x >> 15;
        x *= 0xd35a2d97;
        x ^= x >> 15;
        return x;
    }
    // cf "hash prospector" https://github.com/skeeto/hash-prospector/blob/master/README.md
    
    unsigned n;
    unsigned key;    
};


// counter based rng, 64 bits
// periode 2^64
struct CRNG64
{
    CRNG64( ) : n(), key() { seed64(0xc58efd154ce32f6d); }
    CRNG64( const uint64_t s ) : n(), key() { seed64(s); }
    CRNG64( const unsigned hi, const unsigned lo= 0x40ba6a95 ) : n(), key() { seed(hi, lo); }
    
    void seed( const unsigned hi, const unsigned lo= 0x40ba6a95 ) { seed64( uint64_t(hi) << 32 | uint64_t(lo) ); }
    void seed64( const uint64_t s ) { n= 0; key= (s << 1) | 1; }
    
    CRNG64& index( const uint64_t i ) { n= i; return *this;}
    unsigned sample( ) { return hash(++n * key); }
    
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
    
    unsigned hash( uint64_t v ) 
    { 
        v ^= (v >> 31);
        v *= 0x7fb5d329728ea185;
        v ^= (v >> 27);
        v *= 0x81dadef4bc2dd44d;
        v ^= (v >> 33);
        return v;
    }
    // cf "hash prospector" https://github.com/skeeto/hash-prospector/blob/master/README.md
    
    uint64_t n;
    uint64_t key;
};

#endif
