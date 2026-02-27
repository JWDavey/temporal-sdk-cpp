#pragma once

/// @file Activity definition and registration types.

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <temporalio/async_/task.h>

namespace temporalio::activities {

/// Definition of a registered activity including its name and executor.
///
/// Registration examples:
///   // Free function
///   auto def = ActivityDefinition::create("greet", &greet_function);
///   // Lambda
///   auto def = ActivityDefinition::create("double", [](int x) -> Task<int> {
///       co_return x * 2; });
///   // Member function + instance
///   auto def = ActivityDefinition::create("method", &MyClass::method, &obj);
class ActivityDefinition {
public:
    /// Create from a free function or static method returning Task<R>.
    template <typename R, typename... Args>
    static std::shared_ptr<ActivityDefinition> create(
        const std::string& name,
        async_::Task<R> (*func)(Args...)) {
        static_assert(sizeof...(Args) <= 1,
            "Multiple arguments not yet supported; use a single struct parameter");
        auto def = std::make_shared<ActivityDefinition>();
        def->name_ = name;
        def->executor_ = [func](std::vector<std::any> args)
            -> async_::Task<std::any> {
            if constexpr (sizeof...(Args) == 0) {
                if constexpr (std::is_void_v<R>) {
                    co_await func();
                    co_return std::any{};
                } else {
                    auto result = co_await func();
                    co_return std::any(std::move(result));
                }
            } else if constexpr (sizeof...(Args) == 1) {
                using Arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
                auto arg0 = std::any_cast<Arg0>(args.at(0));
                if constexpr (std::is_void_v<R>) {
                    co_await func(std::move(arg0));
                    co_return std::any{};
                } else {
                    auto result = co_await func(std::move(arg0));
                    co_return std::any(std::move(result));
                }
            }
        };
        return def;
    }

    /// Create from a callable (lambda, std::function, functor).
    /// Note: This overload currently only supports callables that take zero
    /// arguments. For callables with arguments, use the free function or
    /// member function overloads instead.
    template <typename Callable>
    static std::shared_ptr<ActivityDefinition> create(
        const std::string& name, Callable&& callable) {
        static_assert(std::is_invocable_v<std::decay_t<Callable>>,
            "Callable overload only supports zero-argument callables. "
            "For callables with arguments, use the free function or "
            "member function overloads.");
        auto def = std::make_shared<ActivityDefinition>();
        def->name_ = name;
        auto stored = std::make_shared<std::decay_t<Callable>>(
            std::forward<Callable>(callable));
        def->executor_ = [stored](std::vector<std::any> /*args*/)
            -> async_::Task<std::any> {
            auto result = co_await (*stored)();
            co_return std::any(std::move(result));
        };
        return def;
    }

    /// Create from a member function + instance pointer.
    /// IMPORTANT: The caller must ensure that `instance` outlives the
    /// returned ActivityDefinition and any tasks it produces. The definition
    /// captures a raw pointer; destroying the instance while activity tasks
    /// are in flight is undefined behavior.
    template <typename T, typename R, typename... Args>
    static std::shared_ptr<ActivityDefinition> create(
        const std::string& name,
        async_::Task<R> (T::*method)(Args...),
        T* instance) {
        static_assert(sizeof...(Args) <= 1,
            "Multiple arguments not yet supported; use a single struct parameter");
        auto def = std::make_shared<ActivityDefinition>();
        def->name_ = name;
        def->executor_ = [method, instance](std::vector<std::any> args)
            -> async_::Task<std::any> {
            if constexpr (sizeof...(Args) == 0) {
                if constexpr (std::is_void_v<R>) {
                    co_await (instance->*method)();
                    co_return std::any{};
                } else {
                    auto result = co_await (instance->*method)();
                    co_return std::any(std::move(result));
                }
            } else if constexpr (sizeof...(Args) == 1) {
                using Arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
                auto arg0 = std::any_cast<Arg0>(args.at(0));
                if constexpr (std::is_void_v<R>) {
                    co_await (instance->*method)(std::move(arg0));
                    co_return std::any{};
                } else {
                    auto result =
                        co_await (instance->*method)(std::move(arg0));
                    co_return std::any(std::move(result));
                }
            }
        };
        return def;
    }

    /// The activity name (empty for dynamic activities).
    const std::string& name() const noexcept { return name_; }

    /// Whether this is a dynamic activity.
    bool is_dynamic() const noexcept { return name_.empty(); }

    /// Execute the activity with the given arguments.
    async_::Task<std::any> execute(std::vector<std::any> args) const {
        return executor_(std::move(args));
    }

private:
    std::string name_;
    std::function<async_::Task<std::any>(std::vector<std::any>)> executor_;
};

}  // namespace temporalio::activities
