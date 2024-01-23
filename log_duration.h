#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X,cerr, Y) X##Y
#define PROFILE_CONCAT(X, cerr, Y) PROFILE_CONCAT_INTERNAL(X, cerr, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, cerr, __LINE__)
#define  LOG_DURATION_STREAM(x, cerr) LogDuration UNIQUE_VAR_NAME_PROFILE(x, cerr)

class LogDuration {
public:
    // ������� ��� ���� std::chrono::steady_clock
    // � ������� using ��� ��������
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id, std::ostream& cout = std::cerr)
        : id_(id), cout_(cout) {
        
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        std::cerr << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& cout_;
};