#pragma once

#include "VectorUtils.hpp"

#include <concepts>

namespace llgl
{

template<class T>
struct UninitializedVector2
{
    UninitializedVector2() = default;

    UninitializedVector2(T x_, T y_)
        : x { x_ }
        , y { y_ }
    {
    }

    explicit UninitializedVector2(concepts::Vector2 auto other)
        : x { other.x }
        , y { other.y }
    {
    }

    UninitializedVector2& operator=(concepts::Vector2 auto other)
    {
        if constexpr (std::is_same_v<decltype(&other), decltype(this)>)
        {
            if (this == &other)
                return *this;
        }
        x = other.x;
        y = other.y;
        return *this;
    }

    T x, y;
};

template<class T>
class Vector2
{
public:
    constexpr Vector2() = default;

    constexpr Vector2(T x_, T y_)
        : x { x_ }
        , y { y_ }
    {
    }

    constexpr explicit Vector2(concepts::Vector2 auto other)
        : x { static_cast<T>(other.x) }
        , y { static_cast<T>(other.y) }
    {
    }

    // Angle is CCW starting from positive X axis.
    constexpr static Vector2 create_polar(double angle_radians, double length) {
        return { std::sin(angle_radians) * length, std::cos(angle_radians) * length };
    }

    constexpr Vector2& operator=(concepts::Vector2 auto other)
    {
        if constexpr (std::is_same_v<decltype(&other), decltype(this)>)
        {
            if (this == &other)
                return *this;
        }
        x = other.x;
        y = other.y;
        return *this;
    }

    T x {}, y {};
};

using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned>;
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;

}
