/**
 * @file PhysXCore.cpp
 * @brief Implementation of PhysXCore class
 */

#include "Core/PhysXCore.h"
#include <iostream>

namespace PhysXWrapper {

PhysXCore::PhysXCore()
    : m_foundation(nullptr)
    , m_physics(nullptr)
    , m_scene(nullptr)
    , m_dispatcher(nullptr)
    , m_pvd(nullptr)
    , m_pvdTransport(nullptr)
    , m_defaultMaterial(nullptr)
    , m_initialized(false)
{
}

PhysXCore::~PhysXCore()
{
    cleanup();
}

PhysXCore::PhysXCore(PhysXCore&& other) noexcept
    : m_allocator(std::move(other.m_allocator))
    , m_errorCallback(std::move(other.m_errorCallback))
    , m_foundation(other.m_foundation)
    , m_physics(other.m_physics)
    , m_scene(other.m_scene)
    , m_dispatcher(other.m_dispatcher)
    , m_pvd(other.m_pvd)
    , m_pvdTransport(other.m_pvdTransport)
    , m_defaultMaterial(other.m_defaultMaterial)
    , m_config(std::move(other.m_config))
    , m_initialized(other.m_initialized)
    , m_lastError(std::move(other.m_lastError))
{
    // Reset other's pointers
    other.m_foundation = nullptr;
    other.m_physics = nullptr;
    other.m_scene = nullptr;
    other.m_dispatcher = nullptr;
    other.m_pvd = nullptr;
    other.m_pvdTransport = nullptr;
    other.m_defaultMaterial = nullptr;
    other.m_initialized = false;
}

PhysXCore& PhysXCore::operator=(PhysXCore&& other) noexcept
{
    if (this != &other) {
        // Cleanup current resources
        cleanup();

        // Move from other
        m_allocator = std::move(other.m_allocator);
        m_errorCallback = std::move(other.m_errorCallback);
        m_foundation = other.m_foundation;
        m_physics = other.m_physics;
        m_scene = other.m_scene;
        m_dispatcher = other.m_dispatcher;
        m_pvd = other.m_pvd;
        m_pvdTransport = other.m_pvdTransport;
        m_defaultMaterial = other.m_defaultMaterial;
        m_config = std::move(other.m_config);
        m_initialized = other.m_initialized;
        m_lastError = std::move(other.m_lastError);

        // Reset other's pointers
        other.m_foundation = nullptr;
        other.m_physics = nullptr;
        other.m_scene = nullptr;
        other.m_dispatcher = nullptr;
        other.m_pvd = nullptr;
        other.m_pvdTransport = nullptr;
        other.m_defaultMaterial = nullptr;
        other.m_initialized = false;
    }
    return *this;
}

bool PhysXCore::initialize(const PhysXCoreConfig& config)
{
    if (m_initialized) {
        setError("PhysX already initialized");
        return false;
    }

    m_config = config;

    // Create Foundation
    m_foundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        m_allocator,
        m_errorCallback
    );

    if (!m_foundation) {
        setError("Failed to create PhysX Foundation");
        return false;
    }

    // Create PVD (Visual Debugger) if enabled
    if (config.enablePVD) {
        m_pvd = PxCreatePvd(*m_foundation);
        if (m_pvd) {
            m_pvdTransport = PxDefaultPvdSocketTransportCreate(
                config.pvdHost.c_str(),
                config.pvdPort,
                config.pvdTimeout
            );

            if (m_pvdTransport) {
                m_pvd->connect(*m_pvdTransport, PxPvdInstrumentationFlag::eALL);
            } else {
                setError("Warning: Failed to create PVD transport");
                // Continue anyway - PVD is optional
            }
        } else {
            setError("Warning: Failed to create PVD");
            // Continue anyway - PVD is optional
        }
    }

    // Create Physics
    PxTolerancesScale toleranceScale;
    toleranceScale.length = config.toleranceScale;
    toleranceScale.speed = config.toleranceScale;

    m_physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_foundation,
        toleranceScale,
        config.trackOutstandingAllocations,
        m_pvd
    );

    if (!m_physics) {
        setError("Failed to create PhysX Physics");
        cleanup();
        return false;
    }

    // Create CPU Dispatcher
    m_dispatcher = PxDefaultCpuDispatcherCreate(config.numThreads);
    if (!m_dispatcher) {
        setError("Failed to create CPU dispatcher");
        cleanup();
        return false;
    }

    // Create Scene
    PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
    sceneDesc.gravity = config.gravity;
    sceneDesc.cpuDispatcher = m_dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    m_scene = m_physics->createScene(sceneDesc);
    if (!m_scene) {
        setError("Failed to create scene");
        cleanup();
        return false;
    }

    // Configure PVD for scene if available
    if (m_pvd) {
        PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
        if (pvdClient) {
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    // Create default material
    m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.6f);
    if (!m_defaultMaterial) {
        setError("Failed to create default material");
        cleanup();
        return false;
    }

    m_initialized = true;
    m_lastError.clear();
    return true;
}

void PhysXCore::cleanup()
{
    if (!m_initialized) {
        return;
    }

    // Release in reverse order of creation
    PX_RELEASE(m_defaultMaterial);
    PX_RELEASE(m_scene);
    PX_RELEASE(m_dispatcher);
    PX_RELEASE(m_physics);

    if (m_pvd) {
        if (m_pvdTransport) {
            m_pvd->disconnect();
            PX_RELEASE(m_pvdTransport);
        }
        PX_RELEASE(m_pvd);
    }

    PX_RELEASE(m_foundation);

    m_initialized = false;
}

bool PhysXCore::update(float deltaTime)
{
    if (!m_initialized || !m_scene) {
        setError("PhysX not initialized");
        return false;
    }

    if (deltaTime <= 0.0f) {
        setError("Invalid delta time");
        return false;
    }

    // Simulate
    m_scene->simulate(deltaTime);

    // Fetch results (blocking)
    m_scene->fetchResults(true);

    return true;
}

PxMaterial* PhysXCore::createMaterial(float staticFriction, float dynamicFriction, float restitution)
{
    if (!m_initialized || !m_physics) {
        setError("PhysX not initialized");
        return nullptr;
    }

    return m_physics->createMaterial(staticFriction, dynamicFriction, restitution);
}

PxRigidDynamic* PhysXCore::createDynamic(
    const PxTransform& transform,
    const PxGeometry& geometry,
    float density,
    const PxVec3& velocity)
{
    if (!m_initialized || !m_physics || !m_scene || !m_defaultMaterial) {
        setError("PhysX not initialized");
        return nullptr;
    }

    // Create dynamic actor
    PxRigidDynamic* dynamic = PxCreateDynamic(
        *m_physics,
        transform,
        geometry,
        *m_defaultMaterial,
        density
    );

    if (!dynamic) {
        setError("Failed to create dynamic actor");
        return nullptr;
    }

    // Set velocity
    dynamic->setLinearVelocity(velocity);

    // Add some damping for stability
    dynamic->setAngularDamping(0.5f);

    // Add to scene
    m_scene->addActor(*dynamic);

    return dynamic;
}

PxRigidStatic* PhysXCore::createStatic(
    const PxTransform& transform,
    const PxGeometry& geometry)
{
    if (!m_initialized || !m_physics || !m_scene || !m_defaultMaterial) {
        setError("PhysX not initialized");
        return nullptr;
    }

    // Create static actor
    PxRigidStatic* staticActor = PxCreateStatic(
        *m_physics,
        transform,
        geometry,
        *m_defaultMaterial
    );

    if (!staticActor) {
        setError("Failed to create static actor");
        return nullptr;
    }

    // Add to scene
    m_scene->addActor(*staticActor);

    return staticActor;
}

PxRigidStatic* PhysXCore::createGroundPlane(const PxVec3& normal, float distance)
{
    if (!m_initialized || !m_physics || !m_scene || !m_defaultMaterial) {
        setError("PhysX not initialized");
        return nullptr;
    }

    // Create ground plane
    PxRigidStatic* groundPlane = PxCreatePlane(
        *m_physics,
        PxPlane(normal, distance),
        *m_defaultMaterial
    );

    if (!groundPlane) {
        setError("Failed to create ground plane");
        return nullptr;
    }

    // Add to scene
    m_scene->addActor(*groundPlane);

    return groundPlane;
}

void PhysXCore::setError(const std::string& error)
{
    m_lastError = error;
    std::cerr << "PhysXCore Error: " << error << std::endl;
}

} // namespace PhysXWrapper
