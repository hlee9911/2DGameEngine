#pragma once
#ifndef EVENTS_H
#define EVENTS_H

#include "../ECS/ECS.h"
#include "../EventBus/Event.h"

#include <SDL.h>
#include <SDL_image.h>
#include <vector>

class CollisionEvent : public Event
{
public:
	Entity m_entityA;
	Entity m_entityB;

	CollisionEvent(Entity& entityA, Entity& entityB) noexcept
		: m_entityA(entityA), m_entityB(entityB) {}

};

class KeyPressedEvent : public Event
{
public:
	SDL_Keycode m_keyCode;

	KeyPressedEvent(SDL_Keycode keyCode) noexcept
		: m_keyCode(keyCode) {}

};

//class OnEntityDestroyedEvent : public Event
//{
//public:
//	Entity m_entity;
//	EventBus m_eventBus;
//
//	OnEntityDestroyedEvent(Entity entity, EventBus eventBus) noexcept
//		: m_entity(entity), m_eventBus(eventBus) {}
//
//};

//class ShootProjectileEvent : public Event
//{
//public:
//	Entity m_entity;
//
//	ShootProjectileEvent(Entity& entity) noexcept
//		: m_entity(entity) {}
//
//};

#endif // EVENTS_H
