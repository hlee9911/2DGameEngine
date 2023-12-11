#pragma once
#ifndef SYSTEMS_H
#define SYSTEMS_H

#include <SDL.h>
#include <SDL_image.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>
#include <memory>
#include <algorithm>
#include <stdint.h>

#include "../GameEngine/Game.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Logger/Logger.h"
#include "../AssetStore/AssetStore.h"
#include "../Events/Events.h"
#include "../EventBus/EventBus.h"

class MovementSystem : public System
{
public:

	MovementSystem() noexcept
	{
		RequireComponent<TransformComponent>();
		RequireComponent<SpriteComponent>();
		RequireComponent<RigidbodyComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{
		eventBus->SubscribeEvent<CollisionEvent>(this, &MovementSystem::OnEntityCollide);
	}

	void OnEntityCollide(CollisionEvent& event) noexcept
	{
		Entity& a = event.m_entityA;
		Entity& b = event.m_entityB;
		// Logger::Log("Entity destroyed event received. Entity ID: " + std::to_string(event.m_entity.GetID()));
	
		if (a.BelongsToGroup("enemies") && b.BelongsToGroup("obstacles"))
		{
			OnEnemyHitsObstacle(a, b); // "a" is the enemy, "b" is the obstacle
		}
		else if (b.BelongsToGroup("enemies") && a.BelongsToGroup("obstacles"))
		{
			OnEnemyHitsObstacle(b, a); // "b" is the enemy, "a" is the obstacle
		}
	}

	void OnEnemyHitsObstacle(Entity& enemy, Entity& obstacle) noexcept
	{
		// flip the enemy velocity
		if (enemy.HasComponent<RigidbodyComponent>() && enemy.HasComponent<SpriteComponent>())
		{
			auto& enemyRigidbody = enemy.GetComponent<RigidbodyComponent>();
			auto& enemySprite = enemy.GetComponent<SpriteComponent>();

			if (enemyRigidbody.m_velocity.x != 0)
			{
				enemyRigidbody.m_velocity.x *= -1;
				enemySprite.m_flip = (enemySprite.m_flip == SDL_FLIP_NONE) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
			}
			if (enemyRigidbody.m_velocity.y != 0) {

				enemyRigidbody.m_velocity.y *= -1;
				enemySprite.m_flip = (enemySprite.m_flip == SDL_FLIP_NONE) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
			}
		}
	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		//TODO:
		//loop all entities with the system is interested in
		for (auto& entity : GetSystemEntities())
		{
			// Update entity position based on its velocity
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& sprite = entity.GetComponent<SpriteComponent>();
			const auto rigidbody = entity.GetComponent<RigidbodyComponent>();

			transform.m_position.x += rigidbody.m_velocity.x * deltaTime;
			transform.m_position.y += rigidbody.m_velocity.y * deltaTime;
			
			// constraint the position of the entity to the map limits
			if (entity.HasTag("player"))
			{
				constexpr int paddingLeft = 10;
				constexpr int paddingTop = 10;
				constexpr int paddingRight = 50;
				constexpr int paddingBottom = 50;
				transform.m_position.x = transform.m_position.x < paddingLeft ? paddingLeft : transform.m_position.x;
				transform.m_position.x = transform.m_position.x > Game::mapWidth - paddingRight ? Game::mapWidth - paddingRight : transform.m_position.x;
				transform.m_position.y = transform.m_position.y < paddingTop ? paddingTop : transform.m_position.y;
				transform.m_position.y = transform.m_position.y > Game::mapHeight - paddingBottom ? Game::mapHeight - paddingBottom : transform.m_position.y;
			}

			// check fi entity is outside the map limits with 100 pixels of margin
			constexpr int margin = 100;

			bool isEntityOustideMap =
				(
					transform.m_position.x < -margin ||
					transform.m_position.x > Game::mapWidth + margin ||
					transform.m_position.y < -margin ||
					transform.m_position.y > Game::mapHeight + margin
				);

			// kill entities that are outside the map limits
			if (!entity.HasTag("player") && isEntityOustideMap)
			{
				entity.Destroy();
			}

		}
	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	};

};

class RenderSystem : public System
{
public:

	RenderSystem() noexcept
	{
		RequireComponent<TransformComponent>();
		RequireComponent<SpriteComponent>();
	};

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{
		// TODO: sort all the entities based on their zIndex
		auto RenderableEntities = GetSystemEntities();
		auto RenderaableEntitesCopy = RenderableEntities;

		// bypass entities that are outside the camera view except for the fixed sprites?
		// great for performance
		/*for (auto& entity : RenderableEntities)
		{
			TransformComponent transform = entity.GetComponent<TransformComponent>();
			SpriteComponent sprite = entity.GetComponent<SpriteComponent>();

			bool isEntityOutsSideCameraView =
			{
				!sprite.m_isFixed &&
				transform.m_position.x > camera.x ||
				transform.m_position.x <= camera.x + camera.w ||
				transform.m_position.y > camera.y ||
				transform.m_position.y <= camera.y + camera.h
			};

			if (isEntityOutsSideCameraView)
			{
				continue;
			}

			RenderaableEntitesCopy.emplace_back(entity);
		}*/

		std::sort(RenderableEntities.begin(), RenderableEntities.end(), [](const Entity& entity1, const Entity& entity2)
			{
				return entity1.GetComponent<SpriteComponent>().m_zIndex < entity2.GetComponent<SpriteComponent>().m_zIndex;
			});

		for (const auto& entity : RenderableEntities)
		{
			const auto& tranform = entity.GetComponent<TransformComponent>();
			const auto sprite = entity.GetComponent<SpriteComponent>();

			// Set the source rectangle of our original texture
			SDL_Rect srcRect = sprite.m_srcRect;

			// Set the destination rectangle with the x, y position to be rendered
			SDL_Rect dstRect =
			{
				static_cast<int>(tranform.m_position.x - (sprite.m_isFixed ? 0 : camera.x)),
				static_cast<int>(tranform.m_position.y - (sprite.m_isFixed ? 0 : camera.y)),
				static_cast<int>(sprite.m_width * tranform.m_scale.x),
				static_cast<int>(sprite.m_height * tranform.m_scale.y)
			};

			// optionally rotate the texture and flip the texture as well
			SDL_RenderCopyEx
			(
				renderer,
				assetStore->GetTexture(sprite.assetId),
				&srcRect,
				&dstRect,
				tranform.m_rotation,
				NULL,
				sprite.m_flip
			);
		}
	}
};

class AnimationSystem : public System
{
public:

	AnimationSystem() noexcept
	{
		RequireComponent<SpriteComponent>();
		RequireComponent<AnimationComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		for (const auto& entity : GetSystemEntities())
		{
			auto& animation = entity.GetComponent<AnimationComponent>();
			auto& sprite = entity.GetComponent<SpriteComponent>();

			// TODO:
			// change the current frame
			// change the src rectangle of the sprite
			if (animation.shouldLoop)
			{
				animation.currentFrame = ((SDL_GetTicks() - animation.startTime) * animation.frameRateSpeed / 1000) % animation.numFrames;
				sprite.m_srcRect.x = animation.currentFrame * sprite.m_width;
			}
			else
			{
				if (animation.currentFrame < animation.numFrames)
				{
					animation.currentFrame = ((SDL_GetTicks() - animation.startTime) * animation.frameRateSpeed / 1000) % animation.numFrames;
					sprite.m_srcRect.x = animation.currentFrame * sprite.m_width;
					return;
				}
			}
			//animation.startTime += deltaTime * animation.frameRateSpeed;
			//animation.currentFrame = static_cast<int>(animation.startTime) % animation.numFrames;
			//sprite.m_srcRect.x = animation.currentFrame * sprite.m_width;
		}
	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	};
};

class CollisionSystem : public System
{
public:

	CollisionSystem() noexcept
	{
		RequireComponent<TransformComponent>();
		RequireComponent<BoxColliderComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		auto& entities = GetSystemEntities();
		// check all entities that have a boxcollider component
		// to see if they are colliding with each other
		// Use AABB collision detection
		
		// loop all the entities that the system is interested in
		for (auto i = entities.begin(); i != entities.end(); i++)
		{
			Entity a = *i;
			auto& aTransform = a.GetComponent<TransformComponent>();
			auto& aBoxCollider = a.GetComponent<BoxColliderComponent>();

			// loop all the entities that still need to be checked (to the right of it)
			for (auto j = i + 1; j != entities.end(); j++)
			{
				Entity b = *j;

				// bypass the entity if it is the same as the outer loop entity
				// if (a == b) continue;

				auto& bTransform = b.GetComponent<TransformComponent>();
				auto& bBoxCollider = b.GetComponent<BoxColliderComponent>();

				// check for collision between a and b
				bool isCollided = CheckAABBCollision
				(
					aTransform.m_position.x + aBoxCollider.m_offset.x,
					aTransform.m_position.y + aBoxCollider.m_offset.y,
					aBoxCollider.m_width,
					aBoxCollider.m_height,
					bTransform.m_position.x + bBoxCollider.m_offset.x,
					bTransform.m_position.y + bBoxCollider.m_offset.y,
					bBoxCollider.m_width,
					bBoxCollider.m_height
				);
				
				if (isCollided)
				{
					// TODO: emit an event that a collision has occured
					eventBus->PublishEvent<CollisionEvent>(a, b);		
				}
			}
		}
	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	};

	bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW, double bH) const noexcept
	{
		return
		{
			aX < bX + bW &&
			aX + aW > bX &&
			aY < bY + bH &&
			aY + aH > bY
		};
	}

};

class RenderColliderSystem : public System
{
public:

	RenderColliderSystem() noexcept
	{
		RequireComponent<TransformComponent>();
		RequireComponent<BoxColliderComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{
		if (!isDebugMode) return;

		for (const auto& entity : GetSystemEntities())
		{
			const auto& transform = entity.GetComponent<TransformComponent>();
			auto& boxCollider = entity.GetComponent<BoxColliderComponent>();

			boxCollider.m_width = boxCollider.m_width * transform.m_scale.x;
			boxCollider.m_height = boxCollider.m_height * transform.m_scale.y;

			SDL_Rect colliderRect =
			{
				static_cast<int>(transform.m_position.x + boxCollider.m_offset.x - camera.x),
				static_cast<int>(transform.m_position.y + boxCollider.m_offset.y - camera.y),
				static_cast<int>(boxCollider.m_width * transform.m_scale.x),
				static_cast<int>(boxCollider.m_height * transform.m_scale.y)
			};

			// draw the collider rectangle (unfilled red color)
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			SDL_RenderDrawRect(renderer, &colliderRect);
		}
	}

};

class DamageSystem : public System
{
public:

	DamageSystem() noexcept
	{
		RequireComponent<BoxColliderComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{
		eventBus->SubscribeEvent<CollisionEvent>(this, &DamageSystem::OnCollision);
	}

	/// <summary>
	/// Gets execute when a collision event is published
	/// </summary>
	/// <param name="event"></param>
	void OnCollision(CollisionEvent& event) noexcept
	{
		Entity& a = event.m_entityA;
		Entity& b = event.m_entityB;

		if (a.BelongsToGroup("projectiles") && b.HasTag("player"))
		{
			OnProjectileHitsPlayer(a, b); // "a" is the projectile, "b" is the player
		}
		else if (b.BelongsToGroup("projectiles") && a.HasTag("player"))
		{
			OnProjectileHitsPlayer(b, a); // "b" is the projectile, "a" is the player
		}
		else if (a.BelongsToGroup("projectiles") && b.BelongsToGroup("enemies"))
		{
			OnProjectileHitsEnemy(a, b); // "a" is the projectile, "b" is the enemy
		}
		else if (b.BelongsToGroup("projectiles") && a.BelongsToGroup("enemies"))
		{
			OnProjectileHitsEnemy(b, a); // "b" is the projectile, "a" is the enemy
		}
	}

	void OnProjectileHitsPlayer(Entity& projectile, Entity& player) noexcept
	{
		const auto& projectileComponent = projectile.GetComponent<ProjectileComponent>();

		// destroy the projectile if it is NOT friendly
		if (!projectileComponent.m_isFriendly)
		{
			// Reduce the health of the player by the damage of the projectile
			auto& playerHealth = player.GetComponent<HealthComponent>();

			playerHealth.m_currentHealth -= projectileComponent.m_hitPercentDamage;
			Logger::Log("Player current health: " + std::to_string(playerHealth.m_currentHealth));
			if (playerHealth.m_currentHealth <= 0)
			{
				player.Destroy();
			}

			// Destroy the projectile
			projectile.Destroy();
		}
	}

	void OnProjectileHitsEnemy(Entity& projectile, Entity& enemy) noexcept
	{
		const auto& projectileComponent = projectile.GetComponent<ProjectileComponent>();

		// destroy the projectile if it is FRIENDLY
		if (projectileComponent.m_isFriendly)
		{
			// Reduce the health of the enemy by the damage of the projectile
			auto& enemyHealth = enemy.GetComponent<HealthComponent>();

			enemyHealth.m_currentHealth -= projectileComponent.m_hitPercentDamage;
			Logger::Log("Enemy entity ID: " + std::to_string(enemy.GetID()) +  " current health: " + std::to_string(enemyHealth.m_currentHealth));
			if (enemyHealth.m_currentHealth <= 0)
			{
				enemy.Destroy();
			}

			// Destroy the projectile
			projectile.Destroy();
		}
	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, 
				std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	};
};

class KeyboardControlSystem : public System
{
public:

	KeyboardControlSystem() noexcept
	{
		RequireComponent<KeyboardControlledComponent>();
		RequireComponent<SpriteComponent>();
		RequireComponent<RigidbodyComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{
		eventBus->SubscribeEvent<KeyPressedEvent>(this, &KeyboardControlSystem::OnKeyPressed);
		Logger::Log("Key pressed event received. Key code: ");
	}

	void OnKeyPressed(KeyPressedEvent& e) noexcept
	{
		/*std::string keyCode = std::to_string(e.m_keyCode);
		std::string keySymbol = SDL_GetKeyName(e.m_keyCode);
		Logger::Log("Key pressed event received. Letter [" + keyCode + "]: " + keySymbol + " was pressed.");*/

		// change the sprite and the velocity of the entity
		for (const auto& entity : GetSystemEntities())
		{
			const auto& keyboardControlled = entity.GetComponent<KeyboardControlledComponent>();
			auto& sprite = entity.GetComponent<SpriteComponent>();
			auto& rigidbody = entity.GetComponent<RigidbodyComponent>();
			
			switch (e.m_keyCode)
			{
				case SDLK_w:
				case SDLK_UP:
					rigidbody.m_velocity = keyboardControlled.m_upVelocity;
					sprite.m_srcRect.y = sprite.m_height * 0;
					break;
				case SDLK_d:
				case SDLK_RIGHT:
					rigidbody.m_velocity = keyboardControlled.m_rightVelocity;
					sprite.m_srcRect.y = sprite.m_height * 1;
					break;
				case SDLK_s:
				case SDLK_DOWN:
					rigidbody.m_velocity = keyboardControlled.m_downVelocity;
					sprite.m_srcRect.y = sprite.m_height * 2;
					break;
				case SDLK_a:
				case SDLK_LEFT:
					rigidbody.m_velocity = keyboardControlled.m_leftVelocity;
					sprite.m_srcRect.y = sprite.m_height * 3;
					break;
				default: 
					break;
			}
		}

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	};

};

class CameraMovementSystem : public System
{
public:

	CameraMovementSystem() noexcept
	{
		RequireComponent<CameraFollowComponent>();
		RequireComponent<TransformComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		for (const auto& entity : GetSystemEntities())
		{
			// auto& cameraFollow = entity.GetComponent<CameraFollowComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			
			// center the camera on the entity
			if (transform.m_position.x + (camera.w / 2) < Game::mapWidth)
				camera.x = static_cast<int>(transform.m_position.x - (Game::windowWidth / 2));
			if (transform.m_position.y + (camera.h / 2) < Game::mapHeight)
				camera.y = static_cast<int>(transform.m_position.y - (Game::windowHeight / 2));
		
			// keep camera rectangle view inside the screen limtis
			camera.x = camera.x < 0 ? 0 : camera.x;
			camera.x = camera.x > camera.w ? camera.w : camera.x;
			camera.y = camera.y < 0 ? 0 : camera.y;
			camera.y = camera.y > camera.h ? camera.h : camera.y;
		}

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	}
};

class ProjectileEmitSystem : public System
{
public:

	ProjectileEmitSystem() noexcept
	{
		RequireComponent<ProjectileEmitterComponent>();
		RequireComponent<TransformComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{
		eventBus->SubscribeEvent<KeyPressedEvent>(this, &ProjectileEmitSystem::OnShootProjectile);
	}

	void OnShootProjectile(KeyPressedEvent& event) noexcept
	{
		if (event.m_keyCode == SDLK_z)
		{
			for (const auto& entity : GetSystemEntities())
			{
				// to identify if the entity is a player, check if it has a camera follow component
				if (entity.HasComponent<CameraFollowComponent>())
				{
					auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
					const auto& transform = entity.GetComponent<TransformComponent>();

					if (projectileEmitter.m_isManual && SDL_GetTicks() - projectileEmitter.m_lastEmissionTime > projectileEmitter.m_repeatFrequency)
					{
						Logger::Log("Shoot projectile event received.");

						CreateProjectileHelper(entity, projectileEmitter, transform, true);
					}
				}
			}
		}
	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		for (const auto& entity : GetSystemEntities())
		{
			auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
			const auto& transform = entity.GetComponent<TransformComponent>();

			// if emission frequency is zero bypass re-emission logic
			if (projectileEmitter.m_isManual) continue;

			// TODO: check if its time to re-emit a new projectile
			if (SDL_GetTicks() - projectileEmitter.m_lastEmissionTime > projectileEmitter.m_repeatFrequency)
			{
				CreateProjectileHelper(entity, projectileEmitter, transform, false);
			}
		}
	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	}

	void CreateProjectileHelper(const Entity& entity, ProjectileEmitterComponent& projectileEmitter, const TransformComponent& transform, bool isManual) noexcept
	{
		// If parent entitiy has sprite, start the projectile position in the middle
		glm::vec2 projectilePosition = transform.m_position;
		if (entity.HasComponent<SpriteComponent>())
		{
			auto& sprite = entity.GetComponent<SpriteComponent>();
			projectilePosition.x += (transform.m_scale.x * sprite.m_width / 2);
			projectilePosition.y += (transform.m_scale.y * sprite.m_height / 2);
		}

		// if parent entity direction is controlled by the keyboard, modify the projectile velocity
		glm::vec2 projectileVelocity = projectileEmitter.m_projectileVelocity;
		if (isManual && entity.HasComponent<RigidbodyComponent>())
		{
			const auto& rigidbody = entity.GetComponent<RigidbodyComponent>();
			int directionX = 0;
			int directionY = 0;
			if (rigidbody.m_velocity.x > 0) directionX = 1;
			if (rigidbody.m_velocity.x < 0) directionX = -1;
			if (rigidbody.m_velocity.y > 0) directionY = 1;
			if (rigidbody.m_velocity.y < 0) directionY = -1;
			projectileVelocity.x = projectileEmitter.m_projectileVelocity.x * directionX;
			projectileVelocity.y = projectileEmitter.m_projectileVelocity.y * directionY;
		}

		// Create a new projectile entity and add it to the world
		Entity projectile = entity.m_registry->CreateEntity();
		projectile.Group("projectiles");
		projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(1.0, 1.0), 0.0f);
		projectile.AddComponent<RigidbodyComponent>(projectileVelocity);
		projectile.AddComponent<SpriteComponent>("bullet-image", 4, 4, 4);
		projectile.AddComponent<BoxColliderComponent>(4, 4);
		projectile.AddComponent<ProjectileComponent>(projectileEmitter.m_isFriendly,
													 projectileEmitter.m_hitPercentDamage,
													 projectileEmitter.m_projectileDuraiton);

		// update the projectile emitter component last emission to the current milisecond time
		projectileEmitter.m_lastEmissionTime = SDL_GetTicks();
	}

};

class ProjectileLifeCycleSystem : public System
{
public:

	ProjectileLifeCycleSystem() noexcept
	{
		RequireComponent<ProjectileComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{
		
	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{
		for (auto& entity : GetSystemEntities())
		{
			auto& projectile = entity.GetComponent<ProjectileComponent>();

			// TODO: Kill projectile after they reach their duration limit
			if (SDL_GetTicks() - projectile.m_startTime >= projectile.m_duration)
			{
				entity.Destroy();
			}
		}
	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{

	}

};

class RenderTextSystem : public System
{
public:

	RenderTextSystem() noexcept
	{
		RequireComponent<TextLabelComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera, std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{
		for (auto& entity : GetSystemEntities())
		{
			const auto& textLabel = entity.GetComponent<TextLabelComponent>();

			SDL_Surface* surface = TTF_RenderText_Blended(assetStore->GetFont(textLabel.assetId),
														  textLabel.m_text.c_str(),
														  textLabel.m_color);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);

			int labelWidth = 0;
			int labelHeight = 0;

			SDL_QueryTexture(texture, NULL, NULL, &labelWidth, &labelHeight);

			SDL_Rect dstRect =
			{
				static_cast<int>(textLabel.m_position.x - (textLabel.m_isFixed ? 0 : camera.x)),
				static_cast<int>(textLabel.m_position.y - (textLabel.m_isFixed ? 0 : camera.y)),
				labelWidth,
				labelHeight
			};

			SDL_RenderCopy(renderer, texture, NULL, &dstRect);

			SDL_DestroyTexture(texture);
		}
	}

};

class RenderHealthBarSystem : public System
{
public:

	RenderHealthBarSystem() noexcept
	{
		RequireComponent<TransformComponent>();
		RequireComponent<SpriteComponent>();
		RequireComponent<HealthComponent>();
	}

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera,
				std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{
		for (const auto& entity : GetSystemEntities())
		{
			const auto& transform = entity.GetComponent<TransformComponent>();
			const auto& sprite = entity.GetComponent<SpriteComponent>();
			const auto& health = entity.GetComponent<HealthComponent>();

			// Draw the health bar with the correct color for the percentage
			SDL_Color healthBarColor = { 0, 255, 0 };

			if (health.m_currentHealth >= 0 && health.m_currentHealth <= static_cast<int>(health.m_maxHealth / 3))
			{
				// red color
				healthBarColor = { 255, 0, 0 };
			}
			else if (health.m_currentHealth > static_cast<int>(health.m_maxHealth / 3) && health.m_currentHealth <= static_cast<int>(health.m_maxHealth / 2))
			{
				// yellow color
				healthBarColor = { 255, 255, 0 };
			}
			else if (health.m_currentHealth > static_cast<int>(health.m_maxHealth / 2) && health.m_currentHealth <= static_cast<int>(health.m_maxHealth))
			{
				// green color
				healthBarColor = { 0, 255, 0 };
			}

			// position the health bar in the middle-bottom part of the entity sprite
			int healthBarWidth = 15;
			int healthBarHeight = 3;
			double healthBarPosX = (transform.m_position.x + (sprite.m_width * transform.m_scale.x)) - camera.x;
			double healthBarPosY = (transform.m_position.y) - camera.y;

			SDL_Rect healthBarRect = 
			{
				static_cast<int>(healthBarPosX),
				static_cast<int>(healthBarPosY),
				static_cast<int>(healthBarWidth * (health.m_currentHealth) / health.m_maxHealth),
				static_cast<int>(healthBarHeight)
			};
			SDL_SetRenderDrawColor(renderer, healthBarColor.r, healthBarColor.g, healthBarColor.b, 255);
			SDL_RenderFillRect(renderer, &healthBarRect);

			// Render the health percentage text label indicator
			std::string healthText = std::to_string(health.m_currentHealth);
			SDL_Surface* surface = TTF_RenderText_Blended(assetStore->GetFont("pico8-font-8"), healthText.c_str(), healthBarColor);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);

			int labelWidth = 0;
			int labelHeight = 0;
			SDL_QueryTexture(texture, NULL, NULL, &labelWidth, &labelHeight);
			SDL_Rect healthBarTextRect =
			{
				static_cast<int>(healthBarPosX),
				static_cast<int>(healthBarPosY) + 5,
				labelWidth,
				labelHeight
			};

			SDL_RenderCopy(renderer, texture, NULL, &healthBarTextRect);

			SDL_DestroyTexture(texture);
		}
	}

};

class RenderGUISystem : public System
{
public:

	RenderGUISystem() noexcept = default;

	void SubscribeToEvent(std::unique_ptr<EventBus>& eventBus) noexcept override
	{

	}

	void Update(float deltaTime, std::unique_ptr<EventBus>& eventBus, SDL_Rect& camera,
		std::unique_ptr<Registry>& registry, std::unique_ptr<AssetStore>& assetStore, SDL_Renderer* renderer) noexcept override
	{

	}

	void Render(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, SDL_Rect& camera, std::unique_ptr<Registry>& registry, bool isDebugMode) noexcept override
	{
		if (!isDebugMode) return;

		ImGui::NewFrame();
		
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;
		if (ImGui::Begin("Spawn Enemies")/*, NULL, windowFlags*/)
		{
			// static variables to keep the input values between frames
			static int enemyXPos = 0, enemyYPos = 0;
			static int enemyScaleX = 1, enemyScaleY = 1;
			static int enemyXVel = 0, enemyYVel = 0;
			static int enemyHealth = 100;
			static float enemyRotation = 0.0f;
			static float enemyProjAngle = 0.0f;
			static float enemyProjSpeed = 100.0f;
			static int enemyProjRepeat = 1;
			static int enemyProjDuration = 1;
			const char* sprites[] = { "tank-image","truck-image" };
			static int selectedSpriteIndex = 0;

			// Section to input enemy sprite
			if (ImGui::CollapsingHeader("Sprite"), ImGuiTreeNodeFlags_DefaultOpen)
			{
				ImGui::Combo("texture id", &selectedSpriteIndex, sprites, IM_ARRAYSIZE(sprites));
			}
			ImGui::Spacing();
			
			// selection to input enemy transform
			if (ImGui::CollapsingHeader("Transform"), ImGuiTreeNodeFlags_DefaultOpen)
			{
				ImGui::InputInt("x position", &enemyXPos);
				ImGui::InputInt("y position", &enemyYPos);
				ImGui::SliderInt("scale X", &enemyScaleX, 1, 10);
				ImGui::SliderInt("scale Y", &enemyScaleY, 1, 10);
				ImGui::SliderAngle("rotation (deg)", &enemyRotation, 0, 360);
			}
			ImGui::Spacing();

			// selection to input enemy rigid body values
			if (ImGui::CollapsingHeader("Rigidbody"), ImGuiTreeNodeFlags_DefaultOpen)
			{
				ImGui::InputInt("x velocity", &enemyXVel);
				ImGui::InputInt("y velocity", &enemyYVel);
			}
			ImGui::Spacing();
			
			// selection to input enemy projectile emitter values
			if (ImGui::CollapsingHeader("Projectile Emitter"), ImGuiTreeNodeFlags_DefaultOpen)
			{
				ImGui::SliderAngle("angle (deg)", &enemyProjAngle, 0, 360);
				ImGui::SliderFloat("speed (px/sec)", &enemyProjSpeed, 10, 500);
				ImGui::InputInt("repeat (sec)", &enemyProjRepeat);
				ImGui::InputInt("duration (sec)", &enemyProjDuration);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// selection to input enemy health
			if (ImGui::CollapsingHeader("Health"), ImGuiTreeNodeFlags_DefaultOpen)
			{
				ImGui::SliderInt("health %", &enemyHealth, 10, 100);
			}
			ImGui::Spacing();
			
			if (ImGui::Button("Spawn New Enemy"))
			{
				Entity enemy = registry->CreateEntity();
				enemy.Group("enemies");
				enemy.AddComponent<TransformComponent>(glm::vec2(enemyXPos, enemyYPos), glm::vec2(enemyScaleX, enemyScaleY), glm::degrees(enemyRotation));
				enemy.AddComponent<RigidbodyComponent>(glm::vec2(enemyXVel, enemyYVel));
				enemy.AddComponent<SpriteComponent>(sprites[selectedSpriteIndex], IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 2);
				enemy.AddComponent<BoxColliderComponent>(25, 20, glm::vec2(5, 5));
				
				double projVelX = cos(enemyProjAngle) * enemyProjSpeed; // convert angle to radians
				double projVelY = sin(enemyProjAngle) * enemyProjSpeed; // convert angle to radians
				
				enemy.AddComponent<ProjectileEmitterComponent>(glm::vec2(projVelX, projVelY), enemyProjRepeat * 1000, enemyProjDuration * 1000, 10, false, false);
				enemy.AddComponent<HealthComponent>(enemyHealth, enemyHealth);
			
				// reset all input values after we create a new enemy
				enemyXPos = enemyYPos = enemyRotation = enemyProjAngle = 0;
				enemyScaleX = enemyScaleY = 1;
				enemyProjRepeat = enemyProjDuration = 1;
				enemyProjSpeed = 100.0f;
				enemyXVel = enemyYVel = 0;
				enemyHealth = 100;
			}
		}
		ImGui::End();

		// display a small overlay window to display the map position using the mouse
		ImGuiWindowFlags windowFlags2 = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always, ImVec2(0, 0));
		if (ImGui::Begin("Mouse Position", NULL, windowFlags2))
		{
			ImGui::Text("Mouse Position: (x = %.1f, y = %.1f)", ImGui::GetIO().MousePos.x + camera.x, ImGui::GetIO().MousePos.y + camera.y);
		}
		ImGui::End();

		ImGui::Render();
		ImGuiSDL::Render(ImGui::GetDrawData());
	}
};

#endif // SYSTEMS_H
