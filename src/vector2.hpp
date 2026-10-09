#pragma once

#include <cmath>
#include <type_traits>
#include <ostream>
#include <SDL.h>

// Vector2<T>
template <typename T>
struct Vector2
{
    static_assert(std::is_arithmetic<T>::value, "Vector2<T> requires an arithmetic type");

    T x{};
    T y{};

    // ---------------- Constructors ----------------
    constexpr Vector2() = default;
    constexpr Vector2(T x_, T y_) : x(x_), y(y_) {}

    // Convert from a differently-typed Vector2
    template <typename U>
    constexpr explicit Vector2(const Vector2<U>& other)
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

    // ---------------- SDL interop ----------------
    // Only meaningful for integer types, but kept generic for convenience.
    constexpr SDL_Point toSDLPoint() const
    {
        return SDL_Point{ static_cast<int>(x), static_cast<int>(y) };
    }

    constexpr SDL_FPoint toSDLFPoint() const
    {
        return SDL_FPoint{ static_cast<float>(x), static_cast<float>(y) };
    }

    static constexpr Vector2<T> fromSDLPoint(const SDL_Point& p)
    {
        return Vector2<T>(static_cast<T>(p.x), static_cast<T>(p.y));
    }

    static constexpr Vector2<T> fromSDLFPoint(const SDL_FPoint& p)
    {
        return Vector2<T>(static_cast<T>(p.x), static_cast<T>(p.y));
    }

    // ---------------- Arithmetic operators ----------------
    constexpr Vector2<T> operator+(const Vector2<T>& rhs) const { return { x + rhs.x, y + rhs.y }; }
    constexpr Vector2<T> operator-(const Vector2<T>& rhs) const { return { x - rhs.x, y - rhs.y }; }
    constexpr Vector2<T> operator-() const { return { -x, -y }; }

    constexpr Vector2<T> operator*(T scalar) const { return { x * scalar, y * scalar }; }
    constexpr Vector2<T> operator/(T scalar) const { return { x / scalar, y / scalar }; }

    // Component-wise multiply/divide
    constexpr Vector2<T> operator*(const Vector2<T>& rhs) const { return { x * rhs.x, y * rhs.y }; }
    constexpr Vector2<T> operator/(const Vector2<T>& rhs) const { return { x / rhs.x, y / rhs.y }; }

    Vector2<T>& operator+=(const Vector2<T>& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vector2<T>& operator-=(const Vector2<T>& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vector2<T>& operator*=(T scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2<T>& operator/=(T scalar) { x /= scalar; y /= scalar; return *this; }

    // ---------------- Comparison ----------------
    constexpr bool operator==(const Vector2<T>& rhs) const { return x == rhs.x && y == rhs.y; }
    constexpr bool operator!=(const Vector2<T>& rhs) const { return !(*this == rhs); }

    // ---------------- Vector math ----------------
    constexpr T dot(const Vector2<T>& rhs) const { return x * rhs.x + y * rhs.y; }

    // z-component of the 3D cross product (scalar for 2D)
    constexpr T cross(const Vector2<T>& rhs) const { return x * rhs.y - y * rhs.x; }

    T lengthSquared() const { return x * x + y * y; }

    T length() const
    {
        return static_cast<T>(std::sqrt(static_cast<double>(lengthSquared())));
    }

    T distanceTo(const Vector2<T>& other) const { return (*this - other).length(); }
    T distanceSquaredTo(const Vector2<T>& other) const { return (*this - other).lengthSquared(); }

    // Returns a unit vector; safe against divide-by-zero (returns {0,0})
    Vector2<T> normalized() const
    {
        T len = length();
        if (len == static_cast<T>(0))
            return Vector2<T>(0, 0);
        return *this / len;
    }

    void normalize()
    {
        T len = length();
        if (len != static_cast<T>(0))
        {
            x /= len;
            y /= len;
        }
    }

    // Angle in radians, relative to +x axis
    double angle() const
    {
        return std::atan2(static_cast<double>(y), static_cast<double>(x));
    }

    // Rotate by radians (returns new vector; only meaningful for floating types)
    Vector2<T> rotated(double radians) const
    {
        double c = std::cos(radians);
        double s = std::sin(radians);
        return Vector2<T>(
            static_cast<T>(x * c - y * s),
            static_cast<T>(x * s + y * c)
        );
    }

    // Perpendicular vector (90 degrees counter-clockwise)
    constexpr Vector2<T> perpendicular() const { return { -y, x }; }

    // Linear interpolation
    static Vector2<T> lerp(const Vector2<T>& a, const Vector2<T>& b, double t)
    {
        return Vector2<T>(
            static_cast<T>(a.x + (b.x - a.x) * t),
            static_cast<T>(a.y + (b.y - a.y) * t)
        );
    }

    static constexpr Vector2<T> zero() { return Vector2<T>(0, 0); }
    static constexpr Vector2<T> one()  { return Vector2<T>(1, 1); }
};

//<--------Free Functions-------->
template <typename T>
constexpr Vector2<T> operator*(T scalar, const Vector2<T>& v)
{
    return v * scalar;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector2<T>& v)
{
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;