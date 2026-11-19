#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include "zx/colors.hpp"
#include "zx/format.hpp"
#include "zx/image.hpp"
#include "zx/mat/math.hpp"
#include "zx/mat/polygonal_shape.hpp"
#include "zx/mat/spherical_shape.hpp"
#include "zx/mat/vector.hpp"
#include "zx/pipe.hpp"
#include "zx/raster.hpp"
#include "zx/result.hpp"
#include "zx/sequence.hpp"

template <class T, std::size_t N>
struct circular_buffer_t
{
    using value_type = T;
    using size_type = std::size_t;
    std::array<T, N> buffer{};
    size_type _head = 0;
    size_type _tail = 0;
    size_type _size = 0;

    constexpr size_type capacity() const noexcept { return N; }

    constexpr size_type size() const noexcept { return _size; }

    constexpr bool empty() const noexcept { return _size == 0; }

    void advance_head()
    {
        _head = (_head + 1) % capacity();
        if (_size < capacity())
        {
            ++_size;
        }
    }

    void push(const value_type& value)
    {
        buffer[_head] = value;
        advance_head();
    }

    void push(value_type&& value)
    {
        buffer[_head] = std::move(value);
        advance_head();
    }

    value_type pop()
    {
        if (_size == 0)
        {
            throw std::runtime_error("circular buffer is empty");
        }
        _tail = (_tail + 1) % capacity();
        --_size;
        return buffer[_tail];
    }

    void clear()
    {
        _head = 0;
        _tail = 0;
        _size = 0;
    }
};

struct worker_cancelled_t : std::exception
{
    const char* what() const noexcept override { return "operation cancelled"; }
};

// Go-style channel: thread-safe FIFO with blocking receive and close semantics.
// send to a closed channel returns false; receive on a closed, drained channel
// returns nullopt. A channel constructed with a max_size is bounded: send
// blocks while the queue is full (until a receiver makes room or the channel
// is closed), so the queue never grows beyond max_size.
template <class T>
class channel_t
{
public:
    static constexpr std::size_t unbounded = std::numeric_limits<std::size_t>::max();

    channel_t() = default;

    explicit channel_t(std::size_t max_size) : _max_size{ max_size } { }

    channel_t(const channel_t&) = delete;
    channel_t& operator=(const channel_t&) = delete;

    std::size_t max_size() const noexcept { return _max_size; }

    bool send(T value)
    {
        {
            std::unique_lock lock{ _mutex };
            _cv_not_full.wait(lock, [this] { return _queue.size() < _max_size || _closed; });
            if (_closed)
            {
                return false;
            }
            _queue.push_back(std::move(value));
        }
        _cv_not_empty.notify_one();
        return true;
    }

    // Non-blocking send: fails immediately when the channel is full or closed.
    bool try_send(T value)
    {
        {
            std::lock_guard lock{ _mutex };
            if (_closed || _queue.size() >= _max_size)
            {
                return false;
            }
            _queue.push_back(std::move(value));
        }
        _cv_not_empty.notify_one();
        return true;
    }

    std::optional<T> receive()
    {
        std::unique_lock lock{ _mutex };
        _cv_not_empty.wait(lock, [this] { return !_queue.empty() || _closed; });
        return pop_front();
    }

    template <class Rep, class Period>
    std::optional<T> receive_for(const std::chrono::duration<Rep, Period>& timeout)
    {
        std::unique_lock lock{ _mutex };
        _cv_not_empty.wait_for(lock, timeout, [this] { return !_queue.empty() || _closed; });
        return pop_front();
    }

    std::optional<T> try_receive()
    {
        std::lock_guard lock{ _mutex };
        return pop_front();
    }

    void close()
    {
        {
            std::lock_guard lock{ _mutex };
            _closed = true;
        }
        _cv_not_empty.notify_all();
        _cv_not_full.notify_all();
    }

private:
    // requires _mutex to be held
    std::optional<T> pop_front()
    {
        if (_queue.empty())
        {
            return std::nullopt;
        }
        std::optional<T> value{ std::move(_queue.front()) };
        _queue.pop_front();
        _cv_not_full.notify_one();
        return value;
    }

    std::mutex _mutex;
    std::condition_variable _cv_not_empty;
    std::condition_variable _cv_not_full;
    std::deque<T> _queue;
    std::size_t _max_size = unbounded;
    bool _closed = false;
};

// Central event loop: a single queue that marshals callbacks from producer
// threads onto whichever thread pumps it - the C# SynchronizationContext /
// UI message pump equivalent. Share one dispatcher between any number of
// background workers and pump it in one place.
class dispatcher_t
{
public:
    using task_type = std::function<void()>;

    void post(task_type task) { _tasks.send(std::move(task)); }

    // Runs all currently pending tasks on the calling thread; returns the
    // number of tasks executed.
    std::size_t poll()
    {
        std::size_t count = 0;
        while (auto task = _tasks.try_receive())
        {
            std::invoke(*task);
            ++count;
        }
        return count;
    }

    // Pumps tasks on the calling thread until the predicate becomes true.
    // The predicate is re-evaluated periodically, so it may also depend on
    // external conditions (timeouts), not only on effects of the tasks.
    template <class Pred, class Rep, class Period>
    void run_until(Pred pred, const std::chrono::duration<Rep, Period>& timeout)
    {
        while (!std::invoke(pred))
        {
            if (auto task = _tasks.receive_for(timeout))
            {
                std::invoke(*task);
            }
        }
    }

    template <class Pred>
    void run_until(Pred pred)
    {
        run_until(std::move(pred), std::chrono::milliseconds(10));
    }

private:
    channel_t<task_type> _tasks;
};

// C# BackgroundWorker analogue: run work on a background thread with cooperative
// cancellation and progress reporting. The worker thread only *posts* handler
// invocations to the shared dispatcher; they run on whichever thread pumps it.
// Posted events of a destroyed worker are skipped (liveness token), so a worker
// may be destroyed while its events are still queued.
template <class Result>
class background_worker_t
{
public:
    struct completed_event_t
    {
        std::optional<Result> result;
        std::exception_ptr error;
        bool cancelled = false;
    };

    // The work callable's view of the worker: cancellation and progress only,
    // no access to handlers or lifecycle.
    class context_t
    {
    public:
        context_t(const context_t&) = delete;
        context_t& operator=(const context_t&) = delete;

        bool cancellation_pending() const noexcept { return _worker.cancellation_pending(); }

        void throw_if_cancellation_pending() const
        {
            if (cancellation_pending())
            {
                throw worker_cancelled_t{};
            }
        }

        void report_progress(int percent) const { _worker.emit_progress(percent); }

    private:
        friend class background_worker_t;

        explicit context_t(const background_worker_t& worker) : _worker(worker) { }

        const background_worker_t& _worker;
    };

    using work_type = std::function<Result(const context_t&)>;
    using progress_changed_type = std::function<void(int)>;
    using completed_type = std::function<void(const completed_event_t&)>;

    explicit background_worker_t(dispatcher_t& dispatcher, work_type work) : _dispatcher(dispatcher), _work(std::move(work))
    {
    }

    background_worker_t(const background_worker_t&) = delete;
    background_worker_t& operator=(const background_worker_t&) = delete;

    ~background_worker_t()
    {
        cancel_async();
        wait();
    }

    background_worker_t& on_progress_changed(progress_changed_type handler)
    {
        _progress_changed = std::move(handler);
        return *this;
    }

    background_worker_t& on_completed(completed_type handler)
    {
        _completed = std::move(handler);
        return *this;
    }

    bool is_busy() const noexcept { return _busy.load(); }

    bool cancellation_pending() const noexcept { return _cancellation_pending.load(); }

    // Must not be called from within the completed handler (the worker thread
    // cannot join itself).
    void run_worker_async()
    {
        if (_busy.exchange(true))
        {
            throw std::logic_error("background worker is already running");
        }
        wait();
        _cancellation_pending.store(false);
        _thread = std::thread([this] { execute(); });
    }

    void cancel_async() noexcept { _cancellation_pending.store(true); }

    void wait()
    {
        if (_thread.joinable())
        {
            _thread.join();
        }
    }

private:
    // Wraps the task with a liveness token: if this worker is gone by the time
    // the dispatcher pumps the task, it is silently dropped.
    template <class F>
    void post(F task) const
    {
        _dispatcher.post(
            [token = std::weak_ptr<const void>(_alive), task = std::move(task)]
            {
                if (!token.expired())
                {
                    task();
                }
            });
    }

    void emit_progress(int percent) const
    {
        post(
            [this, percent]
            {
                if (_progress_changed)
                {
                    _progress_changed(percent);
                }
            });
    }

    void emit_completed(completed_event_t event)
    {
        post(
            [this, event = std::move(event)]
            {
                if (_completed)
                {
                    _completed(event);
                }
            });
    }

    void execute()
    {
        completed_event_t event{};
        try
        {
            event.result = _work(context_t{ *this });
        }
        catch (const worker_cancelled_t&)
        {
            event.cancelled = true;
        }
        catch (...)
        {
            event.error = std::current_exception();
        }
        _busy.store(false);
        emit_completed(std::move(event));
    }

    dispatcher_t& _dispatcher;
    work_type _work;
    progress_changed_type _progress_changed;
    completed_type _completed;
    std::thread _thread;
    std::shared_ptr<const void> _alive = std::make_shared<int>(0);
    std::atomic<bool> _busy{ false };
    std::atomic<bool> _cancellation_pending{ false };
};

template <class Clock = std::chrono::high_resolution_clock, class Func, class... Args>
auto time_it(Func&& func, Args&&... args) -> std::pair<std::invoke_result_t<Func, Args...>, typename Clock::duration>
{
    const auto start = Clock::now();
    const auto result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    const auto end = Clock::now();
    return { result, end - start };
}

template <class Clock = std::chrono::high_resolution_clock, class Func, class... Args>
auto try_time_it(Func&& func, Args&&... args)
    -> zx::result_t<std::pair<std::invoke_result_t<Func, Args...>, typename Clock::duration>, std::exception_ptr>
{
    auto [result, duration] = time_it<Clock>(zx::result::try_invoke, std::forward<Func>(func), std::forward<Args>(args)...);
    if (!result)
    {
        return zx::result::forward_error(std::move(result));
    }
    return std::pair{ *std::move(result), duration };
}

zx::mat::polygon_t<2, int> polygonize(const zx::mat::circle_t<int>& circle, std::size_t n)
{
    using namespace zx;
    static const auto lower = -mat::math::half_pi<float>;
    static const auto upper = lower + mat::math::two_pi<float>;
    return seq::linspace(lower, upper, n, false)                                                //
           |= seq::transform([](float i) { return zx::mat::from_polar(1.F, i); })               //
           |= seq::transform([&](const auto& v) { return circle.center + v * circle.radius; })  //
           |= seq::transform(zx::mat::to<int>);
}

int demo_background_worker()
{
    using namespace std::chrono_literals;
    using worker_type = background_worker_t<long long>;

    auto dispatcher = dispatcher_t{};

    const auto make_work = [](int steps, std::chrono::milliseconds delay)
    {
        return [=](const worker_type::context_t& ctx)
        {
            long long sum = 0;
            for (int i = 1; i <= steps; ++i)
            {
                ctx.throw_if_cancellation_pending();
                std::this_thread::sleep_for(delay);
                sum += i;
                if (i % (steps / 10) == 0)
                {
                    ctx.report_progress(i * 100 / steps);
                }
            }
            return sum;
        };
    };

    int completed_count = 0;
    const auto report_completed = [&completed_count](std::string_view name)
    {
        return [&completed_count, name](const worker_type::completed_event_t& event)
        {
            ++completed_count;
            if (event.cancelled)
            {
                zx::format_to(std::cout, name, ": cancelled\n");
            }
            else if (event.error)
            {
                zx::format_to(std::cout, name, ": failed\n");
            }
            else
            {
                zx::format_to(std::cout, name, ": completed, result: ", *event.result, "\n");
            }
        };
    };

    auto fast = worker_type{ dispatcher, make_work(50, 10ms) };
    auto slow = worker_type{ dispatcher, make_work(100, 20ms) };

    fast.on_progress_changed([](int percent) { zx::format_to(std::cout, "fast: ", percent, "%\n"); })
        .on_completed(report_completed("fast"));
    slow.on_progress_changed([](int percent) { zx::format_to(std::cout, "slow: ", percent, "%\n"); })
        .on_completed(report_completed("slow"));

    fast.run_worker_async();
    slow.run_worker_async();

    const auto cancel_at = std::chrono::steady_clock::now() + 1s;
    bool cancel_requested = false;
    // One central pump for all workers; every handler runs here, on this thread.
    dispatcher.run_until(
        [&]
        {
            if (!cancel_requested && std::chrono::steady_clock::now() >= cancel_at)
            {
                slow.cancel_async();
                cancel_requested = true;
            }
            return completed_count == 2;
        });
    return 0;
}

/*
make --preset ninja-release -DZX_BUILD_DEVLAB=ON
cmake --build --preset ninja-release --target zx_devlab
./build/ninja-release/devlab/zx_devlab
*/
int run(const std::vector<std::string_view>&)
{
    using namespace zx;

#if 1
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/lenna.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    const auto temp = background |= mat::with(
        [&](auto v)
        {
            mat::convolve(v, mat::kernel::median(mat::mask::square(5)));
            mat::convolve(v, mat::kernel::sobel());
        });

    const auto threshold = zx::mat::lookup_table::threshold(64.F);

    const auto shape = zx::mat::rasterize(
        temp.slice({ { 0, -10 }, { 0, -10 } }).bounds(),
        [&](const auto& loc) { return threshold(mat::filters::gray(temp[loc])[0]) > 0.F; });

    const auto result = background.view() |= mat::with(
        [&](auto v)
        {
            v |= mat::modify(mat::filters::sepia)
                |= mat::modify(mat::lookup_table::contrast(0.25F) * mat::lookup_table::brightness(-64.F));
            mat::draw(v, shape, mat::filters::solid(mat::true_color_t{ 255, 255, 255 }));
            mat::paste(
                v, conan |= mat::flip_horizontal(), mat::rgb_image_t::location_type{ 0, 100 }, mat::filters::blend(0.5F));

            // const auto circle = zx::mat::circle_t<int>{ { 300, 300 }, 150 };
            // mat::draw(v, circle, mat::filters::solid(mat::true_color_t{ 0, 255, 128 }));
            // mat::draw(v, polygonize(circle, 3), mat::filters::solid(mat::true_color_t{ 255, 0, 0 }));
            // mat::draw(v, polygonize(circle, 6), mat::filters::solid(mat::true_color_t{ 255, 255, 128 }));
        });

    mat::save_bitmap(result, mat::filepath_t{ "/home/krzysiek/out.bmp" });
#else
    return demo_background_worker();
#endif
    return 0;
}

struct exception_handler_t
{
    std::function<void(int, std::string_view)> handler;

    void operator()(std::exception_ptr ptr, int level = 0) const
    {
        if (!ptr)
        {
            return;
        }

        try
        {
            std::rethrow_exception(ptr);
        }
        catch (const std::exception& ex)
        {
            handler(level, ex.what());
            try
            {
                std::rethrow_if_nested(ex);
            }
            catch (...)
            {
                (*this)(std::current_exception(), level + 1);
            }
        }
        catch (const std::string& ex)
        {
            handler(level, ex);
        }
        catch (const char* ex)
        {
            handler(level, ex);
        }
        catch (...)
        {
            handler(level, "unknown exception");
        }
    }
};

int main(int argc, char* argv[])
{
    const auto handle_exception = exception_handler_t{ [](int level, std::string_view message)
                                                       { zx::format_to(std::cerr, std::string(level * 2, ' '), message); } };

    if (auto res = try_time_it(run, std::vector<std::string_view>(argv, argv + argc)))
    {
        const auto [result, duration] = *res;
        std::cerr << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms"
                  << std::endl;
        return result;
    }
    else
    {
        handle_exception(res.error());
        return -1;
    }
}
