#ifndef KGPhysicsManager_h
#define KGPhysicsManager_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef struct KGPhysicsManager KGPhysicsManager;
typedef uint32_t KGPhysicsManagerBodyID;
typedef uint32_t KGPhysicsManagerColliderID;
typedef uint32_t KGPhysicsManagerConstraintID;
typedef union KGPhysicsManagerEventData KGPhysicsManagerEventData;
union KGPhysicsManagerEventData
{
	struct
	{
		float contactPosition[3], contactNormal[3];
		uint32_t bodyIDs[2];
	}collision;
	struct
	{
		KGPhysicsManagerBodyID bodyID;
		float
			position[3],
			rotation[4],
			linearVelocity[3],
			angularVelocity[3];
	}bodyMovementStateChange;
};

typedef enum 
{
	KGPhysicsManagerEventTypeNone,
	KGPhysicsManagerEventTypeCollision,
	KGPhysicsManagerEventTypeBodyMovementStateChange
}KGPhysicsManagerEventType;

KGPhysicsManager* KGPhysicsManagerNew(void);
void KGPhysicsManagerDelete(KGPhysicsManager* self);

void KGPhysicsManagerUpdate(KGPhysicsManager* self, float seconds);

void KGPhysicsManagerSetGravity(KGPhysicsManager* self, const float gravityVector[3]);

KGPhysicsManagerBodyID KGPhysicsManagerCreateBody(KGPhysicsManager* self);
void KGPhysicsManagerDestroyBody(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID);
KGPhysicsManagerBodyID KGPhysicsManagerCreateBodyWithMassAndCollider(KGPhysicsManager* self, float mass, KGPhysicsManagerColliderID colliderID);

void KGPhysicsManagerSetBodyPosition(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float in[3]);
void KGPhysicsManagerGetBodyPosition(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float* to);

void KGPhysicsManagerSetBodyRotation(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float in[4]);

void KGPhysicsManagerSetBodyDilation(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to);

void KGPhysicsManagerSetBodyLinearVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[3]);
void KGPhysicsManagerSetBodyAngularVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[3]);

void KGPhysicsManagerSetBodyCCDSpeedThreshold(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to);
void KGPhysicsManagerSetBodyCCDRadius(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to);

void KGPhysicsManagerApplyBodyImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float impulse[3], const float relativePosition[3]);
void KGPhysicsManagerApplyBodyCentralImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float impulse[3]);
void KGPhysicsManagerApplyBodyCentralForce(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float force[3]);

void KGPhysicsManagerSetBodyHasCollisionResponse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, bool to);

void KGPhysicsManagerApplyBodyCentralTorqueImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float amount[3]);
void KGPhysicsManagerApplyBodyCentralTorque(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float amount[3]);

void KGPhysicsManagerSetBodyEventSubscription(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, KGPhysicsManagerEventType type, bool to);

void KGPhysicsManagerSetBodyFriction(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to);

void KGPhysicsManagerSetBodyMass(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to);

void KGPhysicsManagerSetBodyCollider(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, KGPhysicsManagerColliderID to);

KGPhysicsManagerColliderID KGPhysicsManagerCreateCollider(KGPhysicsManager* self);
void KGPhysicsManagerDestroyCollider(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID);

void KGPhysicsManagerSetColliderSphereRadius(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, float to);

void KGPhysicsManagerSetColliderBoxExtents(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float to[3]);

void KGPhysicsManagerSetColliderCapsuleExtents(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, float radius, float height);

void KGPhysicsManagerSetColliderMeshVertexCount(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, uint32_t to);

void KGPhysicsManagerSetColliderMeshVertexPositions(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float* to);

void KGPhysicsManagerSetColliderHullVertexCount(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, uint32_t to);

void KGPhysicsManagerSetColliderHullVertexPositions(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float positions[][3], uint32_t count);

KGPhysicsManagerEventType KGPhysicsManagerPollEvent(KGPhysicsManager* self, KGPhysicsManagerEventData* data);
KGPhysicsManagerBodyID KGPhysicsManagerGetClosestBodyAlongRay(KGPhysicsManager* self, const float from[3], const float to[3], float hitPosition[3], float hitNormal[3], KGPhysicsManagerBodyID excludedBodyID);

void KGPhysicsManagerGetBodyLinearVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float* to);

#ifdef __cplusplus
}
#endif

#endif
