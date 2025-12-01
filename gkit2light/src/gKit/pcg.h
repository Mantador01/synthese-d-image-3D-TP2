
//! \file pcg.h

#ifndef _PCG_H
#define _PCG_H

#include <cstdint>


// reprise de http://www.pcg-random.org
// periode 2^64 mais 2^63 sequences aleatoires
struct PCG32 
{
    PCG32( ) : x(), x0(), key() { seed(0x853c49e6748fea9b, b); }
    PCG32( const uint64_t s, const uint64_t ss= b ) : x(), x0(), key() { seed(s, ss); }
    
    void seed( const uint64_t s, const uint64_t ss= b )
    {
        key= (ss << 1) | 1;
        
        x= key + s;
        sample();
        x0= x;
    }
    
    PCG32& index( const uint64_t i )
    {
        // advance to sample index
        // http://www.pcg-random.org implementation
        uint64_t tmul= a;
        uint64_t tadd= key;
        uint64_t mul= 1;
        uint64_t add= 0;
        
        uint64_t delta= i;
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
        
        x= mul * x0 + add;
        return *this;
    }
    
    unsigned sample( )
    {
        uint64_t xx= x;
        x= a*x + key;
        
        // hash(x)
        uint32_t tmp= ((xx >> 18u) ^ xx) >> 27u;
        uint32_t r= xx >> 59u;
        return (tmp >> r) | (tmp << ((~r + 1u) & 31));
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
    
    static constexpr uint64_t a= 0x5851f42d4c957f2d;
    static constexpr uint64_t b= 0xda3e39cb94b95bdb;
    
    uint64_t x;
    uint64_t x0;
    uint64_t key;
};


// reprise de https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
// version 32 bits, periode 2^32 mais 2^31 sequences aleatoires
struct PCG32I
{
    PCG32I( ) : x(), key() { seed(0x46b56677u, 2891336453u); }
    PCG32I( const unsigned s, const unsigned ss= 2891336453u ) : x(), key() { seed(s, ss); }
    
    void seed( const unsigned s, const unsigned ss )
    {
        key= (ss << 1) | 1;
        x= s + key;
        sample();
    }
    
    // todo index()
    
    unsigned sample( ) 
    {
        unsigned xx= x;
        x= x * 747796405u + key;
        
        unsigned tmp= ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        return (tmp >> 22u) ^ tmp;
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
    
    // c++ interface, compatible avec la stl
    unsigned operator() ( ) { return sample(); }
    static constexpr unsigned min( ) { return 0; }
    static constexpr unsigned max( ) { return ~unsigned(0); }
    typedef unsigned result_type;
    
    unsigned x;
    unsigned key;
};


#endif
