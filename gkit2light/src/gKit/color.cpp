
#include <cmath>
#include <algorithm>

#include "color.h"


float Color::power( ) const
{
    return (r+g+b) / 3;
}

float Color::max( ) const
{
    return std::max(r, std::max(g, std::max(b, float(0))));
}


float srgb( const float x )
{
    if(x < float(0.0031308))
        return float(12.92) * x;
    else
        return float(1.055) * std::pow(x, float(1) / float(2.4)) - float(0.055);
}

Color srgb( const Color& x ) 
{
    return Color(srgb(x.r), srgb(x.g), srgb(x.b), x.a);
}


float linear( const float x )
{
    if(x < float(0.04045))
        return x / float(12.92);
    else
        return std::pow( (x + float(0.055)) / float(1.055), float(2.4) );
}

Color linear( const Color& x )
{
    return Color(linear(x.r), linear(x.g), linear(x.b), x.a);
}


Color Black( )
{
    return Color(0, 0, 0);
}

Color White( )
{
    return Color(1, 1, 1);
}

Color Red( )
{
    return Color(1, 0, 0);
}

Color Green( )
{
    return Color(0, 1, 0);
}

Color Blue( )
{
    return Color(0, 0, 1);
}

Color Yellow( )
{
    return Color(1, 1, 0);
}


Color operator+ ( const Color& a, const Color& b )
{
    return Color(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}

Color operator- ( const Color& c )
{
    return Color(-c.r, -c.g, -c.b, -c.a);
}

Color operator- ( const Color& a, const Color& b )
{
    return a + (-b);
}

Color operator* ( const Color& a, const Color& b )
{
    return Color(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

Color operator* ( const float k, const Color& c )
{
    return Color(c.r * k, c.g * k, c.b * k, c.a * k);
}

Color operator* ( const Color& c, const float k )
{
    return k * c;
}

Color operator/ ( const Color& a, const Color& b )
{
    return Color(a.r / b.r, a.g / b.g, a.b / b.b, a.a / b.a);
}

Color operator/ ( const float k, const Color& c )
{
    return Color(k / c.r, k / c.g, k / c.b, k / c.a);
}

Color operator/ ( const Color& c, const float k )
{
    float kk= 1 / k;
    return kk * c;
}
