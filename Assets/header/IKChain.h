#pragma once

#include "Renderer.h"
#include "GameObject.h"
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <cmath>

using ScrapperEngine::Renderer;
using ScrapperEngine::RenderCommand;
using ScrapperEngine::RenderSystem;
using ScrapperEngine::RenderType;

class IKChain : public Renderer {
public:
    std::vector<Vector2> joints;
    std::vector<float> lengths;
    Vector2 baseAnchor = { 400.0f, 300.0f };
    
    int maxIterations = 15;
    float tolerance = 1.0f;
    bool useFABRIK = true; // true: FABRIK, false: CCD

    IKChain() {
        // Initialize with a default chain of 5 joints, each bone length of 50.0f
        ResetChain(5, 50.0f);
    }

    void ResetChain(int numJoints, float boneLength) {
        if (numJoints < 2) numJoints = 2;
        
        joints.clear();
        lengths.clear();

        joints.resize(numJoints);
        lengths.resize(numJoints - 1, boneLength);

        // Position joints vertically downwards initially
        joints[0] = baseAnchor;
        for (int i = 1; i < numJoints; ++i) {
            joints[i] = { baseAnchor.x, baseAnchor.y + (i * boneLength) };
        }
    }

    void Start() override {
        if (owner) {
            if (auto* transform = owner->GetTransform()) {
                baseAnchor = transform->GetLocalPosition();
            }
            // Snap joints to the starting base anchor
            Vector2 offset = Vector2Subtract(baseAnchor, joints[0]);
            for (auto& joint : joints) {
                joint = Vector2Add(joint, offset);
            }
        }
    }

    void Update(float deltaTime) override {
        (void)deltaTime; // Unused in IK calculations, but kept for interface compliance

        // Allow moving the base anchor by holding down the Left Mouse Button
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (owner) {
                if (auto* transform = owner->GetTransform()) {
                    transform->SetLocalPosition(GetMousePosition());
                }
            }
        }

        // Update base anchor to follow the owner GameObject's position
        if (owner) {
            if (auto* transform = owner->GetTransform()) {
                baseAnchor = transform->GetLocalPosition();
            }
        }

        // --- Handle Keyboard Inputs ---
        // Toggle algorithm
        if (IsKeyPressed(KEY_ONE)) {
            useFABRIK = false;
        }
        if (IsKeyPressed(KEY_TWO)) {
            useFABRIK = true;
        }

        // Adjust chain length
        if (IsKeyPressed(KEY_UP)) {
            ResetChain(joints.size() + 1, 50.0f);
        }
        if (IsKeyPressed(KEY_DOWN) && joints.size() > 2) {
            ResetChain(joints.size() - 1, 50.0f);
        }

        // --- Solve IK ---
        Vector2 target = GetMousePosition();
        Solve(target);
    }

protected:
    void SubmitRenderCommand() override {
        // 1. Draw Bones as thick lines
        for (size_t i = 0; i < joints.size() - 1; ++i) {
            DrawLineEx(joints[i], joints[i + 1], 6.0f, DARKGRAY);
        }

        // 2. Draw Joints as circles
        for (size_t i = 0; i < joints.size(); ++i) {
            Color jointColor = BLUE;
            if (i == 0) {
                jointColor = RED; // Base anchor
            } else if (i == joints.size() - 1) {
                jointColor = GOLD; // End effector
            }
            
            DrawCircleV(joints[i], 8.0f, jointColor);
            DrawCircleLines(joints[i].x, joints[i].y, 8.0f, BLACK);
        }

        // 3. Draw Target Marker (Mouse Cursor)
        Vector2 target = GetMousePosition();
        DrawCircleLines(target.x, target.y, 12.0f, RED);
        DrawCircleV(target, 4.0f, RED);

        // 4. Draw UI & Statistics
        DrawText("Inverse Kinematics (IK) Demo", 20, 20, 22, BLACK);
        DrawText("Controls:", 20, 60, 16, DARKGRAY);
        DrawText("- Move MOUSE: Move IK Target", 20, 80, 14, GRAY);
        DrawText("- Press '1': Use CCD Solver", 20, 100, 14, useFABRIK ? GRAY : GREEN);
        DrawText("- Press '2': Use FABRIK Solver", 20, 120, 14, useFABRIK ? GREEN : GRAY);
        DrawText("- Press 'UP'/'DOWN': Add/Remove Joints", 20, 140, 14, GRAY);

        // Solver status indicators
        DrawText("Solver Status:", 20, 180, 16, DARKGRAY);
        DrawText(TextFormat("Active Solver: %s", useFABRIK ? "FABRIK (Position-Based)" : "CCD (Angle-Based)"), 20, 200, 14, MAROON);
        DrawText(TextFormat("Joint Count  : %d joints", (int)joints.size()), 20, 220, 14, DARKGRAY);
        
        float totalLength = 0.0f;
        for (float l : lengths) totalLength += l;
        DrawText(TextFormat("Chain Length : %.1f px", totalLength), 20, 240, 14, DARKGRAY);
        
        float error = Vector2Distance(joints.back(), target);
        DrawText(TextFormat("Target Error : %.2f px", error), 20, 260, 14, (error < tolerance * 2.0f) ? DARKGREEN : RED);
    }

private:
    void Solve(Vector2 target) {
        if (useFABRIK) {
            SolveFABRIK(target);
        } else {
            SolveCCD(target);
        }
    }

    // --- CCD (Cyclic Coordinate Descent) Solver ---
    void SolveCCD(Vector2 target) {
        int numJoints = joints.size();
        if (numJoints < 2) return;

        for (int iter = 0; iter < maxIterations; ++iter) {
            // Check if end effector is close enough to target
            Vector2 endEffector = joints[numJoints - 1];
            float dist = Vector2Distance(endEffector, target);
            if (dist < tolerance) break;

            // Loop from the second-to-last joint back to the base joint
            for (int i = numJoints - 2; i >= 0; --i) {
                Vector2 currentJoint = joints[i];
                Vector2 endPos = joints[numJoints - 1];

                // Vector from current joint to end effector
                Vector2 toEnd = Vector2Subtract(endPos, currentJoint);
                float lenToEnd = Vector2Length(toEnd);
                if (lenToEnd < 0.001f) continue;

                // Vector from current joint to target
                Vector2 toTarget = Vector2Subtract(target, currentJoint);
                float lenToTarget = Vector2Length(toTarget);
                if (lenToTarget < 0.001f) continue;

                // Calculate angle difference using 2D cross and dot products (or atan2)
                float angleToEnd = atan2f(toEnd.y, toEnd.x);
                float angleToTarget = atan2f(toTarget.y, toTarget.x);
                float angleDiff = angleToTarget - angleToEnd;

                // Clamp angleDiff to avoid huge jumps (optional, but makes movement smoother)
                // For a toy project, direct snapping is fine and shows raw algorithm speed.

                float cosAngle = cosf(angleDiff);
                float sinAngle = sinf(angleDiff);

                // Rotate all descendant joints around the current joint
                for (int j = i + 1; j < numJoints; ++j) {
                    Vector2 relativePos = Vector2Subtract(joints[j], currentJoint);
                    joints[j].x = currentJoint.x + (relativePos.x * cosAngle - relativePos.y * sinAngle);
                    joints[j].y = currentJoint.y + (relativePos.x * sinAngle + relativePos.y * cosAngle);
                }
            }
        }
    }

    // --- FABRIK (Forward And Backward Reaching Inverse Kinematics) Solver ---
    void SolveFABRIK(Vector2 target) {
        int numJoints = joints.size();
        if (numJoints < 2) return;

        Vector2 basePos = baseAnchor;

        // 1. Check if the target is out of reach
        float distToTarget = Vector2Distance(basePos, target);
        float totalLength = 0.0f;
        for (float l : lengths) {
            totalLength += l;
        }

        if (distToTarget > totalLength) {
            // Target is unreachable: Stretch the chain in a straight line towards the target
            for (int i = 0; i < numJoints - 1; ++i) {
                Vector2 dir = Vector2Normalize(Vector2Subtract(target, joints[i]));
                joints[i + 1] = Vector2Add(joints[i], Vector2Scale(dir, lengths[i]));
            }
            return;
        }

        // 2. Target is reachable: Run iterative snapping passes
        for (int iter = 0; iter < maxIterations; ++iter) {
            float dist = Vector2Distance(joints[numJoints - 1], target);
            if (dist < tolerance) break;

            // --- Stage 1: Forward Pass (Target to Base) ---
            joints[numJoints - 1] = target;
            for (int i = numJoints - 2; i >= 0; --i) {
                Vector2 dir = Vector2Subtract(joints[i], joints[i + 1]);
                float currentDist = Vector2Length(dir);
                if (currentDist < 0.001f) currentDist = 0.001f;
                
                Vector2 dirNormalized = Vector2Scale(dir, 1.0f / currentDist);
                joints[i] = Vector2Add(joints[i + 1], Vector2Scale(dirNormalized, lengths[i]));
            }

            // --- Stage 2: Backward Pass (Base to Target) ---
            joints[0] = basePos;
            for (int i = 0; i < numJoints - 1; ++i) {
                Vector2 dir = Vector2Subtract(joints[i + 1], joints[i]);
                float currentDist = Vector2Length(dir);
                if (currentDist < 0.001f) currentDist = 0.001f;
                
                Vector2 dirNormalized = Vector2Scale(dir, 1.0f / currentDist);
                joints[i + 1] = Vector2Add(joints[i], Vector2Scale(dirNormalized, lengths[i]));
            }
        }
    }
};
