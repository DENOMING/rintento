#pragma once

#include <jarvisto/Application.hpp>

namespace jar {

class Config;

class ServiceApplication final : public Application {
public:
    ServiceApplication() = default;

    [[nodiscard]] const char*
    name() const final;

private:
    void
    initialize(Application& application) final;
};

} // namespace jar