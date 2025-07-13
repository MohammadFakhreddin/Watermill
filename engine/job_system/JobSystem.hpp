#pragma once

#include "ThreadPool.hpp"

#include <future>

namespace MFA
{
    class JobSystem
    {
    public:

        static std::shared_ptr<JobSystem> Instance(bool createNewIfNotExists = false);

        [[nodiscard]] static bool HasInstance();

        explicit JobSystem();

        ~JobSystem();

        // I think callback system might be a better choice sometimes
        std::future<void> AssignTask(std::function<void()> task)
        {
            struct Params
            {
                std::promise<void> promise{};
            };
            auto params = std::make_shared<Params>();

            threadPool.AssignTask(
            [task, params]()
            {
                task();
                params->promise.set_value();
            });
            return params->promise.get_future();
        }

        template <typename T>
        std::future<T> AssignTask(std::function<T()> task)
        {
            std::promise<T> promise{};
            struct Params
            {
                std::promise<T> promise{};
            };
            auto params = std::make_shared<Params>();

            threadPool.AssignTask([task, params]() { params->promise.set_value(task()); });
            return params->promise.get_future();
        }

        [[nodiscard]]
        auto NumberOfAvailableThreads() const
        {
            return threadPool.NumberOfAvailableThreads();
        }

        [[nodiscard]]
        auto IsMainThread() const
        {
            return threadPool.IsMainThread();
        }

        // TODO: Add run on main thread!

    private:

		inline static std::weak_ptr<JobSystem> _instance {};

        ThreadPool threadPool{};

    };
} // namespace MFA


namespace MFA
{
    using JS = JobSystem;
}
