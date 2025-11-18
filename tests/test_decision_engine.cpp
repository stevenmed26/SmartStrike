#include <gtest/gtest.h>

#include "strikezone/decision_engine.hpp"
#include "strikezone/types.hpp"
#include "strikezone/ball_tracker.hpp"

#include <opencv2/core.hpp>

using namespace sz;

// Helper to create a simple rectangular strike zone
static StrikeZoneBounds makeRectZone(float x1, float y1, float x2, float y2)
{
    StrikeZoneBounds zone;
    zone.polygon.emplace_back(x1, y1);
    zone.polygon.emplace_back(x2, y1);
    zone.polygon.emplace_back(x2, y2);
    zone.polygon.emplace_back(x1, y2);
    return zone;
}

TEST(DecisionEngineTest, ReturnsStrikeWhenTrajectoryCrossesZone)
{
    DecisionEngine engine;

    // Simple axis-aligned rectangular zone from (100,100) to (200,200)
    StrikeZoneBounds zone = makeRectZone(100.0f, 100.0f, 200.0f, 200.0f);

    // Trajectory: ball moves from y=50 to y=250 at x=150 (vertical line crossing zone)
    std::vector<BallState> traj;
    traj.push_back(BallState{cv::Point2f(150.0f,  50.0f), 0.00});
    traj.push_back(BallState{cv::Point2f(150.0f, 120.0f), 0.05});
    traj.push_back(BallState{cv::Point2f(150.0f, 180.0f), 0.10});
    traj.push_back(BallState{cv::Point2f(150.0f, 250.0f), 0.15});

    auto evtOpt = engine.evaluatePitch(traj, zone);
    ASSERT_TRUE(evtOpt.has_value()) << "DecisionEngine should return a PitchEvent";

    const PitchEvent& evt = *evtOpt;
    EXPECT_TRUE(evt.isStrike) << "Trajectory that crosses the strike zone should be a STRIKE";

    // Crossing point should have x ~ 150 and y between 100 and 200
    EXPECT_NEAR(evt.crossingPoint.x, 150.0f, 5.0f);
    EXPECT_GE(evt.crossingPoint.y, 100.0f);
    EXPECT_LE(evt.crossingPoint.y, 200.0f);

    // CrossingTime should be somewhere between 0.0 and 0.15
    EXPECT_GE(evt.crossingTime, 0.0);
    EXPECT_LE(evt.crossingTime, 0.15);
}

TEST(DecisionEngineTest, ReturnsBallWhenTrajectoryMissesZone)
{
    DecisionEngine engine;

    StrikeZoneBounds zone = makeRectZone(100.0f, 100.0f, 200.0f, 200.0f);

    // Trajectory: ball moves entirely to the left of the zone
    std::vector<BallState> traj;
    traj.push_back(BallState{cv::Point2f( 50.0f,  50.0f), 0.00});
    traj.push_back(BallState{cv::Point2f( 50.0f, 150.0f), 0.05});
    traj.push_back(BallState{cv::Point2f( 50.0f, 250.0f), 0.10});

    auto evtOpt = engine.evaluatePitch(traj, zone);
    ASSERT_TRUE(evtOpt.has_value()) << "DecisionEngine should still return an event for a BALL";

    const PitchEvent& evt = *evtOpt;
    EXPECT_FALSE(evt.isStrike) << "Trajectory that does not cross the zone should be a BALL";

    // For a ball, we expect crossingPoint/time to correspond to the last known state
    EXPECT_NEAR(evt.crossingPoint.x, 50.0f, 1.0f);
    EXPECT_NEAR(evt.crossingPoint.y, 250.0f, 1.0f);
    EXPECT_NEAR(evt.crossingTime, 0.10, 1e-6);
}

TEST(DecisionEngineTest, NoDecisionWhenInsufficientTrajectoryPoints)
{
    DecisionEngine engine;
    StrikeZoneBounds zone = makeRectZone(100.0f, 100.0f, 200.0f, 200.0f);

    // Only one point → cannot form a segment
    std::vector<BallState> traj;
    traj.push_back(BallState{cv::Point2f(150.0f, 50.0f), 0.0});

    auto evtOpt = engine.evaluatePitch(traj, zone);
    EXPECT_FALSE(evtOpt.has_value()) << "Should require at least two points in trajectory";
}
