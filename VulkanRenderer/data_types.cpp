#include "stdafx.h"
#include "data_types.h"

a2b10g10r10_snorm_pack32::a2b10g10r10_snorm_pack32(glm::vec3 rgb)
{
    assert(rgb.r >= -1.f && rgb.r <= 1.f);
    assert(rgb.g >= -1.f && rgb.g <= 1.f);
    assert(rgb.b >= -1.f && rgb.b <= 1.f);

    this->r = int(1023 * (rgb.r + 1) / 2 - 512);
    this->g = int(1023 * (rgb.g + 1) / 2 - 512);
    this->b = int(1023 * (rgb.b + 1) / 2 - 512);
    this->a = 0;
}

a2b10g10r10_unorm_pack32::a2b10g10r10_unorm_pack32(glm::vec3 rgb)
{
    assert(rgb.r >= 0.f && rgb.r <= 1.f);
    assert(rgb.g >= 0.f && rgb.g <= 1.f);
    assert(rgb.b >= 0.f && rgb.b <= 1.f);

    this->r = unsigned int(1023 * rgb.r);
    this->g = unsigned int(1023 * rgb.g);
    this->b = unsigned int(1023 * rgb.b);
    this->a = 0;
}
