#include "stdafx.h"
#include "frame_set.h"

frame_set::frame_set(std::vector<std::unique_ptr<frame>> frames)
    :frames(std::move(frames))
{
}

const frame& frame_set::get(size_t current_image) const
{
    return *frames.at(current_image);
}
