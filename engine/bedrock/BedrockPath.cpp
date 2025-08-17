#include "BedrockPath.hpp"

#include "BedrockAssert.hpp"
#include "BedrockCommon.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

static bool LogCalledOnce = false;

//-------------------------------------------------------------------------------------------------

MFA::Path::Path()
{
	MFA_ASSERT(mInstance == nullptr);
	mInstance = this;
	
#if defined(ASSET_DIR)
	mAssetPath = std::filesystem::absolute(std::string(TO_LITERAL(ASSET_DIR))).string();
#endif

	static constexpr char const * OVERRIDE_ASSET_PATH = "./asset_dir.txt";
	if (std::filesystem::exists(OVERRIDE_ASSET_PATH))
	{
		std::ifstream nameFileout{};
		mAssetPath.clear();

		nameFileout.open(OVERRIDE_ASSET_PATH);
		while (nameFileout >> mAssetPath)
		{
			std::cout << mAssetPath;
		}
		nameFileout.close();
	    if (LogCalledOnce == false)
	    {
	        MFA_LOG_INFO("Override asset path is %s", mAssetPath.c_str());
	    }
	}
	else
	{
	    if (LogCalledOnce == false)
	    {
	        MFA_LOG_INFO("No override found, using the default directory: %s", mAssetPath.c_str());
	    }
	}

    LogCalledOnce = true;
}

//-------------------------------------------------------------------------------------------------

MFA::Path::~Path()
{
    MFA_ASSERT(mInstance == this);
	if (mInstance == this)
	{
		mInstance = nullptr;
	}
}

//-------------------------------------------------------------------------------------------------

std::string MFA::Path::Get(std::string const& address)
{
	if (mInstance != nullptr)
	{
		return mInstance->Private_Get(address.c_str());
	}
	return address;
}

std::string MFA::Path::Get(char const *address)
{
	if (mInstance != nullptr)
	{
		return mInstance->Private_Get(address);
	}
	return "";
}

std::string MFA::Path::Private_Get(char const *address) const
{
    /*if (std::filesystem::exists(address) == true)
    {
        return address;
    }*/
    if (std::strncmp(address, "./", 2) == 0 || std::strncmp(address, "/", 1) == 0)
    {
        return address;
    }
    return std::filesystem::path(mAssetPath).append(address).string();
}

//-------------------------------------------------------------------------------------------------

std::string MFA::Path::Get(char const *address, char const *relativePath)
{
    /*if (std::filesystem::exists(address) == true)
    {
        return address;
    }*/
    if (std::strncmp(address, "./", 2) == 0 || std::strncmp(address, "/", 1) == 0)
    {
        return address;
    }
    return std::filesystem::path(relativePath).append(address).string();
}

//-------------------------------------------------------------------------------------------------

std::string MFA::Path::Relative(char const *address)
{
    if (mInstance != nullptr)
    {
        return mInstance->Private_Relative(address);
    }
    return "";
}

std::string MFA::Path::Private_Relative(char const *address) const
{
    if (std::strncmp(address, mAssetPath.c_str(), mAssetPath.size()) == 0)
    {
        return std::string(address).substr(mAssetPath.size());
    }
    return address;
}

//-------------------------------------------------------------------------------------------------

std::string const & MFA::Path::AssetPath()
{
    static std::string empty;
    if (mInstance != nullptr)
    {
        return mInstance->Private_AssetPath();
    }
    return empty;
}

//-------------------------------------------------------------------------------------------------
