#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/CameraSceneNode.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPC.h"
#include "PrimeEngine/GameObjectModel/PhysicsManager.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl{

// Events sent by behavior state machine (or other high level state machines)
// these are events that specify where a soldier should move
namespace Events{

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_MOVE_TO, Event);

SoldierNPCMovementSM_Event_MOVE_TO::SoldierNPCMovementSM_Event_MOVE_TO(Vector3 targetPos /* = Vector3 */)
: m_targetPosition(targetPos)
{ }

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_STOP, Event);

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_TARGET_REACHED, Event);
}

namespace Components{

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM, Component);


SoldierNPCMovementSM::SoldierNPCMovementSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself) 
: Component(context, arena, hMyself)
, m_state(STANDING)
{}

SceneNode *SoldierNPCMovementSM::getParentsSceneNode()
{
	PE::Handle hParent = getFirstParentByType<Component>();
	if (hParent.isValid())
	{
		// see if parent has scene node component
		return hParent.getObject<Component>()->getFirstComponent<SceneNode>();
		
	}
	return NULL;
}

void SoldierNPCMovementSM::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_MOVE_TO, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO);
	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_STOP, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP);
	
	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCMovementSM::do_UPDATE);
}

void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO(PE::Events::Event *pEvt)
{
	SoldierNPCMovementSM_Event_MOVE_TO *pRealEvt = (SoldierNPCMovementSM_Event_MOVE_TO *)(pEvt);
	
	// change state of this state machine
	m_state = WALKING_TO_TARGET;
	m_targetPostion = pRealEvt->m_targetPosition;

	// make sure the animations are playing
	
	PE::Handle h("SoldierNPCAnimSM_Event_WALK", sizeof(SoldierNPCAnimSM_Event_WALK));
	Events::SoldierNPCAnimSM_Event_WALK *pOutEvt = new(h) SoldierNPCAnimSM_Event_WALK();
	
	SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
	pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

	// release memory now that event is processed
	h.release();
}

void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP(PE::Events::Event *pEvt)
{
	Events::SoldierNPCAnimSM_Event_STOP Evt;

	SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
	pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&Evt);
}

void SoldierNPCMovementSM::do_UPDATE(PE::Events::Event *pEvt)
{
	if (m_state == WALKING_TO_TARGET)
	{
		// see if parent has scene node component
		SceneNode *pSN = getParentsSceneNode();
		if (pSN)
		{
			

			Vector3 curPos = pSN->m_base.getPos();
			//Vector3 nextPoint;
			//Pathfinding::ConvexPolygon* p = new Pathfinding::ConvexPolygon();

		//	p->createPolygon();
			//int n = 0 + (rand() % (int)(10 - 0 + 1)); // Hardcoded index value for its movement .. Remove this when you get values of polygons dynamicaaly!!!
			//Vector3* conv = p->all_polys.at(n)->vertices.at(0);

			//float x = conv->m_x;
			//float y = conv->m_y;
			//float z = conv->m_z;

			//m_targetPostion = Vector3(x,y,z);

			//Physics Implementation
			Event_UPDATE *pRealEvt = (Event_UPDATE *)(pEvt);
			static float speed = 2.5f;
			PE::Handle hPM("PHYSICS_MANAGER", sizeof(PhysicsManager));
			PhysicsManager *pMainPM = new(hPM)PhysicsManager(*m_pContext, m_arena, hPM);

			pMainPM->PrePhysics(curPos, m_targetPostion, speed* pRealEvt->m_frameTime);
			curPos = pMainPM->UpdatePhysics();

			float dsqr = (m_targetPostion - curPos).lengthSqr();

			bool reached = true;
			if (dsqr > 0.01f)
			{
				// not at the spot yet
				Event_UPDATE *pRealEvt = (Event_UPDATE *)(pEvt);
				static float speed = 1.4f;
				float allowedDisp = speed * pRealEvt->m_frameTime;

				Vector3 dir = (m_targetPostion - curPos);
				dir.normalize();
				float dist = sqrt(dsqr);
				if (dist > allowedDisp)
				{
					dist = allowedDisp; // can move up to allowedDisp
					reached = false; // not reaching destination yet
				}

				// instantaneous turn
				pSN->m_base.turnInDirection(dir, 3.1415f);
				pSN->m_base.setPos(curPos + dir * dist);
			}

			if (reached)
			{
				m_state = STANDING;
				
				// target has been reached. need to notify all same level state machines (components of parent)
				{
					PE::Handle h("SoldierNPCMovementSM_Event_TARGET_REACHED", sizeof(SoldierNPCMovementSM_Event_TARGET_REACHED));
					Events::SoldierNPCMovementSM_Event_TARGET_REACHED *pOutEvt = new(h) SoldierNPCMovementSM_Event_TARGET_REACHED();

					PE::Handle hParent = getFirstParentByType<Component>();
					if (hParent.isValid())
					{
						hParent.getObject<Component>()->handleEvent(pOutEvt);
					}
					
					// release memory now that event is processed
					h.release();
				}

				if (m_state == STANDING)
				{
					// no one has modified our state based on TARGET_REACHED callback
					// this means we are not going anywhere right now
					// so can send event to animation state machine to stop
					{
						Events::SoldierNPCAnimSM_Event_STOP evt;
						
						SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
						pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&evt);
					}
				}
			}
		}
	}
}

}}



