#pragma once

#include <filesystem>
#include <memory>
#include <string>

//https://stackoverflow.com/questions/4815423/how-do-i-set-the-working-directory-to-the-solution-directory
namespace MFA
{
	class Path
	{
	public:

		explicit Path();

		~Path();

		// Returns correct address based on platform
		[[nodiscard]]
		static std::string Get(std::string const& address);

	    [[nodiscard]]
        static std::string Get(char const * address);

	    static std::string Get(char const *address, char const *relativePath);

	    static std::string Relative(char const * address);

        [[nodiscard]]
	    static std::string const & AssetPath();

	private:

	    [[nodiscard]]
        std::string Private_Get(char const * address) const;

	    std::string Private_Relative(char const * address) const;

	    [[nodiscard]]
        std::string const & Private_AssetPath() const {return mAssetPath;}

        inline static Path * mInstance;

		std::string mAssetPath {};

	};
};