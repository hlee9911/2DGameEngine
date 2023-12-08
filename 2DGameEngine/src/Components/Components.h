#pragma once
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <string>
#include <SDL.h>
#include <SDL_image.h>
#include <glm/glm.hpp>

#include "../Logger/Logger.h"

struct TransformComponent
{
	glm::vec2 m_position;
	glm::vec2 m_scale;
	double m_rotation;

	TransformComponent(glm::vec2 position = glm::vec2(0, 0), glm::vec2 scale = glm::vec2(1, 1), double rotation = 0.0) noexcept
	{
		this->m_position = position;
		this->m_scale = scale;
		this->m_rotation = rotation;
	}

};

struct RigidbodyComponent
{
	glm::vec2 m_velocity;

	// Constructor
	RigidbodyComponent(glm::vec2 velocity = glm::vec2(0.0, 0.0)) noexcept
		: m_velocity(velocity) {}

};

struct SpriteComponent
{
	std::string assetId;
	int m_width;
	int m_height;
	bool m_isFixed;
	int m_zIndex;
	SDL_RendererFlip m_flip;
	SDL_Rect m_srcRect;

	SpriteComponent(const std::string& assetId = "", int width = 0, int height = 0, int zIndex = 0, bool isFixed = false, int srcRectX = 0, int srcRectY = 0) noexcept
	{
		this->assetId = assetId;
		this->m_width = width;
		this->m_height = height;
		this->m_zIndex = zIndex;
		this->m_flip = SDL_FLIP_NONE; 
		this->m_isFixed = isFixed;
		this->m_srcRect = { srcRectX, srcRectY, width, height };
	};
};

struct AnimationComponent
{
	int numFrames;
	int currentFrame;
	int frameRateSpeed;
	bool shouldLoop;
	int startTime;

	AnimationComponent(int numFrames = 1, int frameRateSpeed = 1, bool shouldLoop = true) noexcept
	{
		this->numFrames = numFrames;
		this->currentFrame = 1;
		this->frameRateSpeed = frameRateSpeed;
		this->shouldLoop = shouldLoop;
		this->startTime = SDL_GetTicks();
	}
};

struct BoxColliderComponent
{
	int m_width;
	int m_height;
	glm::vec2 m_offset;
	bool m_isTrigger;

	BoxColliderComponent(int width = 0, int height = 0, glm::vec2 offset = glm::vec2(0), bool isTrigger = false) noexcept
	{
		this->m_width = width;
		this->m_height = height;
		this->m_offset = offset;
		this->m_isTrigger = isTrigger;
	}
};

struct KeyboardControlledComponent
{
	glm::vec2 m_upVelocity;
	glm::vec2 m_rightVelocity;
	glm::vec2 m_downVelocity;
	glm::vec2 m_leftVelocity;

	constexpr KeyboardControlledComponent(glm::vec2 upVelocity = glm::vec2(0),
								 glm::vec2 rightVelocity = glm::vec2(0),
								 glm::vec2 downVelocity = glm::vec2(0),
								 glm::vec2 leftVelocity = glm::vec2(0)) noexcept :
		m_upVelocity(upVelocity),
		m_rightVelocity(rightVelocity),
		m_downVelocity(downVelocity),
		m_leftVelocity(leftVelocity)
	{

	}
};

struct CameraFollowComponent
{
	constexpr CameraFollowComponent() noexcept = default;
};

struct ProjectileEmitterComponent
{
	glm::vec2 m_projectileVelocity;
	int m_repeatFrequency;
	int m_projectileDuraiton;
	int m_hitPercentDamage;
	bool m_isFriendly;
	int m_lastEmissionTime;
	bool m_isManual;

	ProjectileEmitterComponent(glm::vec2 projectileVelocity = glm::vec2(0),
										 int repeatFrequency = 0,
										 int projectileDuration = 10000,
									     int hitPercentDamage = 10,
										 bool isFriendly = false,
										 bool isManual = false) noexcept :
		m_projectileVelocity(projectileVelocity),
		m_repeatFrequency(repeatFrequency),
		m_projectileDuraiton(projectileDuration),
		m_hitPercentDamage(hitPercentDamage),
		m_isFriendly(isFriendly),
		m_lastEmissionTime(SDL_GetTicks()),
		m_isManual(isManual) {}

};

struct HealthComponent
{
	int m_maxHealth;
	int m_currentHealth;

	constexpr HealthComponent(int maxHealth = 100, int currentHealth = 100) noexcept :
		m_maxHealth(maxHealth),
		m_currentHealth(currentHealth) {}

};

struct ProjectileComponent
{
	bool m_isFriendly;
	int m_hitPercentDamage;
	int m_duration;
	int m_startTime;
	
	ProjectileComponent(bool isFriendly = false,
						int hitPercentDamage = 0,
						int duration = 0) noexcept :
		m_isFriendly(isFriendly),
		m_hitPercentDamage(hitPercentDamage),
		m_duration(duration),
		m_startTime(SDL_GetTicks()) {}

};

struct TextLabelComponent
{
	glm::vec2 m_position;
	std::string m_text;
	std::string assetId;
	SDL_Color m_color;
	bool m_isFixed;

	TextLabelComponent(glm::vec2 position = glm::vec2(0), std::string text = "", std::string assetid = "", const SDL_Color& color = { 0, 0, 0 }, bool isFixed = true) noexcept
	{
		this->m_position = position;
		this->m_text = text;
		this->assetId = assetid;
		this->m_color = color;
		this->m_isFixed = isFixed;
		Logger::Log("TextLabelComponent created");
	}

};


#endif // COMPONENTS_H
