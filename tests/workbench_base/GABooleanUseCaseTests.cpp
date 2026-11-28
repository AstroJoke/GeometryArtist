#include "GAArtifactStore.h"
#include "GABooleanUseCase.h"
#include "GADataStore.h"
#include "GAMeshDataObject.h"
#include "IGABooleanOperator.h"
#include "IGAGeometryResource.h"

#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>

namespace
{
enum class Scenario
{
    Success, MissingInput, EmptyOutput, UnknownOutput, BackendFailure, ImportThrows,
    ApplyThrows, ExportThrows, RejectPublication, PublicationThrows
};

struct InjectedFailure {};

class Backend : public IGAGeometryResource, public IGABooleanOperator
{
public:
    explicit Backend(Scenario scenario) : scenario_(scenario) {}

    GAGeometryHandle importGeometry(const GADataObject&) override
    {
        if (scenario_ == Scenario::ImportThrows && imports == 1)
            throw InjectedFailure{};
        return allocate(++imports);
    }

    GAGeometryResult apply(GABooleanOperation op, GAGeometryHandle left, GAGeometryHandle right,
                           const GAGeometryOptions&) override
    {
        if (op != GABooleanOperation::Difference || !isValid(left) || !isValid(right))
            throw std::logic_error("Expected two live inputs for difference");
        if (scenario_ == Scenario::ApplyThrows)
            throw InjectedFailure{};
        if (scenario_ == Scenario::EmptyOutput)
            return {GAGeometryStatus::Success, GAInvalidGeometryHandle};
        if (scenario_ == Scenario::UnknownOutput)
            return {GAGeometryStatus::Success, 99};
        if (scenario_ == Scenario::BackendFailure)
            return {GAGeometryStatus::Unsupported, allocate(3), "Unsupported operation"};
        return {GAGeometryStatus::Success, allocate(3)};
    }

    GADataHandle exportGeometry(GAGeometryHandle handle, const char* name, GADataStore& data) const override
    {
        if (!isValid(handle))
            throw std::logic_error("Export of unknown handle");
        if (scenario_ == Scenario::ExportThrows)
            throw InjectedFailure{};
        return data.registe(std::make_unique<GAMeshDataObject>(name));
    }

    void releaseGeometry(GAGeometryHandle handle) noexcept override
    {
        if (live.erase(handle) != 1)
            badRelease = true;
    }

    bool isValid(GAGeometryHandle handle) const noexcept override { return live.contains(handle); }
    GAGeometryRepresentation representation(GAGeometryHandle) const noexcept override
    {
        return GAGeometryRepresentation::TriangleMesh;
    }

    unsigned imports = 0;
    std::set<GAGeometryHandle> live;
    bool badRelease = false;

private:
    GAGeometryHandle allocate(GAGeometryHandle id)
    {
        live.insert(id);
        return id;
    }
    Scenario scenario_;
};

class Artifacts : public GAArtifactStore
{
public:
    Scenario publication = Scenario::Success;
    GAArtifactId create(GAArtifactType type, std::string_view name, std::string_view mediaType,
                        GADataHandle data) override
    {
        if (publication == Scenario::PublicationThrows)
            throw InjectedFailure{};
        if (publication == Scenario::RejectPublication)
            return GAInvalidArtifactId;
        return GAArtifactStore::create(type, name, mediaType, data);
    }
};

bool checkScenario(Scenario scenario)
{
    GADataStore data;
    Artifacts artifacts;
    Backend backend(scenario);
    const auto leftData = data.registe(std::make_unique<GAMeshDataObject>("left"));
    const auto rightData = data.registe(std::make_unique<GAMeshDataObject>("right"));
    const auto left = artifacts.create(GAArtifactType::Mesh, "left", "model/mesh", leftData);
    const auto right = artifacts.create(GAArtifactType::Mesh, "right", "model/mesh", rightData);
    artifacts.publication = scenario;
    GABooleanUseCase boolean(data, artifacts, backend, backend);
    GABooleanUseCaseResult result;
    bool threw = false;
    try
    {
        result = boolean.execute(GABooleanOperation::Difference, left,
                                 scenario == Scenario::MissingInput ? GAInvalidArtifactId : right);
    }
    catch (const InjectedFailure&)
    {
        threw = true;
    }
    if (!backend.live.empty() || backend.badRelease || !data.contains(leftData) || !data.contains(rightData) ||
        artifacts.dataHandle(left) != leftData || artifacts.dataHandle(right) != rightData)
        return false;
    if (scenario == Scenario::Success)
        return !threw && result.status == GAGeometryStatus::Success && result.artifact != left &&
               result.artifact != right && data.contains(artifacts.dataHandle(result.artifact)) &&
               data.size() == 3 && artifacts.size() == 3;
    if (result.artifact != GAInvalidArtifactId || data.size() != 2 || artifacts.size() != 2)
        return false;
    switch (scenario)
    {
    case Scenario::ImportThrows:
    case Scenario::ApplyThrows:
    case Scenario::ExportThrows:
    case Scenario::PublicationThrows:
        return threw;
    case Scenario::MissingInput:
        return !threw && backend.imports == 0 && result.status == GAGeometryStatus::InvalidArgument;
    case Scenario::BackendFailure:
        return !threw && result.status == GAGeometryStatus::Unsupported;
    default:
        return !threw && result.status == GAGeometryStatus::Failed;
    }
}
}

int main()
{
    bool passed = true;
    for (const auto scenario : {Scenario::Success, Scenario::MissingInput, Scenario::EmptyOutput, Scenario::UnknownOutput,
                                Scenario::BackendFailure, Scenario::ImportThrows, Scenario::ApplyThrows,
                                Scenario::ExportThrows, Scenario::RejectPublication, Scenario::PublicationThrows})
    {
        try
        {
            if (checkScenario(scenario))
                continue;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << "Boolean use case regression in scenario " << static_cast<int>(scenario) << '\n';
        passed = false;
    }
    return passed ? 0 : 1;
}
