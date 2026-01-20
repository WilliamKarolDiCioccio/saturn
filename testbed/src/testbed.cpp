#include "testbed.hpp"

#include <chrono>
#include <iostream>

#include <saturn/tools/logger.hpp>
#include <saturn/core/timer.hpp>
#include <saturn/window/window_system.hpp>
#include <saturn/input/input_system.hpp>
#include <saturn/graphics/render_system.hpp>
#include <saturn/ecs/entity_registry.hpp>

#include <pieces/utils/string.hpp>

using namespace std::chrono_literals;

namespace testbed
{

void TestbedApplication::onInitialize()
{
    auto window = getWindowSystem()->getWindow("MainWindow");

    window->setResizeable(true);

    auto inputContext = getInputSystem()->getContext(window);

#ifndef SATURN_PLATFORM_ANDROID
    inputContext->updateVirtualKeyboardKeys({
        {"closeApp", input::KeyboardKey::key_escape},
        {"moveLeft", input::KeyboardKey::key_a},
        {"moveRight", input::KeyboardKey::key_d},
        {"moveUp", input::KeyboardKey::key_w},
        {"moveDown", input::KeyboardKey::key_s},
        {"resetCamera", input::KeyboardKey::key_r},
    });

    inputContext->addMouseInputSource();
    inputContext->addKeyboardInputSource();

    if (inputContext->addUnifiedTextInputSource().isOk())
    {
        inputContext->getUnifiedTextInputSource()->setEnabled(false);
    }

    inputContext->registerActions({
        input::Action{
            "moveLeft",
            "Move the camera left.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("moveLeft"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::hold);
            },
        },
        input::Action{
            "moveRight",
            "Move the camera right.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("moveRight"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::hold);
            },
        },
        input::Action{
            "moveUp",
            "Move the camera up.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("moveUp"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::hold);
            },
        },
        input::Action{
            "moveDown",
            "Move the camera down.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("moveDown"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::hold);
            },
        },
        input::Action{
            "resetCamera",
            "Reset the camera position.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("resetCamera"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::release) &&
                       event.metadata.duration > 2500ms && event.metadata.duration < 5000ms;
            },
        },
        input::Action{
            "closeApp",
            "Close the application.",
            [](input::InputContext* ctx)
            {
                auto src = ctx->getKeyboardInputSource();

                const auto srcPollCount = src->getPollCount();
                const auto event = src->getKeyEvent(ctx->translateKey("closeApp"));

                return event.metadata.pollCount == srcPollCount &&
                       utils::hasFlag(event.state, input::ActionableState::double_press);
            },
        },
    });
#endif

    SATURN_INFO("Testbed initialized.");
}

void TestbedApplication::onUpdate() {}

void TestbedApplication::onPause() { SATURN_INFO("Testbed paused."); }

void TestbedApplication::onResume() { SATURN_INFO("Testbed resumed."); }

void TestbedApplication::onShutdown()
{
    getRenderSystem()->destroyAllContexts();
    getInputSystem()->unregisterAllWindows();
    getWindowSystem()->destroyAllWindows();

    SATURN_INFO("Testbed shutdown.");
}

void TestbedApplication::onPollInputs()
{
    auto window = getWindowSystem()->getWindow("MainWindow");

    auto inputContext = getInputSystem()->getContext(window);

    if (inputContext->isActionTriggered("moveLeft")) SATURN_INFO("Moving left.");
    if (inputContext->isActionTriggered("moveRight")) SATURN_INFO("Moving right.");
    if (inputContext->isActionTriggered("moveUp")) SATURN_INFO("Moving up.");
    if (inputContext->isActionTriggered("moveDown")) SATURN_INFO("Moving down.");
    if (inputContext->isActionTriggered("resetCamera")) SATURN_INFO("Resetting camera.");

    if (window->shouldClose() || inputContext->isActionTriggered("closeApp"))
    {
        return requestExit();
    }
}

} // namespace testbed
