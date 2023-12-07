#include "AssetStore.h"

#include "../Logger/Logger.h"

AssetStore::AssetStore() noexcept
{
	Logger::Log("AssetStore constructor called");

}

AssetStore::~AssetStore() noexcept
{
	ClearAssets();
	Logger::Log("AssetStore destructor called");
}

/// <summary>
/// Clear the AssetStore
/// </summary>
void AssetStore::ClearAssets() noexcept
{
	for (auto& texture : textures)
	{
		SDL_DestroyTexture(texture.second);
	}
	textures.clear();

	for (auto& font : fonts)
	{
		TTF_CloseFont(font.second);
	}
	fonts.clear();
}

/// <summary>
/// Add a texture to the AssetStore
/// </summary>
/// <param name="renderer"></param>
/// <param name="assetId"></param>
/// <param name="filePath"></param>
void AssetStore::AddTexture(SDL_Renderer* renderer, const std::string& assetId, const std::string& filePath) noexcept
{
	SDL_Surface* surface = IMG_Load(filePath.c_str());
	if (!surface)
	{
		Logger::Error("Error loading image: " + filePath);
		return;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		Logger::Error("Error creating texture from surface: " + filePath);
		return;
	}

	SDL_FreeSurface(surface);

	// Add the texture to the AssetStore using the assetId as the key
	textures.emplace(assetId, texture);

	Logger::Log("New texture added to Asset Store with id = " + assetId);
}

/// <summary>
/// Get the texture from the AssetStore
/// </summary>
/// <param name="assetId"></param>
/// <returns></returns>
SDL_Texture* AssetStore::GetTexture(const std::string& assetId) const noexcept
{
	return textures.at(assetId);
}

/// <summary>
/// Add a font to the AssetStore
/// </summary>
/// <param name="assetId"></param>
/// <param name="filePath"></param>
/// <param name="fontSize"></param>
void AssetStore::AddFont(const std::string& assetId, const std::string& filePath, unsigned int fontSize) noexcept
{
	TTF_Font* font = TTF_OpenFont(filePath.c_str(), fontSize);
	if (!font)
	{
		Logger::Error("Error loading font: " + filePath);
		return;
	}

	// Add the font to the AssetStore using the assetId as the key
	fonts.emplace(assetId, font);

	Logger::Log("New font added to Asset Store with id = " + assetId);
}

/// <summary>
/// Get the font from the AssetStore
/// </summary>
/// <param name="assetId"></param>
/// <returns></returns>
TTF_Font* AssetStore::GetFont(const std::string& assetId) const noexcept
{
	return fonts.at(assetId);
}
