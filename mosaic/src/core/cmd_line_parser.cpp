#include "saturn/core/cmd_line_parser.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cassert>

#include "saturn/core/sys_console.hpp"

namespace saturn
{
namespace core
{

CommandLineParser* CommandLineParser::s_instance = nullptr;

bool CommandLineParser::initialize(const Config& _config) noexcept
{
    assert(s_instance == nullptr && "CommandLineParser already exists!");
    s_instance = new CommandLineParser();
    return true;
}

void CommandLineParser::shutdown() noexcept
{
    assert(s_instance != nullptr && "CommandLineParser does not exist!");
    delete s_instance;
    s_instance = nullptr;
}

std::optional<std::string> CommandLineParser::parseCommandLine(
    const std::vector<std::string>& _args)
{
    if (_args.size() <= 0)
    {
        return "Invalid argument count or empty args vector";
    }

    if (_args.size() > m_config.maxArguments)
    {
        return "Too many command line arguments";
    }

    reset();
    registerBuiltinOptions();

    m_args.clear();

    auto optionPairs = extractOptionArgumentsPairs(_args);

    for (const auto& pair : optionPairs)
    {
        auto result = handleOptionArgumentsPair(pair);
        if (result.has_value())
        {
            if (m_config.printErrors)
            {
                core::SystemConsole::printError("Error: " + result.value() + '\n');
            }

            return result;
        }
    }

    auto validationResult = validateRequiredOptions();
    if (validationResult.has_value())
    {
        if (m_config.printErrors)
        {
            core::SystemConsole::printError("Error: " + validationResult.value() + '\n');
        }

        return validationResult;
    }

    return std::nullopt;
}

std::optional<std::string> CommandLineParser::registerOption(
    const std::string& _name, const std::string& _shortName, const std::string& _description,
    const std::string& _help, CmdOptionValueTypes _valueType, bool _terminates,
    OptionHandler _handler, ValueValidator _validator, bool _required,
    const std::string& _defaultValue)
{
    if (_name.empty()) return "Option name cannot be empty";

    std::string longName = "--" + _name;
    std::string shortName = _shortName.empty() ? "" : "-" + _shortName;

    if (m_options.find(longName) != m_options.end())
    {
        return "Option '" + longName + "' already registered";
    }

    if (!shortName.empty() && m_shortToLong.find(shortName) != m_shortToLong.end())
    {
        return "Short option '" + shortName + "' already registered";
    }

    auto option = std::make_unique<Option>(_name, _shortName, _description, _help, _valueType,
                                           _terminates, std::move(_handler), std::move(_validator),
                                           _required, _defaultValue);

    m_options[longName] = std::move(option);

    if (!shortName.empty()) m_shortToLong[shortName] = longName;

    return std::nullopt;
}

bool CommandLineParser::unregisterOption(const std::string& _optionName)
{
    std::string normalizedName = normalizeOptionName(_optionName);

    auto it = m_options.find(normalizedName);
    if (it == m_options.end()) return false;

    if (!it->second->shortName.empty()) m_shortToLong.erase("-" + it->second->shortName);

    m_options.erase(it);

    return true;
}

void CommandLineParser::unregisterAllOptions()
{
    m_options.clear();
    m_shortToLong.clear();
}

void CommandLineParser::reset()
{
    m_args.clear();
    m_shouldTerminate = false;

    for (auto& [name, option] : m_options) option->parsed = false;
}

bool CommandLineParser::shouldTerminate() noexcept { return m_shouldTerminate; }

const std::vector<std::string> CommandLineParser::getArgs() noexcept { return m_args; }

size_t CommandLineParser::getArgsCount() noexcept { return m_args.size(); }

const std::string& CommandLineParser::getExecutableName() noexcept { return m_executableName; }

const std::string& CommandLineParser::getExecutablePath() noexcept { return m_executablePath; }

bool CommandLineParser::wasOptionParsed(const std::string& _optionName)
{
    const Option* option = findOption(normalizeOptionName(_optionName));
    return option && option->parsed;
}

std::string CommandLineParser::getHelpText()
{
    std::ostringstream oss;
    oss << "Usage: " << m_executableName << " [options]\n\nOptions:\n";

    for (const auto& [name, option] : m_options)
    {
        oss << "  " << name;
        if (!option->shortName.empty()) oss << ", -" << option->shortName;

        switch (option->valueType)
        {
            case CmdOptionValueTypes::single:
                oss << " <value>";
                break;
            case CmdOptionValueTypes::multiple:
                oss << " <value1> [value2 ...]";
                break;
            case CmdOptionValueTypes::optional:
                oss << " [value]";
                break;
            case CmdOptionValueTypes::none:
            default:
                break;
        }

        if (option->required) oss << " (required)";

        oss << "\n    " << option->help;

        if (!option->defaultValue.empty()) oss << " (default: " << option->defaultValue << ")";

        oss << "\n";
    }

    return oss.str();
}

std::vector<std::string> CommandLineParser::getRegisteredOptions()
{
    std::vector<std::string> options;
    options.reserve(m_options.size());

    for (const auto& [name, option] : m_options) options.push_back(name);

    std::sort(options.begin(), options.end());

    return options;
}

const CommandLineParser::Option* CommandLineParser::findOption(const std::string& _option)
{
    auto it = m_options.find(_option);
    if (it != m_options.end()) return it->second.get();

    // Check short name mapping
    auto shortIt = m_shortToLong.find(_option);
    if (shortIt != m_shortToLong.end())
    {
        auto longIt = m_options.find(shortIt->second);
        if (longIt != m_options.end()) return longIt->second.get();
    }

    return nullptr;
}

uint8_t CommandLineParser::calculateEditDistance(const std::string& _str1, const std::string& _str2)
{
    const size_t len1 = _str1.length();
    const size_t len2 = _str2.length();

    // Use single vector instead of 2D matrix for better cache performance
    std::vector<uint8_t> prev(len2 + 1);
    std::vector<uint8_t> curr(len2 + 1);

    // Initialize first row
    for (size_t j = 0; j <= len2; ++j) prev[j] = static_cast<uint8_t>(j);

    for (size_t i = 1; i <= len1; ++i)
    {
        curr[0] = static_cast<uint8_t>(i);

        for (size_t j = 1; j <= len2; ++j)
        {
            if (_str1[i - 1] == _str2[j - 1])
            {
                curr[j] = prev[j - 1];
            }
            else
            {
                curr[j] = std::min({
                    static_cast<uint8_t>(prev[j - 1] + 1), // substitution
                    static_cast<uint8_t>(curr[j - 1] + 1), // insertion
                    static_cast<uint8_t>(prev[j] + 1)      // deletion
                });
            }
        }

        prev.swap(curr);
    }

    return prev[len2];
}

std::string CommandLineParser::suggestCorrectOption(const std::string& _misspelledOption)
{
    if (!m_config.enableSpellCheck) return _misspelledOption;

    uint8_t minDistance = m_config.maxSpellCheckDistance;
    std::string bestMatch = _misspelledOption;

    for (const auto& [optionName, option] : m_options)
    {
        uint8_t distance = calculateEditDistance(_misspelledOption, optionName);
        if (distance < minDistance)
        {
            minDistance = distance;
            bestMatch = optionName;
        }

        if (!option->shortName.empty())
        {
            distance = calculateEditDistance(_misspelledOption, "-" + option->shortName);
            if (distance < minDistance)
            {
                minDistance = distance;
                bestMatch = "-" + option->shortName;
            }
        }
    }

    return (minDistance < m_config.maxSpellCheckDistance) ? bestMatch : _misspelledOption;
}

std::vector<std::pair<std::string, std::vector<std::string>>>
CommandLineParser::extractOptionArgumentsPairs(std::span<const std::string> _args)
{
    std::vector<std::pair<std::string, std::vector<std::string>>> pairs;

    for (size_t i = 0; i < _args.size(); ++i)
    {
        const std::string& arg = _args[i];

        if (arg.starts_with("--") || (arg.starts_with("-") && arg.length() > 1))
        {
            pairs.emplace_back(arg, std::vector<std::string>{});

            for (size_t j = i + 1; j < _args.size(); ++j)
            {
                const std::string& nextArg = _args[j];
                if (nextArg.starts_with("-")) break;

                pairs.back().second.push_back(nextArg);
            }

            i += pairs.back().second.size();
        }
        else
        {
            m_args.push_back(arg);
        }
    }

    return pairs;
}

std::optional<std::string> CommandLineParser::handleOptionArgumentsPair(
    const std::pair<std::string, std::vector<std::string>>& _pair)
{
    const std::string& optionName = _pair.first;
    const std::vector<std::string>& values = _pair.second;

    const Option* option = findOption(optionName);
    if (!option)
    {
        if (m_config.allowUnknownOptions) return std::nullopt;

        std::string suggestion = suggestCorrectOption(optionName);
        std::string message = "Unknown option '" + optionName + "'";

        if (suggestion != optionName && m_config.enableSpellCheck)
        {
            message += ". Did you mean '" + suggestion + "'?";
        }

        return message;
    }

    const_cast<Option*>(option)->parsed = true;

    if (option->terminates) m_shouldTerminate = true;

    switch (option->valueType)
    {
        case CmdOptionValueTypes::none:
        {
            if (!values.empty()) return "Option '" + optionName + "' does not accept arguments";
        }
        break;

        case CmdOptionValueTypes::single:
        {
            if (values.empty()) return "Option '" + optionName + "' requires exactly one argument";
            if (values.size() > 1) return "Option '" + optionName + "' accepts only one argument";
        }
        break;

        case CmdOptionValueTypes::multiple:
        {
            if (values.empty()) return "Option '" + optionName + "' requires at least one argument";
        }
        break;

        case CmdOptionValueTypes::optional:
            break;
    }

    std::vector<ArgumentValue> typedValues;

    if (values.empty() && !option->defaultValue.empty())
    {
        if (option->validator)
        {
            auto result = option->validator(option->defaultValue);
            if (!result.has_value()) return "Invalid default value for option '" + optionName + "'";

            typedValues.push_back(*result);
        }
        else
        {
            typedValues.emplace_back(option->defaultValue);
        }
    }
    else
    {
        for (const std::string& value : values)
        {
            if (option->validator)
            {
                auto result = option->validator(value);
                if (!result) return "Invalid value '" + value + "' for option '" + optionName + "'";

                typedValues.push_back(*result);
            }
            else
            {
                typedValues.emplace_back(value);
            }
        }
    }

    // No values for option value type 'none'

    if (option->handler && !option->handler(typedValues))
    {
        return "Handler failed for option '" + optionName + "'";
    }

    return std::nullopt;
}

void CommandLineParser::registerBuiltinOptions()
{
    if (!m_config.autoHelp) return;

    registerOption("help", "h", "Show help", "Display this help message and exit",
                   CmdOptionValueTypes::none, true,
                   [=](const std::vector<ArgumentValue>&) -> bool
                   {
                       core::SystemConsole::print(getHelpText());
                       return true;
                   });

    registerOption("version", "v", "Show version", "Display version information and exit",
                   CmdOptionValueTypes::none, true,
                   [=](const std::vector<ArgumentValue>&) -> bool
                   {
                       core::SystemConsole::print("Version information not available");
                       return true;
                   });
}

std::optional<std::string> CommandLineParser::validateRequiredOptions()
{
    for (const auto& [name, option] : m_options)
    {
        if (option->required && !option->parsed)
        {
            return "Required option '" + name + "' was not provided";
        }
    }

    return std::nullopt;
}

std::string CommandLineParser::normalizeOptionName(const std::string& _option)
{
    if (_option.starts_with("--"))
    {
        return _option;
    }
    else if (_option.starts_with("-") && _option.length() == 2)
    {
        return _option;
    }
    else if (_option.length() == 1)
    {
        return "-" + _option;
    }
    else
    {
        return "--" + _option;
    }
}

std::optional<ArgumentValue> CommandLineParser::validateString(const std::string& _value)
{
    return ArgumentValue{_value};
}

std::optional<ArgumentValue> CommandLineParser::validateInt(const std::string& _value)
{
    try
    {
        size_t pos;
        int result = std::stoi(_value, &pos);
        if (pos != _value.length()) return std::nullopt;

        return ArgumentValue{result};
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<ArgumentValue> CommandLineParser::validateFloat(const std::string& _value)
{
    try
    {
        size_t pos;
        float result = std::stof(_value, &pos);
        if (pos != _value.length()) return std::nullopt;

        return ArgumentValue{result};
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<ArgumentValue> CommandLineParser::validateBool(const std::string& _value)
{
    std::string lower = _value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
    {
        return ArgumentValue{true};
    }
    else if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
    {
        return ArgumentValue{false};
    }

    return std::nullopt;
}

} // namespace core
} // namespace saturn
