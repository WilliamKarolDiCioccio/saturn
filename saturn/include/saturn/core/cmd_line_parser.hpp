#pragma once

#include "saturn/defines.hpp"

#include "saturn/tools/logger.hpp"

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <span>
// Needed for type traits used in templates
#include <type_traits>

namespace saturn
{
namespace core
{

/**
 * @brief The command line option value types.
 */
enum class CmdOptionValueTypes : uint8_t
{
    none = 0,     // Flag options (no value)
    single = 1,   // Single value required
    multiple = 2, // Multiple values allowed
    optional = 3  // Single optional value
};

/**
 * @brief Supported argument value types
 */
using ArgumentValue = std::variant<std::string, int, float, bool>;

/**
 * @brief Handler function signature
 */
using OptionHandler = std::function<bool(const std::vector<ArgumentValue>&)>;

/**
 * @brief Value validator function signature
 */
using ValueValidator = std::function<std::optional<ArgumentValue>(const std::string&)>;

/**
 * @brief The command line parser class for managing and parsing command line arguments.
 */
class CommandLineParser
{
   public:
    /**
     * @brief Configuration for the parser behavior
     */
    struct Config
    {
        bool allowUnknownOptions;
        bool caseSensitive;
        bool enableSpellCheck;
        uint8_t maxSpellCheckDistance;
        bool printErrors;
        bool autoHelp;
        uint8_t maxArguments;

        Config() noexcept
            : allowUnknownOptions(false),
              caseSensitive(true),
              enableSpellCheck(true),
              maxSpellCheckDistance(3),
              printErrors(true),
              autoHelp(true),
              maxArguments(64) {};
    };

   private:
    struct Option
    {
        std::string name;
        std::string shortName;
        std::string description;
        std::string help;
        std::string defaultValue;
        CmdOptionValueTypes valueType;
        bool terminates;
        OptionHandler handler;
        ValueValidator validator;
        bool required;
        bool parsed;

        Option() = default;
        Option(std::string _name, std::string _shortName, std::string _description,
               std::string _help, CmdOptionValueTypes _valueType, bool _terminates,
               OptionHandler _handler, ValueValidator _validator = nullptr, bool _required = false,
               std::string _defaultValue = "")
            : name(std::move(_name)),
              shortName(std::move(_shortName)),
              description(std::move(_description)),
              help(std::move(_help)),
              defaultValue(std::move(_defaultValue)),
              valueType(_valueType),
              terminates(_terminates),
              handler(std::move(_handler)),
              validator(std::move(_validator)),
              required(_required),
              parsed(false) {};
    };

    SATURN_API static CommandLineParser* s_instance;

    Config m_config;
    std::string m_executableName;
    std::string m_executablePath;
    std::vector<std::string> m_args;
    bool m_shouldTerminate;
    std::unordered_map<std::string, std::unique_ptr<Option>> m_options;
    std::unordered_map<std::string, std::string> m_shortToLong;

   public:
    CommandLineParser()
        : m_executableName(""),
          m_executablePath(""),
          m_args(),
          m_shouldTerminate(false),
          m_options(),
          m_shortToLong() {};
    ~CommandLineParser() = default;

   public:
    CommandLineParser(const CommandLineParser&) = delete;
    CommandLineParser& operator=(const CommandLineParser&) = delete;

    CommandLineParser(CommandLineParser&&) = delete;
    CommandLineParser& operator=(CommandLineParser&&) = delete;

   public:
    SATURN_API static bool initialize(const Config& _config = Config()) noexcept;
    SATURN_API static void shutdown() noexcept;

    /**
     * @brief Parse command line arguments from vector
     *
     * @param _args Vector of argument strings
     * @return ParseResult indicating success or failure details
     */
    SATURN_API std::optional<std::string> parseCommandLine(const std::vector<std::string>& _args);

    /**
     * @brief Register a command line option with enhanced features
     *
     * @param _name Long name of the option (without --)
     * @param _shortName Single character short name (without -, can be empty)
     * @param _description Brief description
     * @param _help Detailed help text
     * @param _valueType Type of values this option accepts
     * @param _terminates Whether this option should terminate the program
     * @param _handler Function to handle the option values
     * @param _validator Optional value validator/converter
     * @param _required Whether this option is required
     * @param _defaultValue Default value if not provided
     * @return ParseResult indicating registration success
     */
    std::optional<std::string> registerOption(
        const std::string& _name, const std::string& _shortName, const std::string& _description,
        const std::string& _help, CmdOptionValueTypes _valueType, bool _terminates,
        OptionHandler _handler, ValueValidator _validator = nullptr, bool _required = false,
        const std::string& _defaultValue = "");

    /**
     * @brief Register option with common validators
     */
    template <typename T>
    std::optional<std::string> registerTypedOption(
        const std::string& _name, const std::string& _shortName, const std::string& _description,
        const std::string& _help, CmdOptionValueTypes _valueType,
        std::function<bool(const std::vector<T>&)> _handler, bool _required = false,
        const T& _defaultValue = T{});

    /**
     * @brief Unregister a command line option
     */
    SATURN_API bool unregisterOption(const std::string& _optionName);

    /**
     * @brief Unregister all options (including built-ins)
     */
    SATURN_API void unregisterAllOptions();

    /**
     * @brief Reset parser state for re-use
     */
    SATURN_API void reset();

    /**
     * @brief Set parser configuration
     */
    SATURN_API void setConfig(const Config& _config) { m_config = _config; }

    /**
     * @brief Get current configuration
     */
    SATURN_API const Config& getConfig() noexcept { return m_config; }

   public:
    SATURN_API [[nodiscard]] bool shouldTerminate() noexcept;

    SATURN_API [[nodiscard]] const std::vector<std::string> getArgs() noexcept;

    SATURN_API [[nodiscard]] size_t getArgsCount() noexcept;

    SATURN_API [[nodiscard]] const std::string& getExecutableName() noexcept;

    SATURN_API [[nodiscard]] const std::string& getExecutablePath() noexcept;

    /**
     * @brief Check if a specific option was parsed
     */
    SATURN_API [[nodiscard]] bool wasOptionParsed(const std::string& _optionName);

    /**
     * @brief Get help text for all registered options
     */
    SATURN_API [[nodiscard]] std::string getHelpText();

    /**
     * @brief Get list of all registered option names
     */
    SATURN_API [[nodiscard]] std::vector<std::string> getRegisteredOptions();

    [[nodiscard]] static inline CommandLineParser* getInstance() { return s_instance; }

   private:
    const Option* findOption(const std::string& _option);
    uint8_t calculateEditDistance(const std::string& _str1, const std::string& _str2);
    std::string suggestCorrectOption(const std::string& _misspelledOption);
    std::vector<std::pair<std::string, std::vector<std::string>>> extractOptionArgumentsPairs(
        std::span<const std::string> _args);
    std::optional<std::string> handleOptionArgumentsPair(
        const std::pair<std::string, std::vector<std::string>>& _pair);
    void registerBuiltinOptions();
    std::optional<std::string> validateRequiredOptions();
    std::string normalizeOptionName(const std::string& _option);

    std::optional<ArgumentValue> validateString(const std::string& _value);
    std::optional<ArgumentValue> validateInt(const std::string& _value);
    std::optional<ArgumentValue> validateFloat(const std::string& _value);
    std::optional<ArgumentValue> validateBool(const std::string& _value);
};

template <typename T>
std::optional<std::string> CommandLineParser::registerTypedOption(
    const std::string& _name, const std::string& _shortName, const std::string& _description,
    const std::string& _help, CmdOptionValueTypes _valueType,
    std::function<bool(const std::vector<T>&)> _handler, bool _required, const T& _defaultValue)
{
    ValueValidator validator;
    if constexpr (std::is_same_v<T, std::string>)
    {
        validator = [this](const std::string& v) { return this->validateString(v); };
    }
    else if constexpr (std::is_same_v<T, int>)
    {
        validator = [this](const std::string& v) { return this->validateInt(v); };
    }
    else if constexpr (std::is_same_v<T, float>)
    {
        validator = [this](const std::string& v) { return this->validateFloat(v); };
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        validator = [this](const std::string& v) { return this->validateBool(v); };
    }
    else
    {
        return "Unsupported type for typed option";
    }

    auto wrappedHandler = [_handler](const std::vector<ArgumentValue>& _values) -> bool
    {
        std::vector<T> typedValues;
        typedValues.reserve(_values.size());

        for (const auto& value : _values)
        {
            if (std::holds_alternative<T>(value))
            {
                typedValues.push_back(std::get<T>(value));
            }
            else
            {
                return false;
            }
        }

        return _handler(typedValues);
    };

    std::string defaultStr;
    if constexpr (std::is_convertible_v<T, std::string>)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            defaultStr = _defaultValue;
        }
        else
        {
            defaultStr = std::to_string(_defaultValue);
        }
    }

    return registerOption(_name, _shortName, _description, _help, _valueType, false, wrappedHandler,
                          validator, _required, defaultStr);
}

} // namespace core
} // namespace saturn
