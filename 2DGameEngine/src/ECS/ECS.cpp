#include "ECS.h"

#include <algorithm>
#include <string>

// define static variables
int IComponent::nextId = 0;

/////////////// Entity class implementations ///////////////

/// <summary>
/// Destroy the entity
/// </summary>
/// <param name="entity"></param>
/// <returns></returns>
void Entity::Destroy() noexcept
{
	// Logger::Log("Entity with id: " + std::to_string(m_id) + " destroyed");
	m_registry->DestroyEntity(*this);
}

void Entity::Tag(const std::string& tag) noexcept
{
	m_registry->TagEntity(*this, tag);
}

bool Entity::HasTag(const std::string& tag) const noexcept
{
	return m_registry->EntityHasTag(*this, tag);
}

void Entity::Group(const std::string& group) noexcept
{
	m_registry->GroupEntity(*this, group);
}

bool Entity::BelongsToGroup(const std::string& group) const noexcept
{
	return m_registry->EntityBelongsToGroup(*this, group);
}

/////////////// System class implementations ///////////////

/// <summary>
/// Adds an entity to the system
/// </summary>
/// <param name="entity"></param>
void System::AddEntityToSystem(Entity entity) noexcept
{
	m_entities.emplace_back(entity);
}

/// <summary>
/// Remove the specified entity from the system
/// </summary>
/// <param name="entity"></param>
void System::RemoveEntityFromSystem(Entity entity) noexcept
{
	m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(), [&entity](Entity other) 
		{
			return entity == other;
		}), m_entities.end());
}

/////////////// Registry class implementations ///////////////

/// <summary>
/// Creates an entity, add it to the toBeAdded set and returns it
/// </summary>
/// <returns></returns>
Entity Registry::CreateEntity() noexcept
{
	int entityId;

	if (m_freeEntityIDs.empty())
	{
		// if there are no free entity ids, create a new one
		entityId = m_numEntities++;
		// make sure the entityComponentSignatures vector is big enough to hold the new entity
		// is this code necessary?
		if (entityId >= static_cast<int>(m_entityComponentSignatures.size()))
			m_entityComponentSignatures.resize(entityId + 1);
	}
	else
	{
		// Reuse an entity id from the free entity ids
		entityId = m_freeEntityIDs.front();
		m_freeEntityIDs.pop_front();
	}

	Entity entity(entityId);
	entity.m_registry = this;
	m_entitiesToBeAdded.insert(entity);

	Logger::Log("Entity created with id: " + std::to_string(entityId));

	return entity;
}

void Registry::DestroyEntity(Entity entity) noexcept
{
	m_entitiesToBeKilled.insert(entity);
};

// Responsible getting the entity and comparing the signature to the system signature
void Registry::AddEntityToSystems(Entity entity) noexcept
{
	const auto entityId = entity.GetID();

	// TODO: match entityComponentSignature <--> systemComponentSignature
	const auto& entityComponentSignature = m_entityComponentSignatures[entityId];

	// Loop all the systems
	for (auto& system : m_systems)
	{
		const auto& systemComponentSignature = system.second->GetComponentSignature();
	
		// Check if the system is interested in the entity
		// using bitwise AND
		// if this returns true, then the system is interested in the entity since the entity has all the components the system is interested in
		bool isIntersted = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;
		
		if (isIntersted)
			system.second->AddEntityToSystem(entity);
	}
}

// Responsible getting the entity and comparing the signature to the system signature
// and remove the entity from the system
void Registry::RemoveEntityFromSystems(Entity entity) noexcept
{
	for (auto& system : m_systems)
	{
		system.second->RemoveEntityFromSystem(entity);
	}

	// TODO: need to remove all the components from the entity to be removed?
}

void Registry::TagEntity(Entity entity, const std::string& tag) noexcept
{
	m_entityPerTag.emplace(tag, entity);
	m_tagPerEntity.emplace(entity.GetID(), tag);
}

bool Registry::EntityHasTag(Entity entity, const std::string& tag) const noexcept
{
	if (m_tagPerEntity.find(entity.GetID()) == m_tagPerEntity.end())
	{
		return false;
	}
	return m_entityPerTag.find(tag)->second == entity;
}

Entity Registry::GetEntityByTag(const std::string& tag) const noexcept
{
	return m_entityPerTag.at(tag);
}

void Registry::RemoveEntityTag(Entity entity) noexcept
{
	auto taggedEntity = m_tagPerEntity.find(entity.GetID());
	if (taggedEntity != m_tagPerEntity.end())
	{
		auto tag = taggedEntity->second;
		m_entityPerTag.erase(tag);
		m_tagPerEntity.erase(taggedEntity);
	}
}

void Registry::GroupEntity(Entity entity, const std::string& group) noexcept
{
	m_entitiesPerGroup.emplace(group, std::set<Entity>());
	m_entitiesPerGroup[group].emplace(entity);
	m_groupPerEntity.emplace(entity.GetID(), group);
}

bool Registry::EntityBelongsToGroup(Entity entity, const std::string& group) const noexcept
{
	/*if (m_groupPerEntity.find(entity.GetID()) == m_groupPerEntity.end())
	{
		return false;
	}
	return m_groupPerEntity.find(entity.GetID())->second == group;*/
	if (m_entitiesPerGroup.find(group) == m_entitiesPerGroup.end()) 
	{
		return false;
	}
	auto groupEntities = m_entitiesPerGroup.at(group);
	return groupEntities.find(entity.GetID()) != groupEntities.end();
}

std::vector<Entity> Registry::GetEntitiesByGroup(const std::string& group) const noexcept
{
	auto& setOfEntities = m_entitiesPerGroup.at(group);
	return std::vector<Entity>(setOfEntities.begin(), setOfEntities.end());
}

void Registry::RemoveEntityGroup(Entity entity) noexcept
{
	// if in group, remove entity from group management
	auto groupedEntity = m_groupPerEntity.find(entity.GetID());
	if (groupedEntity != m_groupPerEntity.end())
	{ 
		auto group = m_entitiesPerGroup.find(groupedEntity->second);
		if (group != m_entitiesPerGroup.end())
		{
			auto entityInGroup = group->second.find(entity.GetID());
			if (entityInGroup != group->second.end())
			{
				group->second.erase(entityInGroup);
			}
		}
		m_groupPerEntity.erase(groupedEntity);
	}
}

/// <summary>
/// Iterates through all active systems and calls their SubscribeToEvent function
/// </summary>
/// <param name="eventBus"></param>
void Registry::SubscribeToEvents(std::unique_ptr<EventBus>& eventBus) noexcept
{
	for (auto& system : m_systems)
	{
		system.second->SubscribeToEvent(eventBus);
	}
}

/// <summary>
/// Iterates through all active systems and calls their update function
/// </summary>
/// <param name="deltaTime"></param>
void Registry::Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, 
					  std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept
{
	// Add the entities that are waiting to be created to the active Systems
	for (auto& entity : m_entitiesToBeAdded)
	{
		AddEntityToSystems(entity);
	}
	m_entitiesToBeAdded.clear();
		
	// Update all the active Systems
	for (const auto& system : m_systems)
	{
		system.second->Update(deltaTime, eventBus, camera, registry, assetStore, renderer);
	}

	// Remove the entities that are waiting to be removed from the active Systems
	for (auto& entity : m_entitiesToBeKilled)
	{
		/*if (entity.HasComponent<TransformComponent>())
		{
			eventBus->PublishEvent<OnEntityDestroyedEvent>(entity, eventBus);
		}*/

		RemoveEntityFromSystems(entity);

		// Reset the entity signature for the entity that is being killed
		m_entityComponentSignatures[entity.GetID()].reset();
		
		// remove the entity from the component pool
		for (auto& componentPool : m_componentPools)
		{
			if (componentPool)
				componentPool->RemoveEntityFromPool(entity.GetID());
		}

		// Make the entity id available again
		m_freeEntityIDs.push_back(entity.GetID());

		RemoveEntityTag(entity);
		RemoveEntityGroup(entity);
	}
	m_entitiesToBeKilled.clear();
}

/// <summary>
/// Iterates through all active systems and calls their render function
/// </summary>
/// <param name="renderer"></param>
void Registry::Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept
{
	// Render all the active Systems
	for (const auto& system : m_systems)
	{
		system.second->Render(renderer, assetStore, camera, registry, isDebugMode);
	}
}