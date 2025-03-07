#include "intent/ScriptAction.hpp"

#include <boost/assert.hpp>
#include <boost/process.hpp>
#include <jarvisto/core/Logger.hpp>

#include <chrono>

namespace fs = std::filesystem;
namespace pr = boost::process::v2;

namespace jar {

Action::Ptr
ScriptAction::create(std::filesystem::path exec,
                     Args args /*= {}*/,
                     std::filesystem::path home /*= {}*/,
                     Environment env /*= {}*/,
                     bool inheritParentEnv /*= false*/,
                     Timeout timeout /*= kDefaultTimeout*/)
{
    BOOST_ASSERT(not exec.empty());
    return Action::Ptr{new ScriptAction{std::move(exec),
                                        std::move(args),
                                        std::move(home),
                                        std::move(env),
                                        inheritParentEnv,
                                        timeout}};
}

ScriptAction::ScriptAction(std::filesystem::path exec,
                           Args args /*= {}*/,
                           std::filesystem::path home /*= {}*/,
                           Environment env /*= {}*/,
                           bool inheritParentEnv /*= false*/,
                           Timeout timeout /*= kDefaultTimeout*/)
    : _exec{std::move(exec)}
    , _args{std::move(args)}
    , _home{std::move(home)}
    , _env{std::move(env)}
    , _inheritParentEnv{inheritParentEnv}
    , _timeout{timeout}
{
}

ScriptAction::Ptr
ScriptAction::clone() const
{
    return Action::Ptr{new ScriptAction{_exec, _args, _home, _env, _inheritParentEnv, _timeout}};
}

void
ScriptAction::execute(io::any_io_executor executor)
{
    _executor = std::move(executor);

    io::post(_executor, [weakSelf = weak_from_this()]() {
        if (auto self = weakSelf.lock()) {
            self->run();
        }
    });
}

void
ScriptAction::run()
{
    _exec = fs::path{pr::environment::find_executable(boost::filesystem::path{_exec}).string()};
    if (_exec.empty()) {
        LOGE("Unable to locate program executable file");
        complete(std::make_error_code(std::errc::invalid_argument));
        return;
    }

    if (_home.empty()) {
        _home = fs::current_path();
        LOGD("Use <{}> path as home directory", _home);
    }

    std::unordered_map<pr::environment::key, pr::environment::value> env;
    if (_inheritParentEnv) {
        LOGD("Copy env from current process");
        for (const auto& keyValueView : pr::environment::current()) {
            env[keyValueView.key()].assign(keyValueView.value());
        }
    }
    if (not _env.empty()) {
        for (const auto& [name, value] : _env) {
            env[name].assign(value);
        }
    }

    auto onComplete = [weakSelf = weak_from_this()](sys::error_code ec, int exitCode) {
        if (auto self = weakSelf.lock()) {
            self->cancelTimer();
            if (ec) {
                LOGE("Unable to execute program");
                self->complete(ec);
            } else {
                LOGI("Program ended with <{}> exit code", exitCode);
                self->complete();
            }
        }
    };

    pr::async_execute(
        pr::process{
            _executor,
            boost::filesystem::path{_exec},
            _args,
            pr::process_environment{env},
            pr::process_start_dir{boost::filesystem::path{_home}},
            pr::process_stdio{nullptr, nullptr, nullptr},
        },
        io::bind_cancellation_slot(_runningSig.slot(), std::move(onComplete)));

    scheduleTimer();
}

void
ScriptAction::terminate()
{
    LOGI("Terminate program due to timeout");
    _runningSig.emit(io::cancellation_type::terminal);
}

void
ScriptAction::scheduleTimer()
{
    namespace krn = std::chrono;

    _runningTimer = std::make_unique<io::steady_timer>(_executor, _timeout);
    _runningTimer->async_wait([weakSelf = weak_from_this()](sys::error_code ec) {
        if (ec) {
            if (ec != io::error::operation_aborted) {
                LOGE("Unable to wait given timeout: error<{}>", ec.message());
            }
        } else {
            if (auto self = weakSelf.lock()) {
                self->terminate();
            }
        }
    });
}

void
ScriptAction::cancelTimer()
{
    if (_runningTimer) {
        _runningTimer->cancel();
    }
    _runningTimer.reset();
}

} // namespace jar
