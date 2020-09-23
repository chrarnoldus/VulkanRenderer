#pragma once
#include "frame.h"

class frame_set
{
    std::vector<std::unique_ptr<frame>> frames;
public:
    frame_set(std::vector<std::unique_ptr<frame>> frames);
    const frame& get(size_t current_image) const;
};

