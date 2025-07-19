#pragma once

#include <variant>
#include <functional>
#include <type_traits>
#include <stdexcept>

namespace pieces
{

template <typename T, template <typename...> class Template>
struct is_specialization_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type
{
};

template <typename T, template <typename...> class Template>
constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

// A helper struct to unwrap reference wrappers.
template <typename T>
struct UnwrapRef
{
    [[nodiscard]] static T& get(T& _value) { return _value; }
    [[nodiscard]] static T const& get(T const& v) { return v; }
};

// A specialization of UnwrapRef for std::reference_wrapper.
template <typename T>
struct UnwrapRef<std::reference_wrapper<T>>
{
    [[nodiscard]] static T& get(std::reference_wrapper<T>& _value) { return _value.get(); }
    [[nodiscard]] static T const& get(std::reference_wrapper<T> const& _value)
    {
        return _value.get();
    }
};

/**
 * @brief A result type that represents either a success (Ok) or a failure (Err).
 *
 * This is a basic implementation of Railway Oriented Programming for C++,
 * inspired by functional languages like F# and Rust.
 *
 * @tparam T The type for the success value.
 * @tparam E The type for the error value.
 */
template <typename T, typename E>
class Result final
{
   public:
    using SuccessType = T;
    using ErrorType = E;

   private:
    union Data
    {
        T success;
        E error;

        // Constructors are need to avoid implicitly deleted union
        Data() {}
        ~Data() {}
    } m_data;

    bool m_isError;

   private:
    /**
     * @brief Construct a new Result as a success or error.
     *
     * @tparam N The index of the variant to construct.
     * @tparam Args The types of the constructor arguments.
     * @param args The constructor arguments.
     */
    template <size_t N, typename... Args>
    explicit Result(std::in_place_index_t<N>, Args&&... args)
    {
        if constexpr (N == 0)
        {
            new (&m_data.success) T(std::forward<Args>(args)...);
            m_isError = false;
        }
        else if constexpr (N == 1)
        {
            new (&m_data.error) E(std::forward<Args>(args)...);
            m_isError = true;
        }
        else
        {
            static_assert(N < 2, "Result only holds index 0 or 1");
        }
    }

   public:
    // Constructs a success result.
    static Result Ok(T value) { return Result(std::in_place_index<0>, std::move(value)); }

    // Constructs an error result.
    static Result Err(E error) { return Result(std::in_place_index<1>, std::move(error)); }

    ~Result()
    {
        if (m_isError)
        {
            m_data.error.~E();
        }
        else
        {
            m_data.success.~T();
        }
    }

    // Checks if the result is a success.
    [[nodiscard]] bool isOk() const { return m_isError == false; }

    // Checks if the result is an error.
    [[nodiscard]] bool isErr() const { return !isOk(); }

    // Gets the success value, throwing if it is an error.
    [[nodiscard]] auto unwrap() -> decltype(UnwrapRef<T>::get(m_data.success))
    {
        if (isErr()) throw std::runtime_error("Attempted to access success from an error result");

        return UnwrapRef<T>::get(m_data.success);
    }

    // Gets the error value, throwing if it is a success.
    [[nodiscard]] auto error() -> decltype(UnwrapRef<E>::get(m_data.error))
    {
        if (isOk()) throw std::runtime_error("Attempted to access error from a success result");

        return UnwrapRef<E>::get(m_data.error);
    }

    /**
     * @brief Applies a function to the success value if the result is a success conserving the
     * error type.
     *
     * @tparam F The type of the function to apply.
     * @param f The function to apply to the success value.
     * @return std::invoke_result_t<F, T>
     */
    template <typename F>
        requires std::is_invocable_v<F, T> &&
                 is_specialization_of_v<std::invoke_result_t<F, T>, Result> &&
                 std::is_same_v<typename std::invoke_result_t<F, T>::ErrorType, E>
    [[nodiscard]] auto andThen(F&& f) -> std::invoke_result_t<F, T>
    {
        using Return = std::invoke_result_t<F, T>;

        if (isOk())
        {
            return std::invoke(std::forward<F>(f), m_data.success);
        }
        else
        {
            return Return::Err(m_data.error);
        }
    }

    /**
     * @brief Applies a function to the error value if the result is an error conserving the
     * success type.
     *
     * @tparam F The type of the function to apply.
     * @param f The function to apply to the error value.
     * @return std::invoke_result_t<F, E>
     */
    template <typename F>
        requires std::is_invocable_v<F, E> &&
                 is_specialization_of_v<std::invoke_result_t<F, E>, Result> &&
                 std::is_same_v<typename std::invoke_result_t<F, E>::SuccessType, T>
    [[nodiscard]] auto orElse(F&& f) -> std::invoke_result_t<F, E>
    {
        using Return = std::invoke_result_t<F, E>;

        if (isErr())
        {
            return std::invoke(std::forward<F>(f), m_data.error);
        }
        else
        {
            return Return::Ok(m_data.success);
        }
    }
};

// A Result constructor for success values.
template <typename U, typename E>
[[nodiscard]] Result<U, E> Ok(U&& _u)
{
    return Result<U, E>::Ok(std::forward<U>(_u));
}

// A Result constructor for error values.
template <typename U, typename E>
[[nodiscard]] Result<U, E> Err(E&& _e)
{
    return Result<U, E>::Err(std::forward<E>(_e));
}

// A specialization of Result for reference types.
template <typename U, typename E>
using RefResult = Result<std::reference_wrapper<U>, E>;

// A RefResult constructor for success values.
template <typename U, typename E>
[[nodiscard]] RefResult<U, E> OkRef(U& _u)
{
    return RefResult<U, E>::Ok(std::ref(_u));
}

// A RefResult constructor for error values.
template <typename U, typename E>
[[nodiscard]] RefResult<U, E> ErrRef(E&& _e)
{
    return RefResult<U, E>::Err(_e);
}

} // namespace pieces
