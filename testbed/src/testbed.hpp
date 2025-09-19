#include <mosaic/core/application.hpp>

namespace testbed
{

using namespace mosaic;

class TestbedApplication : public mosaic::core::Application
{
   public:
    TestbedApplication() : Application("Testbed") {}

   private:
    void onInitialize() override;
    void onUpdate() override;
    void onPause() override;
    void onResume() override;
    void onShutdown() override;
    void onPollInputs() override;
};

} // namespace testbed
