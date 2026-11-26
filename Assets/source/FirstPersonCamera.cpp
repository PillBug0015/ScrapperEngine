#include "FirstPersonCamera.h"
#include "Scrapper/Jolt/SpringCharacter.h"

using namespace Scrapper;


void FirstPersonCamera::LateUpdate(float) {
    const GameObject body = player.Body();
    if (!body) return;
    const RagdollAsset& asset = *body.Get<Ragdoll>().asset;
    const PartId head = body.Get<CrewRig>().head;
    const RagdollAsset::Part& headRest = asset.At(head);
    const Quaternion pitch = QuaternionFromAxisAngle({ 1.0f, 0.0f, 0.0f }, player.Pitch());
    const Quaternion yaw = QuaternionFromAxisAngle({ 0.0f, 1.0f, 0.0f }, player.Yaw());

    // Steady: upright above the body's root, turned only by the mouse, so swaying limbs don't move the view.
    const Vector3 eyeFromRoot = Vector3Subtract(Vector3Add(headRest.position, EyeFromHead), asset.At(RagdollAsset::Root).position);
    const Vector3 steadyEye = Vector3Add(body.GetPosition(), Vector3RotateByQuaternion(eyeFromRoot, yaw));

    // Tumbling: wherever the head is, turned as far as the head has turned from rest.
    const GameObject headPart(body.GetWorld(), Ragdolls::PartEntity(body.GetWorld().Registry(), body.Entity(), head));
    const Quaternion headTurn = QuaternionMultiply(headPart.GetRotation(), QuaternionInvert(headRest.rotation));
    const Vector3 tumblingEye = Vector3Add(headPart.GetPosition(), Vector3RotateByQuaternion(EyeFromHead, headTurn));

    const float stability = body.Get<SpringCharacter>().stability;
    gameObject.SetPosition(Vector3Lerp(tumblingEye, steadyEye, stability));
    gameObject.SetRotation(QuaternionSlerp(QuaternionMultiply(headTurn, pitch), QuaternionMultiply(yaw, pitch), stability));
}
