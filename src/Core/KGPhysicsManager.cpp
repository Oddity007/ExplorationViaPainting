#if 1

extern "C"
{
#include "KGPhysicsManager.h"
}

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include <queue>
#include <vector>
#include <map>
#include <string.h>

#define PUBLIC_C_VISIBILITY extern "C" __attribute__ ((visibility ("default")))

typedef enum 
{
	ColliderTypeNone,
	ColliderTypeSphere,
	ColliderTypeBox,
	ColliderTypeBVHTriangleMesh,
	ColliderTypeCapsule,
	ColliderTypeHull,
}ColliderType;

typedef std::map<float, btCollisionShape*> ScaledCollisionShapeMapping;

struct Collider
{
	btCollisionShape* collisionShape;
	union {
		/*struct {float radius;} sphere;
		struct {AEVec3 halfSize;} box;*/
		struct 
		{
			btTriangleMesh* collisionTriangleMesh;
			uint32_t vertexCount;
		} bvhTriangleMesh;
		/*struct 
		{
			float radius;
			float height;
		}capsule;*/
	}data;
	ColliderType type;
	uint32_t referenceCount;
	ScaledCollisionShapeMapping* scaledVariations;
};

struct Event
{
	KGPhysicsManagerEventType type;
	KGPhysicsManagerEventData data;
};

struct RigidBodyUserdata
{
	uint32_t bodyID;
	bool
		isSubscribedToCollision :1,
		isSubscribedToMovementStateChange :1;
};

struct KGPhysicsManager
{
	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;
	std::vector<btRigidBody*> rigidBodies;
	std::vector<Collider> colliders;
	uint32_t collisionMessageResponderCount;
	std::queue<Event> eventQueue;
};

//static KGPhysicsManagerColliderID KGPhysicsManagerRetainCollider(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID);

PUBLIC_C_VISIBILITY
KGPhysicsManager* KGPhysicsManagerNew(void)
{
	KGPhysicsManager* self = new KGPhysicsManager;
	self->broadphase = new btDbvtBroadphase();
	self->collisionConfiguration = new btDefaultCollisionConfiguration();
	self->dispatcher = new btCollisionDispatcher(self->collisionConfiguration);
	self->solver = new btSequentialImpulseConstraintSolver;
	self->dynamicsWorld = new btDiscreteDynamicsWorld(self->dispatcher, self->broadphase, self->solver, self->collisionConfiguration);
	return self;
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerDelete(KGPhysicsManager* self)
{
	delete self->dynamicsWorld->getDebugDrawer();
	delete self->dynamicsWorld;
	delete self->solver;
	delete self->collisionConfiguration;
	delete self->dispatcher;
	delete self->broadphase;
	delete self;
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerUpdate(KGPhysicsManager* self, float seconds)
{
	self->dynamicsWorld->stepSimulation(seconds);
	for (std::vector<btRigidBody*>::const_iterator iterator = self->rigidBodies.begin(); iterator not_eq self->rigidBodies.end(); iterator++)
	{
		if (*iterator == NULL) continue;
		const RigidBodyUserdata* userdata = (const RigidBodyUserdata*) (*(iterator))->getUserPointer();
		assert(userdata);
		if(userdata->isSubscribedToMovementStateChange)
		{
			Event event;
			event.type = KGPhysicsManagerEventTypeBodyMovementStateChange;
			event.data.bodyMovementStateChange.bodyID = userdata->bodyID;
			for (int i = 0; i < 3; i++) event.data.bodyMovementStateChange.position[i] =  (*(iterator))->getWorldTransform().getOrigin()[i];
			for (int i = 0; i < 4; i++) event.data.bodyMovementStateChange.rotation[i] =  (*(iterator))->getWorldTransform().getRotation()[i];
			for (int i = 0; i < 3; i++) event.data.bodyMovementStateChange.linearVelocity[i] =  (*(iterator))->getLinearVelocity()[i];
			for (int i = 0; i < 3; i++) event.data.bodyMovementStateChange.angularVelocity[i] =  (*(iterator))->getAngularVelocity()[i];
			self->eventQueue.push(event);
		}
	}
	/*printf("number of manifolds: %i\n", (int)self->dispatcher->getNumManifolds());
	printf("number of collision objects: %i\n", (int)self->dynamicsWorld->getNumCollisionObjects());*/
	int manifoldCount = self->dynamicsWorld->getDispatcher()->getNumManifolds();
	//if(self->collisionMessageResponderCount) 
	for (int i=0; i<manifoldCount; i++) {
		const btPersistentManifold* contactManifold = self->dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		btCollisionObject* objectA = (btCollisionObject*) contactManifold->getBody0();
		btCollisionObject* objectB = (btCollisionObject*) contactManifold->getBody1();
		
		const RigidBodyUserdata* userdata1 = (const RigidBodyUserdata*) objectA->getUserPointer();
		const RigidBodyUserdata* userdata2 = (const RigidBodyUserdata*) objectB->getUserPointer();
		//if(not (idA and idB)) continue;
		assert(userdata1);
		assert(userdata2);
		
		if(not (userdata1->isSubscribedToCollision or userdata2->isSubscribedToCollision)) continue;
		
		int contactCount = contactManifold->getNumContacts();
		if(contactCount < 1) continue;
		const btManifoldPoint& pt = contactManifold->getContactPoint(0);
		Event event;
		event.type = KGPhysicsManagerEventTypeCollision;
		event.data.collision.bodyIDs[0] = userdata1->bodyID;
		event.data.collision.bodyIDs[1] = userdata2->bodyID;
		for (int i = 0; i < 3; i++) event.data.collision.contactPosition[i] = (pt.getPositionWorldOnB())[i];
		for (int i = 0; i < 3; i++) event.data.collision.contactNormal[i] = (pt.m_normalWorldOnB)[i];
		self->eventQueue.push(event);
	}
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetGravity(KGPhysicsManager* self, const float gravityVector[3])
{
	self->dynamicsWorld->setGravity(btVector3(gravityVector[0], gravityVector[1], gravityVector[2]));
}

PUBLIC_C_VISIBILITY
KGPhysicsManagerBodyID KGPhysicsManagerCreateBody(KGPhysicsManager* self)
{
	btRigidBody* rigidBody = NULL;
	size_t index = 0;
	for (size_t i = 0; i < self->rigidBodies.size(); i++) if(self->rigidBodies[i]==NULL) {index = i + 1; break;}
	btRigidBody::btRigidBodyConstructionInfo constructionInfo(0, NULL, NULL);
	rigidBody = new btRigidBody(constructionInfo);
	self->dynamicsWorld->addRigidBody(rigidBody);
	rigidBody->setFriction(SIMD_INFINITY);
	if(not index) 
	{
		self->rigidBodies.push_back(rigidBody);
		index = self->rigidBodies.size();
	}
	else self->rigidBodies[index - 1] = rigidBody;
	RigidBodyUserdata* userdata = new RigidBodyUserdata();
	userdata->bodyID = index;
	userdata->isSubscribedToCollision  = false;
	rigidBody->setUserPointer(userdata);
	return index;
}

PUBLIC_C_VISIBILITY
KGPhysicsManagerBodyID KGPhysicsManagerCreateBodyWithMassAndCollider(KGPhysicsManager* self, float mass, KGPhysicsManagerColliderID colliderID)
{
	btRigidBody* rigidBody = NULL;
	size_t index = 0;
	for (size_t i = 0; i < self->rigidBodies.size(); i++) if(self->rigidBodies[i]==NULL) {index = i + 1; break;}
	btCollisionShape* collisionShape = colliderID ? self->colliders[colliderID - 1].collisionShape : NULL;
	btVector3 inertia(0,0,0);
	if(collisionShape) collisionShape->calculateLocalInertia(mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo constructionInfo(mass, NULL, collisionShape, inertia);
	rigidBody = new btRigidBody(constructionInfo);
	rigidBody->setFriction(SIMD_INFINITY);
	self->dynamicsWorld->addRigidBody(rigidBody);
	if(not index) 
	{
		self->rigidBodies.push_back(rigidBody);
		index = self->rigidBodies.size();
	}
	else self->rigidBodies[index - 1] = rigidBody;
	RigidBodyUserdata* userdata = new RigidBodyUserdata();
	userdata->bodyID = index;
	userdata->isSubscribedToCollision  = false;
	rigidBody->setUserPointer(userdata);
	return index;
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerDestroyBody(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID)
{
	if(not bodyID) return;
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	delete (RigidBodyUserdata*) rigidBody->getUserPointer();
	self->dynamicsWorld->removeCollisionObject(rigidBody);
	delete rigidBody;
	self->rigidBodies[bodyID - 1] = NULL;
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyPosition(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->getWorldTransform().setOrigin(btVector3(to[0], to[1], to[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerGetBodyPosition(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float* to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	for (int i = 0; i < 3; i++) to[i] =  rigidBody->getWorldTransform().getOrigin()[i];
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyRotation(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[4])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->getWorldTransform().setRotation(btQuaternion(to[0], to[1], to[2], to[3]));
}

/*PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyDilation(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	btCollisionShape* collisionShape = rigidBody->getRootCollisionShape();
	const uint32_t* collisionShapeUserPointer = (const uint32_t*) collisionShape->getUserPointer();
	if(not collisionShapeUserPointer) return;
	uint32_t colliderID = *collisionShapeUserPointer;
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->collisionShape == collisionShape and to == 1) return;
	if(not collider->scaledVariations)
	{
		collider->scaledVariations = new ScaledCollisionShapeMapping;
	}
	const ScaledCollisionShapeMapping::const_iterator i = collider->scaledVariations->find(to);
	btCollisionShape* scaledCollisionShape = NULL;
	if(i == collider->scaledVariations->end())
	{
		if(collider->type == ColliderTypeBVHTriangleMesh)
		{
			scaledCollisionShape = new btScaledBvhTriangleMeshShape((btBvhTriangleMeshShape*) collider->collisionShape, btVector3(to, to, to));
		}
		else
		{
			scaledCollisionShape = new btUniformScalingShape((btConvexShape*) collisionShape, to);
		}
		scaledCollisionShape->setUserPointer((void*) collisionShapeUserPointer);
		(*collider->scaledVariations)[to] = scaledCollisionShape;
	}
	else scaledCollisionShape = i->second;
	
	rigidBody->setCollisionShape(scaledCollisionShape);
	btScalar mass = 1.0/rigidBody->getInvMass();
	btVector3 inertia(0,0,0);
	if(scaledCollisionShape) scaledCollisionShape->calculateLocalInertia(mass, inertia);
	rigidBody->setMassProps(mass, inertia);
	self->dynamicsWorld->removeCollisionObject(rigidBody);
	self->dynamicsWorld->addCollisionObject(rigidBody);
}*/

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyLinearVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setActivationState(DISABLE_DEACTIVATION);
	//rigidBody->setDeactivationTime(0);
	rigidBody->setLinearVelocity(btVector3(to[0], to[1], to[2]));
}


PUBLIC_C_VISIBILITY
void KGPhysicsManagerGetBodyLinearVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float* to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	auto linearVelocity = rigidBody->getLinearVelocity();
	for (int i = 0; i < 3; i++) to[i] =  linearVelocity[i];
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyAngularVelocity(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float to[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setDeactivationTime(0);
	rigidBody->setAngularVelocity(btVector3(to[0], to[1], to[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyCCDSpeedThreshold(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->setCcdMotionThreshold(to);
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyCCDRadius(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->setCcdSweptSphereRadius(to);
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyHasCollisionResponse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, bool to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	if(not to) rigidBody->setCollisionFlags(btRigidBody::CF_NO_CONTACT_RESPONSE bitor rigidBody->getCollisionFlags());
	else rigidBody->setCollisionFlags((compl btRigidBody::CF_NO_CONTACT_RESPONSE) bitand rigidBody->getCollisionFlags());
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerApplyBodyImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float impulse[3], const float relativePosition[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setDeactivationTime(0);
	//rigidBody->applyCentralImpulse(AEVec3AsBulletVector(impulse));
	rigidBody->applyImpulse(btVector3(impulse[0], impulse[1], impulse[2]), btVector3(relativePosition[0], relativePosition[1], relativePosition[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerApplyBodyCentralImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float impulse[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setActivationState(DISABLE_DEACTIVATION);
	//rigidBody->setDeactivationTime(0);
	rigidBody->applyCentralImpulse(btVector3(impulse[0], impulse[1], impulse[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerApplyBodyCentralForce(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float force[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setDeactivationTime(0);
	rigidBody->applyCentralForce(btVector3(force[0], force[1], force[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerApplyBodyCentralTorqueImpulse(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float amount[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setDeactivationTime(0);
	rigidBody->applyTorqueImpulse(btVector3(amount[0], amount[1], amount[2]));
	//rigidBody->applyCentralForce(AEVec3AsBulletVector(force));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerApplyBodyCentralTorque(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, const float amount[3])
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->activate(true);
	//rigidBody->setDeactivationTime(0);
	rigidBody->applyTorque(btVector3(amount[0], amount[1], amount[2]));
	//rigidBody->applyCentralForce(AEVec3AsBulletVector(force));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyEventSubscription(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, KGPhysicsManagerEventType type, bool to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	RigidBodyUserdata* userdata = (RigidBodyUserdata*) rigidBody->getUserPointer();
	switch (type)
	{
		case KGPhysicsManagerEventTypeCollision:
			userdata->isSubscribedToCollision = to;
			break;
		case KGPhysicsManagerEventTypeBodyMovementStateChange:
			userdata->isSubscribedToMovementStateChange = to;
		default:
			break;
	}
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyFriction(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	rigidBody->setFriction(to);
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyMass(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, float to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	const btCollisionShape* collisionShape = rigidBody->getCollisionShape();
	btVector3 inertia(0,0,0);
	if(not collisionShape) abort();
	if(collisionShape) collisionShape->calculateLocalInertia(to, inertia);
	rigidBody->setMassProps(to, inertia);
	self->dynamicsWorld->removeCollisionObject(rigidBody);
	self->dynamicsWorld->addCollisionObject(rigidBody);
	if(to)
	{
		rigidBody->activate(true);
		//rigidBody->setDeactivationTime(0);
		//rigidBody->setActivationState(DISABLE_DEACTIVATION);
	}
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetBodyCollider(KGPhysicsManager* self, KGPhysicsManagerBodyID bodyID, KGPhysicsManagerColliderID to)
{
	btRigidBody* rigidBody = self->rigidBodies[bodyID - 1];
	btCollisionShape* collisionShape = to ? self->colliders[to - 1].collisionShape : NULL;
	rigidBody->setCollisionShape(collisionShape);
	btScalar mass = 1.0/rigidBody->getInvMass();
	btVector3 inertia(0,0,0);
	if(collisionShape) collisionShape->calculateLocalInertia(mass, inertia);
	rigidBody->setMassProps(mass, inertia);
	self->dynamicsWorld->removeCollisionObject(rigidBody);
	self->dynamicsWorld->addCollisionObject(rigidBody);
}

PUBLIC_C_VISIBILITY
KGPhysicsManagerColliderID KGPhysicsManagerCreateCollider(KGPhysicsManager* self)
{
	Collider collider;
	memset(& collider, 0, sizeof(collider));
	size_t index = 0;
	for (size_t i = 0; i < self->colliders.size(); i++) if(memcmp(& self->colliders[i], & collider, sizeof(Collider)) == 0) {index = i + 1; break;}
	collider.referenceCount = 1;
	if(not index) 
	{
		self->colliders.push_back(collider);
		index = self->colliders.size();
	}
	else self->colliders[index - 1] = collider;
	return index;
}

/*static KGPhysicsManagerColliderID KGPhysicsManagerRetainCollider(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID)
{
	if(not colliderID) return 0;
	Collider* collider = & self->colliders[colliderID - 1];
	collider->referenceCount++;
	return colliderID;
}*/

PUBLIC_C_VISIBILITY
void KGPhysicsManagerDestroyCollider(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID)
{
	if(not colliderID) return;
	Collider* collider = & self->colliders[colliderID - 1];
	collider->referenceCount--;
	if(collider->referenceCount) return;
	if(collider->type == ColliderTypeBVHTriangleMesh) delete collider->data.bvhTriangleMesh.collisionTriangleMesh;
	if (collider->scaledVariations)
		for (ScaledCollisionShapeMapping::const_iterator i = collider->scaledVariations->begin(); i not_eq collider->scaledVariations->end(); i++)
		{
			delete i->second;
		}
	delete collider->scaledVariations;
	delete (uint32_t*) collider->collisionShape->getUserPointer();
	delete collider->collisionShape;
	memset(collider, 0, sizeof(Collider));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderSphereRadius(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, float to)
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type == ColliderTypeNone) 
	{
		collider->type = ColliderTypeSphere;
		collider->collisionShape = new btSphereShape(to);
		collider->collisionShape->setUserPointer(new uint32_t(colliderID));
	}
	else if(collider->type not_eq ColliderTypeSphere) abort();
	else ((btSphereShape*)collider->collisionShape)->setUnscaledRadius(to);
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderBoxExtents(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float to[3])
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type == ColliderTypeNone) 
	{
		collider->type = ColliderTypeBox;
		collider->collisionShape = new btBoxShape(btVector3(to[0], to[1], to[2]));
		collider->collisionShape->setUserPointer(new uint32_t(colliderID));
	}
	else if(collider->type not_eq ColliderTypeBox) abort();
	else ((btBoxShape*)collider->collisionShape)->setImplicitShapeDimensions(btVector3(to[0], to[1], to[2]));
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderCapsuleExtents(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, float radius, float height)
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type == ColliderTypeNone) 
	{
		collider->type = ColliderTypeCapsule;
		collider->collisionShape = new btCapsuleShape(radius, height);
		collider->collisionShape->setUserPointer(new uint32_t(colliderID));
	}
	else if(collider->type not_eq ColliderTypeCapsule) abort();
	else abort();
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderMeshVertexCount(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, uint32_t to)
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type == ColliderTypeNone) 
	{
		collider->type = ColliderTypeBVHTriangleMesh;
		collider->data.bvhTriangleMesh.collisionTriangleMesh = new btTriangleMesh();
		collider->data.bvhTriangleMesh.vertexCount = to;
		collider->collisionShape = NULL;
	}
	else if(collider->type not_eq ColliderTypeBVHTriangleMesh) abort();
	else abort();
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderMeshVertexPositions(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float* to)
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type not_eq ColliderTypeBVHTriangleMesh) abort();
	assert(collider->data.bvhTriangleMesh.collisionTriangleMesh->getNumTriangles() == 0);
	for (uint32_t i=0; i<collider->data.bvhTriangleMesh.vertexCount; i+=3)
		collider->data.bvhTriangleMesh.collisionTriangleMesh->addTriangle(btVector3(to[i * 3 + 0], to[i * 3 + 1], to[i * 3 + 2]), btVector3(to[(i+1) * 3 + 0], to[(i+1) * 3 + 1], to[(i+1) * 3 + 2]), btVector3(to[(i+2) * 3 + 0], to[(i+2) * 3 + 1], to[(i+2) * 3 + 2]));
	if(not collider->collisionShape)
	{
		collider->collisionShape = new btBvhTriangleMeshShape(collider->data.bvhTriangleMesh.collisionTriangleMesh, true, true);
		//collider->collisionShape->setMargin(0.1);
		collider->collisionShape->setUserPointer(new uint32_t(colliderID));
	}
}

PUBLIC_C_VISIBILITY
void KGPhysicsManagerSetColliderHullVertexPositions(KGPhysicsManager* self, KGPhysicsManagerColliderID colliderID, const float positions[][3], uint32_t count)
{
	Collider* collider = & self->colliders[colliderID - 1];
	if(collider->type == ColliderTypeNone) 
	{
		collider->type = ColliderTypeHull;
		collider->collisionShape = new btConvexHullShape();
		for (uint32_t i = 0; i < count; i++)
			((btConvexHullShape*)collider->collisionShape)->addPoint(btVector3(positions[i][0], positions[i][1], positions[i][2]));
		collider->collisionShape->setUserPointer(new uint32_t(colliderID));
	}
	else if(collider->type not_eq ColliderTypeHull) abort();
	else abort();
}

PUBLIC_C_VISIBILITY
KGPhysicsManagerEventType KGPhysicsManagerPollEvent(KGPhysicsManager* self, KGPhysicsManagerEventData* data)
{
	if(self->eventQueue.empty()) return KGPhysicsManagerEventTypeNone;
	const Event event = self->eventQueue.front();
	*data = event.data;
	self->eventQueue.pop();
	return event.type;
}

namespace 
{
class btKinematicClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
        btKinematicClosestNotMeRayResultCallback (btVector3 from, btVector3 to, btCollisionObject* me) : btCollisionWorld::ClosestRayResultCallback(from, to)
        {
                m_me = me;
        }

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
        {
                if (rayResult.m_collisionObject == m_me)
                        return 1.0;

                return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
        }
protected:
        btCollisionObject* m_me;
};
}

PUBLIC_C_VISIBILITY
KGPhysicsManagerBodyID KGPhysicsManagerGetClosestBodyAlongRay(KGPhysicsManager* self, const float from[3], const float to[3], float hitPosition[3], float hitNormal[3], KGPhysicsManagerBodyID excludedBodyID)
{
	abort();
	btCollisionObject* excludedRigidBody = (btCollisionObject*) excludedBodyID ? (self->rigidBodies[excludedBodyID - 1]) : NULL;
	btKinematicClosestNotMeRayResultCallback callback(btVector3(from[0], from[1], from[2]), btVector3(to[0], to[1], to[2]), excludedRigidBody);
	self->dynamicsWorld->rayTest(btVector3(from[0], from[1], from[2]), btVector3(to[0], to[1], to[2]), callback);
	if(callback.m_collisionObject) 
	{
		for (int i = 0; i < 3; i++) hitNormal[i] = callback.m_hitNormalWorld[i];
		for (int i = 0; i < 3; i++) hitPosition[i] = callback.m_hitPointWorld[i];
		return ((RigidBodyUserdata*)callback.m_collisionObject->getUserPointer())->bodyID;
	}
	return 0;
}

#endif
