#pragma once

typedef enum DyeColor {
    DyeColor_None = 0,
    DyeColor_Blue = 2,
    DyeColor_Green = 3,
    DyeColor_Purple = 4,
    DyeColor_Red = 5,
    DyeColor_Yellow = 6,
    DyeColor_Brown = 7,
    DyeColor_Orange = 8,
    DyeColor_Silver = 9,
    DyeColor_Black = 10,
    DyeColor_Gray = 11,
    DyeColor_White = 12,
    DyeColor_Pink = 13
} DyeColor;

int DyeColor_FromInt(int value, DyeColor *result)
{
    switch (value) {
    case DyeColor_None:
    case DyeColor_Blue:
    case DyeColor_Green:
    case DyeColor_Purple:
    case DyeColor_Red:
    case DyeColor_Yellow:
    case DyeColor_Brown:
    case DyeColor_Orange:
    case DyeColor_Silver:
    case DyeColor_Black:
    case DyeColor_Gray:
    case DyeColor_White:
    case DyeColor_Pink:
        *result = (DyeColor)value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

DyeColor DyeColor_FromIntOrNone(int value)
{
    DyeColor result;
    if (DyeColor_FromInt(value, &result) != ERR_OK) {
        result = DyeColor_None;
    }
    return result;
}

DyeColor DyeColor_First(uint16_t colors)
{
    return DyeColor_FromIntOrNone(colors & 15);
}

DyeColor DyeColor_Second(uint16_t colors)
{
    return DyeColor_FromIntOrNone((colors >> 4) & 15);
}

DyeColor DyeColor_Third(uint16_t colors)
{
    return DyeColor_FromIntOrNone((colors >> 8) & 15);
}

DyeColor DyeColor_Fourth(uint16_t colors)
{
    return DyeColor_FromIntOrNone((colors >> 12) & 15);
}
