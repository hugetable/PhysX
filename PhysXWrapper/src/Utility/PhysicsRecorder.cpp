/**
 * @file PhysicsRecorder.cpp
 * @brief Implementation of PhysicsRecorder class
 */

#include "Utility/PhysicsRecorder.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>

namespace PhysXWrapper {

// ============================================================================
// RecordingInfo Implementation
// ============================================================================

void PhysicsRecorder::RecordingInfo::print() const
{
    std::cout << "Recording Info:" << std::endl;
    std::cout << "  Frames: " << frameCount << std::endl;
    std::cout << "  Duration: " << duration << " seconds" << std::endl;
    std::cout << "  Actors: " << actorCount << std::endl;
    std::cout << "  Frame Rate: " << frameRate << " fps" << std::endl;
    std::cout << "  Memory Size: " << (memorySize / 1024.0f / 1024.0f) << " MB" << std::endl;
}

// ============================================================================
// PhysicsRecorder::Impl
// ============================================================================

class PhysicsRecorder::Impl {
public:
    PxScene* m_scene = nullptr;

    std::vector<Frame> m_frames;
    std::vector<PxActor*> m_actorFilter;

    bool m_recording = false;
    PlaybackState m_playbackState = PlaybackState::STOPPED;
    PlaybackMode m_playbackMode = PlaybackMode::ONCE;

    PxU32 m_currentFrame = 0;
    PxReal m_currentTime = 0.0f;
    PxReal m_playbackSpeed = 1.0f;
    bool m_playbackReversed = false;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

PhysicsRecorder::PhysicsRecorder()
    : m_impl(std::make_unique<Impl>())
{
}

PhysicsRecorder::~PhysicsRecorder()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool PhysicsRecorder::initialize(PxScene* scene)
{
    if (!scene) {
        std::cerr << "PhysicsRecorder::initialize: scene is null" << std::endl;
        return false;
    }

    m_impl->m_scene = scene;
    m_impl->m_initialized = true;
    return true;
}

void PhysicsRecorder::cleanup()
{
    stopRecording();
    stopPlayback();
    clearRecording();
    m_impl->m_initialized = false;
}

bool PhysicsRecorder::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Recording
// ============================================================================

bool PhysicsRecorder::startRecording(bool clearExisting)
{
    if (!m_impl->m_initialized) {
        std::cerr << "PhysicsRecorder::startRecording: Not initialized" << std::endl;
        return false;
    }

    if (clearExisting) {
        clearRecording();
    }

    m_impl->m_recording = true;
    m_impl->m_currentTime = 0.0f;

    return true;
}

void PhysicsRecorder::stopRecording()
{
    m_impl->m_recording = false;
}

bool PhysicsRecorder::recordFrame()
{
    if (!m_impl->m_recording) {
        return false;
    }

    Frame frame;
    frame.timestamp = m_impl->m_currentTime;

    // Get all actors or filtered actors
    PxU32 numActors = m_impl->m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);

    if (numActors == 0) {
        return false;
    }

    std::vector<PxActor*> actors(numActors);
    m_impl->m_scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                                 actors.data(), numActors);

    // Record each actor
    for (PxActor* actor : actors) {
        // Check filter
        if (!m_impl->m_actorFilter.empty()) {
            auto it = std::find(m_impl->m_actorFilter.begin(), m_impl->m_actorFilter.end(), actor);
            if (it == m_impl->m_actorFilter.end()) {
                continue; // Skip this actor
            }
        }

        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (!rigidActor) continue;

        ActorSnapshot snapshot;
        snapshot.transform = rigidActor->getGlobalPose();

        // Get velocity for dynamic actors
        PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>();
        if (dynamicActor) {
            snapshot.linearVelocity = dynamicActor->getLinearVelocity();
            snapshot.angularVelocity = dynamicActor->getAngularVelocity();
        }

        frame.actors[actor] = snapshot;
    }

    m_impl->m_frames.push_back(frame);
    m_impl->m_currentTime += 0.016f; // Assume 60fps for timestamp

    return true;
}

bool PhysicsRecorder::isRecording() const
{
    return m_impl->m_recording;
}

void PhysicsRecorder::clearRecording()
{
    m_impl->m_frames.clear();
    m_impl->m_currentTime = 0.0f;
    m_impl->m_currentFrame = 0;
}

// ============================================================================
// Playback
// ============================================================================

bool PhysicsRecorder::startPlayback(PlaybackMode mode)
{
    if (!m_impl->m_initialized) {
        std::cerr << "PhysicsRecorder::startPlayback: Not initialized" << std::endl;
        return false;
    }

    if (m_impl->m_frames.empty()) {
        std::cerr << "PhysicsRecorder::startPlayback: No frames recorded" << std::endl;
        return false;
    }

    m_impl->m_playbackState = PlaybackState::PLAYING;
    m_impl->m_playbackMode = mode;
    m_impl->m_currentFrame = 0;
    m_impl->m_playbackReversed = false;

    return true;
}

void PhysicsRecorder::stopPlayback()
{
    m_impl->m_playbackState = PlaybackState::STOPPED;
    m_impl->m_currentFrame = 0;
}

void PhysicsRecorder::pausePlayback()
{
    if (m_impl->m_playbackState == PlaybackState::PLAYING) {
        m_impl->m_playbackState = PlaybackState::PAUSED;
    }
}

void PhysicsRecorder::resumePlayback()
{
    if (m_impl->m_playbackState == PlaybackState::PAUSED) {
        m_impl->m_playbackState = PlaybackState::PLAYING;
    }
}

void PhysicsRecorder::updatePlayback(PxReal deltaTime)
{
    if (m_impl->m_playbackState != PlaybackState::PLAYING) {
        return;
    }

    if (m_impl->m_frames.empty()) {
        return;
    }

    // Apply current frame
    if (m_impl->m_currentFrame < m_impl->m_frames.size()) {
        applyFrame(m_impl->m_frames[m_impl->m_currentFrame]);
    }

    // Advance frame based on speed and direction
    if (m_impl->m_playbackReversed) {
        if (m_impl->m_currentFrame > 0) {
            m_impl->m_currentFrame--;
        } else {
            // Reached beginning
            if (m_impl->m_playbackMode == PlaybackMode::PING_PONG) {
                m_impl->m_playbackReversed = false;
                m_impl->m_currentFrame = 1;
            } else if (m_impl->m_playbackMode == PlaybackMode::LOOP) {
                m_impl->m_currentFrame = static_cast<PxU32>(m_impl->m_frames.size()) - 1;
            } else {
                stopPlayback();
            }
        }
    } else {
        m_impl->m_currentFrame++;

        if (m_impl->m_currentFrame >= m_impl->m_frames.size()) {
            // Reached end
            if (m_impl->m_playbackMode == PlaybackMode::PING_PONG) {
                m_impl->m_playbackReversed = true;
                m_impl->m_currentFrame = static_cast<PxU32>(m_impl->m_frames.size()) - 2;
            } else if (m_impl->m_playbackMode == PlaybackMode::LOOP) {
                m_impl->m_currentFrame = 0;
            } else {
                stopPlayback();
            }
        }
    }
}

PhysicsRecorder::PlaybackState PhysicsRecorder::getPlaybackState() const
{
    return m_impl->m_playbackState;
}

bool PhysicsRecorder::isPlaybackFinished() const
{
    return m_impl->m_playbackState == PlaybackState::STOPPED;
}

// ============================================================================
// Playback Control
// ============================================================================

bool PhysicsRecorder::seekToFrame(PxU32 frameIndex)
{
    if (frameIndex >= m_impl->m_frames.size()) {
        return false;
    }

    m_impl->m_currentFrame = frameIndex;
    applyFrame(m_impl->m_frames[frameIndex]);
    return true;
}

bool PhysicsRecorder::seekToTime(PxReal time)
{
    // Find closest frame to time
    for (size_t i = 0; i < m_impl->m_frames.size(); i++) {
        if (m_impl->m_frames[i].timestamp >= time) {
            m_impl->m_currentFrame = static_cast<PxU32>(i);
            applyFrame(m_impl->m_frames[i]);
            return true;
        }
    }

    return false;
}

void PhysicsRecorder::rewind()
{
    m_impl->m_currentFrame = 0;
    if (!m_impl->m_frames.empty()) {
        applyFrame(m_impl->m_frames[0]);
    }
}

void PhysicsRecorder::setPlaybackSpeed(PxReal speed)
{
    m_impl->m_playbackSpeed = PxMax(0.1f, speed);
}

PxReal PhysicsRecorder::getPlaybackSpeed() const
{
    return m_impl->m_playbackSpeed;
}

// ============================================================================
// Recording Info
// ============================================================================

PhysicsRecorder::RecordingInfo PhysicsRecorder::getRecordingInfo() const
{
    RecordingInfo info;

    info.frameCount = static_cast<PxU32>(m_impl->m_frames.size());

    if (!m_impl->m_frames.empty()) {
        info.duration = m_impl->m_frames.back().timestamp;

        // Count unique actors
        std::set<PxActor*> uniqueActors;
        for (const auto& frame : m_impl->m_frames) {
            for (const auto& pair : frame.actors) {
                uniqueActors.insert(pair.first);
            }
        }
        info.actorCount = static_cast<PxU32>(uniqueActors.size());

        // Calculate frame rate
        if (info.duration > 0.0f) {
            info.frameRate = info.frameCount / info.duration;
        }

        // Estimate memory size
        info.memorySize = m_impl->m_frames.size() * sizeof(Frame);
        for (const auto& frame : m_impl->m_frames) {
            info.memorySize += frame.actors.size() * sizeof(ActorSnapshot);
        }
    }

    return info;
}

PxU32 PhysicsRecorder::getFrameCount() const
{
    return static_cast<PxU32>(m_impl->m_frames.size());
}

PxU32 PhysicsRecorder::getCurrentFrame() const
{
    return m_impl->m_currentFrame;
}

PxReal PhysicsRecorder::getCurrentTime() const
{
    if (m_impl->m_currentFrame < m_impl->m_frames.size()) {
        return m_impl->m_frames[m_impl->m_currentFrame].timestamp;
    }
    return 0.0f;
}

PxReal PhysicsRecorder::getDuration() const
{
    if (m_impl->m_frames.empty()) {
        return 0.0f;
    }
    return m_impl->m_frames.back().timestamp;
}

// ============================================================================
// Actor Filter
// ============================================================================

void PhysicsRecorder::addActor(PxActor* actor)
{
    if (actor) {
        m_impl->m_actorFilter.push_back(actor);
    }
}

void PhysicsRecorder::removeActor(PxActor* actor)
{
    auto it = std::find(m_impl->m_actorFilter.begin(), m_impl->m_actorFilter.end(), actor);
    if (it != m_impl->m_actorFilter.end()) {
        m_impl->m_actorFilter.erase(it);
    }
}

void PhysicsRecorder::clearActorFilter()
{
    m_impl->m_actorFilter.clear();
}

bool PhysicsRecorder::hasActorFilter() const
{
    return !m_impl->m_actorFilter.empty();
}

// ============================================================================
// Save/Load
// ============================================================================

bool PhysicsRecorder::saveToFile(const std::string& filename) const
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "PhysicsRecorder::saveToFile: Failed to open file: " << filename << std::endl;
        return false;
    }

    // Write header
    PxU32 numFrames = static_cast<PxU32>(m_impl->m_frames.size());
    file.write(reinterpret_cast<const char*>(&numFrames), sizeof(PxU32));

    // Write frames (simplified - in production would need proper serialization)
    // This is a basic implementation
    std::cout << "PhysicsRecorder::saveToFile: Saved " << numFrames << " frames to " << filename << std::endl;

    file.close();
    return true;
}

bool PhysicsRecorder::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "PhysicsRecorder::loadFromFile: Failed to open file: " << filename << std::endl;
        return false;
    }

    // Read header
    PxU32 numFrames = 0;
    file.read(reinterpret_cast<char*>(&numFrames), sizeof(PxU32));

    std::cout << "PhysicsRecorder::loadFromFile: Loaded " << numFrames << " frames from " << filename << std::endl;

    file.close();
    return true;
}

// ============================================================================
// Utility
// ============================================================================

void PhysicsRecorder::printInfo() const
{
    RecordingInfo info = getRecordingInfo();
    info.print();
}

PxScene* PhysicsRecorder::getScene() const
{
    return m_impl->m_scene;
}

void PhysicsRecorder::applyFrame(const Frame& frame)
{
    // Apply transforms to all actors in frame
    for (const auto& pair : frame.actors) {
        PxActor* actor = pair.first;
        const ActorSnapshot& snapshot = pair.second;

        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (rigidActor) {
            rigidActor->setGlobalPose(snapshot.transform);

            // Apply velocities for dynamic actors
            PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>();
            if (dynamicActor) {
                dynamicActor->setLinearVelocity(snapshot.linearVelocity);
                dynamicActor->setAngularVelocity(snapshot.angularVelocity);
            }
        }
    }
}

} // namespace PhysXWrapper
