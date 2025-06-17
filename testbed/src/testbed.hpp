#include <mosaic/core/application.hpp>
#include <mosaic/core/logger.hpp>
#include <mosaic/core/timer.hpp>
#include <mosaic/window/window.hpp>
#include <mosaic/input/input_system.hpp>
#include <mosaic/graphics/render_system.hpp>

namespace testbed
{

using namespace mosaic;

class TestbedApplication : public mosaic::core::Application
{
   public:
    TestbedApplication() : Application("Testbed") {}

   private:
    std::optional<std::string> onInitialize() override;
    std::optional<std::string> onUpdate() override;
    void onPause() override;
    void onResume() override;
    void onShutdown() override;
};

} // namespace testbed
