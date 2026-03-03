#pragma once
#include "unleash/Domain/toggleSet.hpp"



namespace unleash 
{

class IStorageProvider {
public: 
    virtual ~IStorageProvider() = default;
    virtual const std::optional<ToggleSet> get() = 0;
    virtual void save(const ToggleSet& t) = 0;
};

class LocalStorageProvider final : public IStorageProvider {
public:
    const std::optional<ToggleSet> get() override {
        return empty_;
    }

    void save(const ToggleSet&) override {
        // Nothing will happen! 
    }

private:
    std::optional<ToggleSet> empty_{};
};

}// namespace unleash