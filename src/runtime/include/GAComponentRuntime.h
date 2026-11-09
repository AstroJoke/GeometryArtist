#pragma once

#include "GAComponentTypes.h"
#include "IGAObject.h"

#include <cstddef>
#include <memory>
#include <string_view>

/**
 * @brief Abstract factory that creates and destroys one component type.
 *
 * Destruction is routed back through the factory so an object allocated inside
 * a plugin is released by the same binary and allocator.
 *
 */
class IGAComponentFactory
{
public:
    virtual ~IGAComponentFactory() = default;

    [[nodiscard]] virtual GAComponentDescriptor descriptor() const noexcept = 0;
    [[nodiscard]] virtual IGAObject* create() = 0;
    virtual void destroy(IGAObject* object) noexcept = 0;
};

/**
 * @brief Discovery port for component factories keyed by stable type name.
 *
 */
class IGAComponentRegistry
{
public:
    virtual ~IGAComponentRegistry() = default;

    virtual bool registerFactory(std::shared_ptr<IGAComponentFactory> factory) = 0;
    virtual bool unregisterFactory(std::string_view typeName) = 0;
    [[nodiscard]] virtual std::shared_ptr<IGAComponentFactory> find(std::string_view typeName) const = 0;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    virtual void clear() noexcept = 0;
};

/**
 * @brief Port for contributing factories from a dynamically loaded component.
 *
 */
class IGAComponentPluginLoader
{
public:
    virtual ~IGAComponentPluginLoader() = default;

    virtual bool load(std::string_view filePath, IGAComponentRegistry& registry) = 0;
    virtual void unload(IGAComponentRegistry& registry) noexcept = 0;
};

/// Thread-safe default implementation of IGAComponentRegistry.
class GAComponentRegistry : public IGAComponentRegistry
{
public:
    GAComponentRegistry();
    ~GAComponentRegistry();

    GAComponentRegistry(const GAComponentRegistry&) = delete;
    GAComponentRegistry& operator=(const GAComponentRegistry&) = delete;

    bool registerFactory(std::shared_ptr<IGAComponentFactory> factory) override;
    bool unregisterFactory(std::string_view typeName) override;
    [[nodiscard]] std::shared_ptr<IGAComponentFactory> find(std::string_view typeName) const override;
    [[nodiscard]] std::size_t size() const noexcept override;
    void clear() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
