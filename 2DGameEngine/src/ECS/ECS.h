#pragma once
#ifndef ECS_H
#define ECS_H

#include "../Logger/Logger.h"
#include "../AssetStore/AssetStore.h"
#include "../EventBus/EventBus.h"
#include "../Components/Components.h"

#include <vector>
#include <bitset>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <utility>
#include <memory>
#include <deque>
#include <iostream>

#include <SDL.h>
#include <SDL_image.h>

namespace
{
	constexpr unsigned int MAX_COMPONENTS = 32;
}

/// <summary>
/// We use a bitset (1s and 0s) to keep track of which components an entity has.
/// and also helps keep track of which entities a system is interested in.
/// </summary>
typedef std::bitset<MAX_COMPONENTS> Signature;

// Base class for all components - similar to interface
struct IComponent
{
protected:
	// component will also have an id
	static int nextId;
};

// Used to assign a unique id to a different component type class (class template)
template <typename T>
class Component : public IComponent
{
public:
	// Returns the unique id of the Component<T>
	static int GetID() noexcept
	{
		static auto id = nextId++;
		return id;
	}
};

class Entity
{
public:

	// constructor
	constexpr Entity(int id) noexcept : m_id(id), m_registry(nullptr) {}

	// default copy constructor
	constexpr Entity(const Entity& entity) noexcept = default;

	// default assignment operator
	constexpr Entity& operator= (const Entity& other) noexcept = default;

	void Destroy() noexcept;
	
	inline int GetID() const noexcept { return m_id; }

	// Manage entity tags and groups
	void Tag(const std::string& tag) noexcept;
	bool HasTag(const std::string& tag) const noexcept;
	void Group(const std::string& group) noexcept;
	bool BelongsToGroup(const std::string& group) const noexcept;
	
	bool operator ==(const Entity& other) const noexcept { return m_id == other.m_id; }
	bool operator !=(const Entity& other) const noexcept { return m_id != other.m_id; }
	bool operator >(const Entity& other) const noexcept { return m_id > other.m_id; }
	bool operator <(const Entity& other) const noexcept { return m_id < other.m_id; }

	template<typename TComponent, typename... TArgs> void AddComponent(TArgs&&... args) noexcept;
	template<typename TComponent> void RemoveComponent() noexcept;
	template<typename TComponent> bool HasComponent() const noexcept;
	template<typename TComponent> TComponent& GetComponent() const noexcept;

	// Hold a pointer to the entity's owner registry
	class Registry* m_registry;

private:

	int m_id;

};

// System class
// System process entities that contain specific signature (set of 1 or 0s to determine if componenets is enabled or disabled on a certain entity)
class System
{
public:

	System() noexcept = default;
	virtual ~System() noexcept = default;

	void AddEntityToSystem(Entity entity) noexcept;
	void RemoveEntityFromSystem(Entity entity) noexcept;
	
	inline std::vector<Entity>& GetSystemEntities() noexcept { return m_entities; }
	inline const size_t GetSystemEntitiesSize() const noexcept { return m_entities.size(); }
	inline const Signature& GetComponentSignature() const noexcept { return m_componentSignature; }

	// Defines the component type TComponent that entities must have to be considered by the system
	template<typename TComponent> void RequireComponent() noexcept;

	virtual void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept = 0;
	virtual void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, 
						SDL_Rect& camera, std::unique_ptr<Registry>& registry,
						std::unique_ptr<AssetStore>& assetStore,
						SDL_Renderer* renderer) noexcept = 0;
	virtual void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, 
						SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode = false) noexcept = 0;

private:

	Signature m_componentSignature;
	std::vector<Entity> m_entities;

};

// interface for all pools
class IPool
{
public:
	// virtual destructor so that derived classes can be deleted through a pointer to the base class
	virtual ~IPool() noexcept = default;
	virtual void RemoveEntityFromPool(int entityId) noexcept = 0;
};

// A pool is a just a vector (contiguous data) of object of type T
// Need to specify the type, so we use IPool instead of Pool
template <typename T>
class Pool : public IPool
{
public:

	constexpr Pool(int capacity = 100) noexcept 
	{
		m_data.resize(capacity);
		m_size = 0;
	}
	virtual ~Pool() noexcept = default;

	inline bool IsEmpty() const noexcept { return m_size == 0; }
	inline int GetSize() const noexcept { return m_size; }
	inline void Resize(int n) noexcept { m_data.resize(n); }
	inline void Clear() noexcept { m_data.clear(); m_size = 0; }

	inline void Add(T object) noexcept { m_data.emplace_back(object); }

	void Set(int entityId, T object) noexcept 
	{ 
		if (m_entityIdToIndex.find(entityId) != m_entityIdToIndex.end())
		{
			// if the entity id already exists, then just update the object
			int index = m_entityIdToIndex[entityId];
			m_data[index] = object;
		}
		else
		{
			// when adding a new object, we keep track of the entity id and the index
			int index = m_size;
			m_entityIdToIndex.emplace(entityId, index);
			m_indexToEntityId.emplace(index, entityId);
			if (index >= m_data.capacity())
			{
				// if necessary, resize the vector
				m_data.resize(m_size * 2);
			}
			m_data[index] = object;
			m_size++;
		}
	}

	void Remove(int entityId) noexcept
	{
		// copy the last element to the deleted position to keep the array packed
		int indexOfRemoved = m_entityIdToIndex[entityId];
		int indexOfLast = m_size - 1;
		m_data[indexOfRemoved] = m_data[indexOfLast];

		// Update the index-entity maps to point to the correct elements
		int entityIdOfLastElement = m_indexToEntityId[indexOfLast];
		m_entityIdToIndex[entityIdOfLastElement] = indexOfRemoved;
		m_indexToEntityId[indexOfRemoved] = entityIdOfLastElement;

		// Remove the entity id from the maps
		m_entityIdToIndex.erase(entityId);
		m_indexToEntityId.erase(indexOfLast);

		m_size--;
	}

	void RemoveEntityFromPool(int entityId) noexcept override
	{
		if (m_entityIdToIndex.find(entityId) != m_entityIdToIndex.end())
			Remove(entityId);
	}

	inline T& Get(int entityId) noexcept 
	{ 
		int index = m_entityIdToIndex[entityId];
		return static_cast<T&>(m_data[index]); 
	}
	
	T& operator[] (unsigned int index) noexcept { return m_data[index]; }

private:

	// we keep track of the vector of component objects and their correct size
	std::vector<T> m_data;
	int m_size;

	// map to keep track of entity ids per index so the vector is always packed
	std::unordered_map<int, int> m_entityIdToIndex;
	std::unordered_map<int, int> m_indexToEntityId;

};

// Entity Manager or world class
// Manages the creation and destruction of entities, add systems, and components
class Registry
{
public:

	// Registry() noexcept = default;

	Registry(/*int componentPoolSize = 256*//*, int componentSignatureSize = 256*/) noexcept 
	{ 
		// m_componentPools.reserve(componentPoolSize); 
		// m_entityComponentSignatures.reserve(componentSignatureSize);
		Logger::Log("Registry constructor called");
	}

	~Registry() noexcept
	{
		Logger::Log("Registry destructor called");
	}

	// Entity Management
	Entity CreateEntity() noexcept;
	void DestroyEntity(Entity entity) noexcept;

	// Component Management
	template<typename TComponent, typename... TArgs> void AddComponent(Entity entity, TArgs&&... args) noexcept;
	template<typename TComponent> void RemoveComponent(Entity entity) noexcept;
	template<typename TComponent> bool HasComponent(Entity entity) const noexcept;
	template<typename TComponent> TComponent& GetComponent(Entity entity) const noexcept;
	// GetComponent(Entity entity)

	// System Management
	template<typename TSystem, typename... TArgs> void AddSystem(TArgs&&... args) noexcept;
	template<typename TSystem> void RemoveSystem() noexcept;
	template<typename TSystem> bool HasSystem() const noexcept;
	template<typename TSystem> TSystem& GetSystem() const noexcept;

	// Tag management
	void TagEntity(Entity entity, const std::string& tag) noexcept;
	bool EntityHasTag(Entity entity, const std::string& tag) const noexcept;
	Entity GetEntityByTag(const std::string& tag) const noexcept;
	void RemoveEntityTag(Entity entity) noexcept;

	// Group management
	void GroupEntity(Entity entity, const std::string& group) noexcept;
	bool EntityBelongsToGroup(Entity entity, const std::string& group) const noexcept;
	std::vector<Entity> GetEntitiesByGroup(const std::string& group) const noexcept;
	void RemoveEntityGroup(Entity entity) noexcept;
	 
	// Checks the component signature of an entity and add the entity to the systems 
	// that are interested in that signature
	void AddEntityToSystems(Entity entity) noexcept;
	// Checks the component signature of an entity and remove the entity from the systems
	void RemoveEntityFromSystems(Entity entity) noexcept;

	// iterate through all the system and subscribe to their events
	void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus) noexcept;

	// Here is where we actually insert/delete the entities that are waiting to be added/removed
	// We do this because we don't want to confuse our Systems by adding/removing entities in the middle
	// of the frame logic. Therefore, we will wait until the end of the frame to perate and perform the
	// the creation and the deletion of entities 
	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, 
				std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore,
				SDL_Renderer* renderer) noexcept;
	// Render all the systems
	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, 
				SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode = false) noexcept;

private:

	// keep track of how many entities were added to the scene
	int m_numEntities = 0;

	// vector of component pools
	// each pool contains all the data for a certain component type
	// [vector index = componentID (componentType)], 
	// [pool index = entityID]
	std::vector<std::shared_ptr<IPool>> m_componentPools;

	// Vector of component signatures per entity
	// the signature lets us know which components are turned "on" for an entity
	// [vector index = entity id]
	std::vector<Signature> m_entityComponentSignatures;

	// Map of active system 
	// [unordered map index (key) = system typeID]
	std::unordered_map<std::type_index, std::shared_ptr<System>> m_systems;

	// These are to avoid adding and removing entities while updating the registry (will be added at the end of the game loop)
	std::set<Entity> m_entitiesToBeAdded; // Entities awating creation in the next Registry Update()
	std::set<Entity> m_entitiesToBeKilled; // Entities awating destruction in the next Registry Update()

	// Entity tags (one tag name per entity)
	std::unordered_map<std::string, Entity> m_entityPerTag;
	std::unordered_map<int, std::string> m_tagPerEntity;

	// Entity groups (a set of entities per group name)
	std::unordered_map<std::string, std::set<Entity>> m_entitiesPerGroup;
	std::unordered_map<int, std::string> m_groupPerEntity;
	
	// deque of available free entity ids that were previously removed
	std::deque<int> m_freeEntityIDs; 
};

/////////////// Pool Template Methods Implementation ///////////////


/////////////// System Template Methods Implementation ///////////////

/// <summary>
/// Used for the system to specify which components it is interested in.
/// For template funcitons, the implementation must be in the header file.
/// </summary>
/// <typeparam name="TComponent"></typeparam>
template <typename TComponent>
void System::RequireComponent() noexcept
{
	const auto componentId = Component<TComponent>::GetID();
	m_componentSignature.set(componentId);
}

/////////////// Registry System Management Methods Implementation ///////////////

// Add a system to the registry
template<typename TSystem, typename... TArgs>
void Registry::AddSystem(TArgs&&... args) noexcept
{
	// Create a new system object of type TSystem, and forward the various parameters to the constructor
	std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);

	// Add the new system to the systems map
	// key: typeid(TSystem) (type_index)
	// value: newSystem (System*)
	m_systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template<typename TSystem>
void Registry::RemoveSystem() noexcept
{
	// Find the system in the systems map
	auto system = m_systems.find(std::type_index(typeid(TSystem)));
	// If the system is found, then delete it
	if (system != m_systems.end()) 
		m_systems.erase(system);
}

template<typename TSystem>
bool Registry::HasSystem() const noexcept
{
	// Find the system in the systems map
	// If the system is found, then return true
	// If the system is not found, then return false
	return m_systems.find(std::type_index(typeid(TSystem))) != m_systems.end();
}

template<typename TSystem>
TSystem& Registry::GetSystem() const noexcept
{
	auto system = m_systems.find(std::type_index(typeid(TSystem)));

	// If the system is found, then return it
	// if (system != m_systems.end())
	return *(std::static_pointer_cast<TSystem>(system->second));
	//else 
	//	return NULL;
}


/////////////// Registry Component Management Template Methods Implementation ///////////////

template <typename TComponent, typename... TArgs>
void Registry::AddComponent(Entity entity, TArgs&&... args) noexcept
{
	const auto componentId = Component<TComponent>::GetID();
	const auto entityId = entity.GetID();

	// If the component id is greater than the current size of the componentPools, then resize the vector
	// this resizing is very expensive, so we should try to avoid it
	if (componentId >= m_componentPools.size())
	{
		m_componentPools.resize(componentId + 1, nullptr);
	}

	// If we still don't have a Pool for that component Type,
	if (m_componentPools.size() <= componentId ||
		(m_componentPools.size() > componentId && !m_componentPools[componentId]))
	{
		// Create a new Pool for that component type
		std::shared_ptr<Pool<TComponent>> newComponentPool = std::make_shared<Pool<TComponent>>();
		// Add the new Pool to the componentPools vector
		m_componentPools[componentId] = newComponentPool;
		// m_componentPools.emplace_back(newComponentPool);
	}

	// Get the pool of component values for that component type
	std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(m_componentPools[componentId]);
	// std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(m_componentPools.back());
	// Pool<TComponent>* componentPool = m_componentPools[componentId];

	// if the entity id isi greater than the current size of the component pool, then resize the pool
	//if (entityId >= componentPool->GetSize())
	//{
	//	componentPool->Resize(entityId + 1);
	//}

	// Create a new component object of thje type TComponent, and forward the various parameters to the constructor
	TComponent newComponent(std::forward<TArgs>(args)...);

	// Add the enw component to the pool list, using the entity id as the index
	// componentPool->Add(newComponent);
	componentPool->Set(entityId, newComponent);

	// Finally, change the component signature of the entity and set the componenet id on the bitset to 1
	m_entityComponentSignatures[entityId].set(componentId);

	// Logger::Log("Component id = " + std::to_string(componentId) + " was added to entity id " + std::to_string(entityId));

	Logger::Log("Component id = " + std::to_string(componentId) + " --> POOL SIZE: " + std::to_string(componentPool->GetSize()));
}

/// <summary>
/// Ask RemoveComponent<TComponent> from an entity 
/// </summary>
/// <typeparam name="TComponent"></typeparam>
/// <param name="entity"></param>
template <typename TComponent>
void Registry::RemoveComponent(Entity entity) noexcept
{
	const auto componentId = Component<TComponent>::GetID();
	const auto entityId = entity.GetID();

	// Get the pool of component values for that component type
	std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(m_componentPools[componentId]);
	// Remove the component from the the component list for that entity
	componentId.Remove(entityId);
	
	// set this comonent signature for that entity to false
	m_entityComponentSignatures[entityId].set(componentId, false);
	
	Logger::Log("Component id = " + std::to_string(componentId) + " was removed from entity id " + std::to_string(entityId));
}

/// <summary>
/// Checks if an entity has a component of type TComponent
/// </summary>
/// <typeparam name="TComponent"></typeparam>
/// <param name="entity"></param>
template <typename TComponent>
bool Registry::HasComponent(Entity entity) const noexcept
{
	const auto componentId = Component<TComponent>::GetID();
	const auto entityId = entity.GetID();

	// checks and returns if signature has the component id set to 1 or 0
	return m_entityComponentSignatures[entityId].test(componentId);
}

/// <summary>
/// Gets the component of type TComponent from an entity
/// </summary>
/// <typeparam name="TComponent"></typeparam>
/// <param name="entity"></param>
template <typename TComponent>
TComponent& Registry::GetComponent(Entity entity) const noexcept
{
	const auto componentId = Component<TComponent>::GetID();
	const auto entityId = entity.GetID();

	// Get the pool of component values for that component type
	auto componentPool = std::static_pointer_cast<Pool<TComponent>>(m_componentPools[componentId]);

	// Get the component from the pool
	return componentPool->Get(entityId);
}


/////////////// Entity Registry Management Methods Implementation ///////////////

template <typename TComponent, typename... TArgs>
void Entity::AddComponent(TArgs&&... args) noexcept
{
	// Ask the registry to add the component to the entity
	m_registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::RemoveComponent() noexcept
{
	// Ask the registry to remove the component from the entity
	m_registry->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const noexcept
{
	// Ask the registry if the entity has the component
	return m_registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::GetComponent() const noexcept
{
	// Ask the registry to get the component from the entity
	return m_registry->GetComponent<TComponent>(*this);
}

#endif // ECS_H

// ECS Design Notes

// Entity Component System
// Entity - ID
// Component - Data
// System - Logic

// Example
// Entity - Player
// Component - Transform, Sprite, Collider, Health, Score (list of components)
// System - Movement, Rendering, Collision, Health, Score

// Entity - Enemy
// Component - Transform, Sprite, ProjectileEmitter, Health, Score (list of components)
// System - Movement, Rendering, Collision, Health, Score

// Entity - Door Trigger
// Component - Transform, Collider, (list of components)

// Entity Class
// vector of components
// addComponent()
// getComponent()
// removeComponent()
// Update(deltaTime)
// Render()

// Component Class
// Entity* owner
// virtual Update(deltaTime)
// virtual Render()

// Registry Class (Entity Manager)
// vector of entities
// createEntity(entity)
// destroyEntity(entity)
// update(deltaTime)
// render()

// class cardinality
// means how many of a class can be created



// ECS Designs

// Entities
// - simply an ID
// - they represent the objects that populate your game scene

// Components
// - components are pure data (contain no logic)
// - they are organized the data itself rather than by entity
//   * This organiation is the key difference between OOP and data-oriented design
// - sequences of components are used to represent entities

// Systems
// - systems are the logic that transform components from one state to next state
// - for example, a MovementSystem might update the position of all moving entities by their velocity since the previous frame


// code snippets of ECS
// Entities -> just an ID
// Components -> pure data
// Systems -> perform logic on components and entities
// - RequireComponent -> a system that requires a component to be present on an entity
// - certain system interested in certain entities
