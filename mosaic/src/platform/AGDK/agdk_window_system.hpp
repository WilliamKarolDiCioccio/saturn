#pragma once

#include "saturn/window/window_system.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

class AGDKWindowSystem : public window::WindowSystem
{
   public:
    ~AGDKWindowSystem() override = default;

   public:
    virtual pieces::RefResult<System, std::string> initialize() override;
    void shutdown() override;

    virtual pieces::RefResult<System, std::string> update() override;
};

} // namespace agdk
} // namespace platform
} // namespace saturn
