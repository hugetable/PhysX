# PhysX 5.x API Compatibility Issues

This document tracks API compatibility issues between PhysXWrapper and PhysX 5.x.

## Summary

PhysXWrapper was originally written for PhysX 4.x and contains numerous API incompatibilities with PhysX 5.x. These issues need to be addressed systematically.

## Critical API Changes

### 1. PxScene API Changes
- `PxScene::getSceneDesc()` - **REMOVED** in PhysX 5.x
  - Used in: ContactModifier.cpp (lines 123, 137)
  - Solution: Store scene descriptor during creation

- `PxScene::getAggregate(PxAggregate&)` - **REMOVED** in PhysX 5.x
  - Used in: AggregateManager.cpp (lines 136, 150, 243)
  - Replacement: Use `PxScene::getAggregates()` instead

- `PxScene::removeAggregate()` - **Changed return type** from bool to void
  - Used in: AggregateManager.cpp (line 234)
  - Solution: Check aggregate presence before removal

### 2. PxPhysics API Changes
- `PxPhysics::createAggregate(maxActors, enableSelfCollision)` - **Signature changed**
  - New signature: `createAggregate(PxU32 maxActor, PxU32 maxShape, PxAggregateFilterHint filterHint)`
  - Used in: AggregateManager.cpp (line 101)
  - Solution: Calculate or estimate maxShape from maxActors

### 3. PxArticulationReducedCoordinate API Changes
- `setMaxDepenetrationVelocity()` - **REMOVED** in PhysX 5.x
  - Used in: ArticulationManager.cpp (line 641)
  - Solution: This setting may have moved or been removed

### 4. PxContactModifyPair API Changes
- `actor` field changed from `PxRigidActor*` to `const PxRigidActor*`
- `shape` field changed from `PxShape*` to `const PxShape*`
  - Used in: ContactModifier.cpp (lines 255, 376-379)
  - Solution: Update function signatures to accept const pointers

### 5. PxJoint API Changes
- `getConcreteType()` returns `PxType` instead of `PxJointConcreteType::Enum`
  - Used in: DebugDrawer.cpp (line 197)
  - Solution: Cast to PxJointConcreteType::Enum

### 6. GPU/CUDA Features
Several GPU-related APIs have changed or been removed:

- `PX_EXT_PINNED_MEMORY_FREE` → `PX_PINNED_HOST_FREE`
  - Used in: DeformableVolumeManager.cpp (lines 123, 126, 129, 132, 430, 433, 436, 439)

- `PxDeformableVolumeExt::copyToHost()` - **REMOVED/CHANGED**
  - Used in: DeformableVolumeManager.cpp (line 457)

**NOTE**: Deformable volumes and PBD features may require GPU support enabled during build.

### 7. Vector Math
- No `operator/` for PxVec3 / PxVec3 division
  - Used in: PointDistanceQuery.cpp (line 450)
  - Solution: Perform component-wise division manually

## Compilation Status

### Successfully Compiled
- ✅ PhysXCore
- ✅ ArticulationManager (partial)
- ✅ CharacterController
- ✅ Query/GeometryQuery (fixed)
- ✅ Query/FrustumQuery
- ✅ Query/PointDistanceQuery (needs fix)
- ✅ RigidBody/RigidBodyCCD
- ✅ RigidBody/RigidBodyContactHandler
- ✅ RigidBody/RigidBodyTrigger

### Failed Compilation
- ❌ Debug/DebugDrawer (minor API change)
- ❌ RigidBody/AggregateManager (API changes)
- ❌ RigidBody/ContactModifier (API changes)
- ❌ Deformable/DeformableVolumeManager (GPU-specific, may need GPU build)

## Recommended Actions

### Short Term (Get Basic Build Working)
1. **Disable GPU features**: Set `PHYSXWRAPPER_ENABLE_GPU=OFF` (already done)
2. **Fix critical API changes**:
   - Update createAggregate() calls
   - Fix const correctness in ContactModifier
   - Fix vector division in PointDistanceQuery
   - Update aggregate scene query methods

### Medium Term (Full Compatibility)
1. **Update all API calls** to PhysX 5.x equivalents
2. **Add compile-time feature detection** for GPU vs CPU builds
3. **Create compatibility layer** for removed APIs
4. **Add PhysX version detection** macros

### Long Term (Best Practices)
1. **Version-specific code paths** using preprocessor directives
2. **Automated API compatibility tests**
3. **Documentation of PhysX version requirements**

## Quick Fixes Applied

1. ✅ Added `#include <string>` to DebugDrawer.h
2. ✅ Added `#include <string>` to JointManager.cpp
3. ✅ Fixed `pxHit.uv` → `PxVec2(pxHit.u, pxHit.v)` in GeometryQuery.cpp
4. ✅ Removed `PxQueryFlag::eMULTIPLE` (deprecated) in GeometryQuery.cpp
5. ✅ Fixed GyroscopicForces default initializer using static defaultConfig()
6. ✅ Fixed AggregateManager default initializer using static defaultConfig()

## Remaining Issues

See compilation errors above for detailed list of remaining incompatibilities.

---
**Last Updated**: 2025-11-06
**PhysX Version**: 5.x (built from source with GCC)
**Compiler**: GCC 13.3.0
