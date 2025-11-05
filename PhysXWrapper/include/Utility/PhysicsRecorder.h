/**
 * @file PhysicsRecorder.h
 * @brief Physics recorder for recording and playback
 *
 * This class provides utilities for recording physics simulations
 * and playing them back, useful for debugging, replays, and
 * cinematics.
 *
 * Records actor transforms over time
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <string>
#include <map>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class PhysicsRecorder
 * @brief Recorder for physics simulations
 *
 * PhysicsRecorder captures the state of physics actors over time
 * and allows playback of recorded simulations. This is useful for:
 * - Debugging physics behavior
 * - Creating replay systems
 * - Recording cinematics
 * - Performance analysis
 * - Determinism testing
 *
 * Features:
 * - Record actor transforms (position, rotation)
 * - Record linear and angular velocity
 * - Frame-by-frame playback
 * - Save/load recordings to disk
 * - Playback controls (play, pause, rewind, seek)
 * - Multiple recording slots
 *
 * Usage:
 * @code
 * PhysicsRecorder recorder;
 * recorder.initialize(scene);
 *
 * // Start recording
 * recorder.startRecording();
 *
 * // Simulation loop
 * while (simulating) {
 *     scene->simulate(dt);
 *     scene->fetchResults(true);
 *     recorder.recordFrame(); // Record current state
 * }
 *
 * recorder.stopRecording();
 *
 * // Playback
 * recorder.startPlayback();
 * while (!recorder.isPlaybackFinished()) {
 *     recorder.updatePlayback(dt);
 * }
 * @endcode
 */
class PhysicsRecorder {
public:
    /**
     * @brief Playback mode
     */
    enum class PlaybackMode {
        ONCE,       ///< Play once then stop
        LOOP,       ///< Loop continuously
        PING_PONG   ///< Play forward then backward
    };

    /**
     * @brief Playback state
     */
    enum class PlaybackState {
        STOPPED,    ///< Not playing
        PLAYING,    ///< Currently playing
        PAUSED      ///< Paused
    };

    /**
     * @brief Actor snapshot (single frame data)
     */
    struct ActorSnapshot {
        PxTransform transform;      ///< Position and rotation
        PxVec3 linearVelocity;      ///< Linear velocity
        PxVec3 angularVelocity;     ///< Angular velocity

        ActorSnapshot()
            : transform(PxIdentity)
            , linearVelocity(0, 0, 0)
            , angularVelocity(0, 0, 0)
        {}
    };

    /**
     * @brief Frame data (all actors in one frame)
     */
    struct Frame {
        PxReal timestamp;                           ///< Frame timestamp
        std::map<PxActor*, ActorSnapshot> actors;  ///< Actor data

        Frame() : timestamp(0.0f) {}
    };

    /**
     * @brief Recording info
     */
    struct RecordingInfo {
        PxU32 frameCount = 0;           ///< Number of frames
        PxReal duration = 0.0f;         ///< Total duration (seconds)
        PxU32 actorCount = 0;           ///< Number of actors
        PxReal frameRate = 0.0f;        ///< Average frame rate
        size_t memorySize = 0;          ///< Memory size (bytes)

        void print() const;
    };

public:
    /**
     * @brief Constructor
     */
    PhysicsRecorder();

    /**
     * @brief Destructor
     */
    ~PhysicsRecorder();

    // Disable copy
    PhysicsRecorder(const PhysicsRecorder&) = delete;
    PhysicsRecorder& operator=(const PhysicsRecorder&) = delete;

    /**
     * @brief Initialize recorder
     * @param scene PhysX scene instance
     * @return true if successful
     */
    bool initialize(PxScene* scene);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Recording
    // ========================================================================

    /**
     * @brief Start recording
     * @param clearExisting Clear existing recording
     * @return true if successful
     */
    bool startRecording(bool clearExisting = true);

    /**
     * @brief Stop recording
     */
    void stopRecording();

    /**
     * @brief Record current frame
     * @return true if successful
     *
     * Call this after each simulation step
     */
    bool recordFrame();

    /**
     * @brief Check if recording
     * @return true if recording
     */
    bool isRecording() const;

    /**
     * @brief Clear recording
     */
    void clearRecording();

    // ========================================================================
    // Playback
    // ========================================================================

    /**
     * @brief Start playback
     * @param mode Playback mode
     * @return true if successful
     */
    bool startPlayback(PlaybackMode mode = PlaybackMode::ONCE);

    /**
     * @brief Stop playback
     */
    void stopPlayback();

    /**
     * @brief Pause playback
     */
    void pausePlayback();

    /**
     * @brief Resume playback
     */
    void resumePlayback();

    /**
     * @brief Update playback
     * @param deltaTime Time step
     *
     * Call this each frame during playback
     */
    void updatePlayback(PxReal deltaTime);

    /**
     * @brief Get playback state
     * @return Playback state
     */
    PlaybackState getPlaybackState() const;

    /**
     * @brief Check if playback finished
     * @return true if finished
     */
    bool isPlaybackFinished() const;

    // ========================================================================
    // Playback Control
    // ========================================================================

    /**
     * @brief Seek to specific frame
     * @param frameIndex Frame index
     * @return true if successful
     */
    bool seekToFrame(PxU32 frameIndex);

    /**
     * @brief Seek to time
     * @param time Time in seconds
     * @return true if successful
     */
    bool seekToTime(PxReal time);

    /**
     * @brief Rewind to beginning
     */
    void rewind();

    /**
     * @brief Set playback speed
     * @param speed Speed multiplier (1.0 = normal)
     */
    void setPlaybackSpeed(PxReal speed);

    /**
     * @brief Get playback speed
     * @return Speed multiplier
     */
    PxReal getPlaybackSpeed() const;

    // ========================================================================
    // Recording Info
    // ========================================================================

    /**
     * @brief Get recording info
     * @return Recording information
     */
    RecordingInfo getRecordingInfo() const;

    /**
     * @brief Get frame count
     * @return Number of frames
     */
    PxU32 getFrameCount() const;

    /**
     * @brief Get current frame index
     * @return Current frame (during playback)
     */
    PxU32 getCurrentFrame() const;

    /**
     * @brief Get current playback time
     * @return Current time in seconds
     */
    PxReal getCurrentTime() const;

    /**
     * @brief Get recording duration
     * @return Duration in seconds
     */
    PxReal getDuration() const;

    // ========================================================================
    // Actor Filter
    // ========================================================================

    /**
     * @brief Add actor to recording
     * @param actor Actor to record
     */
    void addActor(PxActor* actor);

    /**
     * @brief Remove actor from recording
     * @param actor Actor to remove
     */
    void removeActor(PxActor* actor);

    /**
     * @brief Clear actor list (record all actors)
     */
    void clearActorFilter();

    /**
     * @brief Check if using actor filter
     * @return true if filtering actors
     */
    bool hasActorFilter() const;

    // ========================================================================
    // Save/Load
    // ========================================================================

    /**
     * @brief Save recording to file
     * @param filename Output filename
     * @return true if successful
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief Load recording from file
     * @param filename Input filename
     * @return true if successful
     */
    bool loadFromFile(const std::string& filename);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Print recording info
     */
    void printInfo() const;

    /**
     * @brief Get PhysX scene
     * @return Scene instance
     */
    PxScene* getScene() const;

private:
    /**
     * @brief Apply frame to scene
     * @param frame Frame to apply
     */
    void applyFrame(const Frame& frame);

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
