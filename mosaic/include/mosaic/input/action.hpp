#pragma once

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "events.hpp"
#include "mappings.hpp"

namespace saturn
{
namespace input
{

class InputContext;

/**
 * @brief Represents an action that can be triggered by user input.
 *
 * An action consists of a name, a description, and a trigger function.
 * The trigger function takes an InputContext pointer and returns a boolean indicating whether the
 * action was triggered.
 */
struct Action
{
    std::string name;
    std::string description;
    std::function<bool(InputContext*)> trigger;

    Action() : name(""), description(""), trigger([](const InputContext*) { return false; }){};

    Action(const std::string& name, const std::string& description,
           std::function<bool(InputContext*)> trigger)
        : name(name), description(description), trigger(trigger){};
};

} // namespace input
} // namespace saturn
