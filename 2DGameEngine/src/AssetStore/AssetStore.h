#pragma once
#ifndef ASSETSTORE_H
#define ASSETSTORE_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <map>
#include <string>
#include <fstream>

class AssetStore
{
public:

	AssetStore() noexcept;
	~AssetStore() noexcept;

	void ClearAssets() noexcept;

	void AddTexture(SDL_Renderer* renderer, const std::string& assetId, const std::string& filePath) noexcept;
	SDL_Texture* GetTexture(const std::string& assetId) const noexcept;

	void AddFont(const std::string& assetId, const std::string& filePath, unsigned int fontSize) noexcept;
	TTF_Font* GetFont(const std::string& assetId) const noexcept;

private:
	
	std::map <std::string, SDL_Texture*> textures;
	std::map <std::string, TTF_Font*> fonts;
	// TODO: create a map for audio
	std::map <std::string, Mix_Music*> audio;

};

#endif // ASSETSTORE_H
