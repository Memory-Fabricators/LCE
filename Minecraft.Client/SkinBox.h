#pragma once

enum class BodyPart
{
    Unknown = 0,
    Head,
    Body,
    Arm0,
    Arm1,
    Leg0,
    Leg1,
};

struct SkinBox
{
    BodyPart ePart;
    float fX, fY, fZ, fW, fH, fD, fU, fV;
};
