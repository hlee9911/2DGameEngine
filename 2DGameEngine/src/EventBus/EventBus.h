#pragma once
#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "../Logger/Logger.h"
#include "../EventBus/Event.h"

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <typeindex>
#include <memory>
#include <list>

class IEventCallback
{
public:
	virtual ~IEventCallback() noexcept = default;

	void Invoke(Event& e) noexcept
	{
		// invoke the callback function
		Call(e);
	}

private:

	virtual void Call(Event& e) noexcept = 0;

};

// wrapper around the function pointer
template<typename TOwner, typename TEvent>
class EventCallback : public IEventCallback
{
private:
	typedef void(TOwner::*CallbackFunction)(TEvent&);

	TOwner* m_ownerInstance;
	CallbackFunction m_callbackFunction;

	virtual void Call(Event& e) noexcept override
	{
		std::invoke(m_callbackFunction, m_ownerInstance, static_cast<TEvent&>(e));
	}

public:

	EventCallback(TOwner* ownerInstance, CallbackFunction callbackFunction) noexcept
		: m_ownerInstance(ownerInstance), m_callbackFunction(callbackFunction) {}

	virtual ~EventCallback() noexcept override = default;

};

typedef std::list<std::unique_ptr<IEventCallback>> HandlerList;

class EventBus
{
public:

	EventBus() noexcept  { Logger::Log("EventBus constructor called"); }

	~EventBus() noexcept { Logger::Log("EventBus destructor called"); }

	/// <summary>
	/// Clear all subscribers
	/// </summary>
	void Reset() noexcept
	{
		m_subscribers.clear();
	}

	/// <summary>
	/// Subscribe to an event, a listener will be added to the event
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// example: eventBus->SubscribeEvent<CollisionEvent>(this, &Game::OnCollision);
	template<typename TEvent, typename TOwner>
	void SubscribeEvent(TOwner* ownerInstance, void(TOwner::*callbackFunction)(TEvent&)) noexcept
	{
		if (!m_subscribers[typeid(TEvent)].get())
		{
			// create a new list of handlers for the event
			m_subscribers[typeid(TEvent)] = std::make_unique<HandlerList>();
		}
		auto subscriber = std::make_unique<EventCallback<TOwner, TEvent>>(ownerInstance, callbackFunction);
		m_subscribers[typeid(TEvent)]->push_back(std::move(subscriber));
	}

	/// <summary>
	/// Unsubscribe from an event, a listener will be removed from the event
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename TEvent>
	void UnsubscribeEvent() noexcept
	{
		if (m_subscribers[typeid(TEvent)].get())
		{
			m_subscribers[typeid(TEvent)]->clear();
		}
	}

	/// <summary>
	/// Publish an event, all listeners will be notified
	/// </summary>
	/// example: eventBus->PublishEvent<CollisionEvent>(entityA, entityB);
	template<typename TEvent, typename... TArgs>
	void PublishEvent(TArgs&&... args) noexcept
	{
		auto handlers = m_subscribers[typeid(TEvent)].get();
		if (handlers)
		{
			for (auto it = handlers->begin(); it != handlers->end(); it++)
			{
				auto handler = it->get();
				TEvent event(std::forward<TArgs>(args)...);
				handler->Invoke(event);
			}
		}
	}

private:

	// a map to hold a list of handlers for a specific event
	std::map<std::type_index, std::unique_ptr<HandlerList>> m_subscribers;

};

#endif // EVENTBUS_H
