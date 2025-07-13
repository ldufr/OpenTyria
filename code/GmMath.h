#pragma once

float Vec2f_Dist2(Vec2f left, Vec2f right)
{
    float dx = right.x - left.x;
    float dy = right.y - left.y;
    return sqrtf(dx * dx + dy * dy);
}

Vec2f Vec2fSub(Vec2f v1, Vec2f v2)
{
    return (Vec2f) { v1.x - v2.x, v1.y - v2.y };
}

// < 0 => v1 is to the left of v2
// > 0 => v1 is to the right of v2
// = 0 => v1 and v2 are on the same line
float Vec2fCross(Vec2f v1, Vec2f v2)
{
    return (v1.x * v2.y) - (v1.y * v2.x);
}

float Vec2fDot(Vec2f v1, Vec2f v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y);
}

float Vec2fDist(Vec2f v1, Vec2f v2)
{
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    return sqrtf(dx * dx + dy * dy);
}
