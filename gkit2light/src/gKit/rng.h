
//! \file rng.h

#ifndef _RNG_H
#define _RNG_H

#include <cstdint>


template< typename UInt, UInt a, UInt c, UInt m >
struct RNGT
{
    RNGT( ) : x(), key() { seed(123451); }
    RNGT( const UInt s ) : x(), key() { seed(s); }
    void seed( const UInt s ) { key= (s << 1) | 1; x= key; }
    
    RNGT& index( const UInt i )
    {
        UInt tmul= a;
        UInt tadd= c;
        UInt mul= 1;
        UInt add= 0;
        
        UInt delta= i;
        while(delta)
        {
            if(delta & 1)
            {
                mul= mul * tmul;
                add= add * tmul + tadd;
            }
            
            tadd= tmul * tadd + tadd;
            tmul= tmul * tmul;
            delta= delta >> 1;
        }
        
        x= mul * key + add;
        return *this;
    }
    
    unsigned sample( ) { x= (a*x+c) % m; return unsigned(x); }
    
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
    
    // c++ interface, compatible avec la stl
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return unsigned(m-1); }
    typedef unsigned result_type;
    
    // etat interne
    UInt x;
    UInt key;
};


typedef RNGT<uint32_t, 48271u, 0u, (1u << 31) -1> RNG0;					// minstd constants
typedef RNGT<uint32_t, 1103515245u, 12345u, 1u << 31> RNG32;			// glibc constants
typedef RNGT<uint64_t, 6364136223846793005lu, 1lu, 1lu << 63> RNG64;	// newlib constants

#endif
