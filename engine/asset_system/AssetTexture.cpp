#include "AssetTexture.hpp"

#include "BedrockAssert.hpp"

namespace MFA::Asset
{

	//-------------------------------------------------------------------------------------------------

	Texture::Texture(
	    Format const format,
        uint16_t const slices,
        uint16_t const depth,
        uint8_t const mipCount
	)
	{
		MFA_ASSERT(format != Format::INVALID);
		mFormat = format;
		MFA_ASSERT(slices > 0);
		mSlices = slices;
		MFA_ASSERT(depth > 0);
		mDepth = depth;
	    MFA_ASSERT(mipCount > 0);
		mMipCount = mipCount;
	    mMipmaps.resize(mMipCount);
	}

	//-------------------------------------------------------------------------------------------------

	Texture::~Texture()
	{
	    mMipmaps.clear();
	}

	//-------------------------------------------------------------------------------------------------

	size_t Texture::MipSizeBytes(Format format, uint16_t slices, Dimensions const& mipLevelDimension)
	{
		auto const& d = mipLevelDimension;
		size_t const p = FormatTable[static_cast<unsigned>(format)].bits_total / 8;
		return p * slices * d.width * d.height * d.depth;
	}

	//-------------------------------------------------------------------------------------------------

	Texture::Dimensions Texture::MipDimensions(
		uint8_t const mipLevel, 
		uint8_t const mipCount,
		Dimensions const originalImageDims
	)
	{
		Dimensions ret = {};
		if (mipLevel < mipCount)
		{
			uint32_t const pow = mipLevel;
			uint32_t const add = (1 << pow) - 1;
			ret.width = (originalImageDims.width + add) >> pow;
			ret.height = (originalImageDims.height + add) >> pow;
			ret.depth = static_cast<uint16_t>((originalImageDims.depth + add) >> pow);
		}
		return ret;
	}

	//-------------------------------------------------------------------------------------------------

    void Texture::SetMipmapInfo(
        uint8_t const mipIdx,
        Dimensions const& dimension
    )
	{
	    MFA_ASSERT(mipIdx < mMipCount);

	    mMipmaps[mipIdx].dimension = dimension;
	}

	//-------------------------------------------------------------------------------------------------

	void Texture::SetMipmapData(uint8_t const mipIdx, std::shared_ptr<Blob> const &data)
    {
        MFA_ASSERT(mipIdx < mMipCount);

        mMipmaps[mipIdx].data = data;
        ++mMipCount;
    }

    //-------------------------------------------------------------------------------------------------

    void Texture::ClearMipmapBuffer(int const index) { mMipmaps[index].data = nullptr; }
    void Texture::ClearMipmapBuffer()
	{
	    for (auto & mipmap : mMipmaps)
	    {
	        mipmap.data = nullptr;
	    }
	}

	//-------------------------------------------------------------------------------------------------

	Texture::Format Texture::GetFormat() const noexcept
	{
		return mFormat;
	}

	//-------------------------------------------------------------------------------------------------

	uint16_t Texture::GetSlices() const noexcept
	{
		return mSlices;
	}

	//-------------------------------------------------------------------------------------------------

	uint8_t Texture::GetMipCount() const noexcept
	{
		return mMipCount;
	}

	//-------------------------------------------------------------------------------------------------

	Texture::Dimensions const& Texture::GetMipmapDimension(uint8_t const mipLevel) const noexcept
	{
	    MFA_ASSERT(mipLevel < mMipCount);
		return mMipmaps[mipLevel].dimension;
	}

	//-------------------------------------------------------------------------------------------------

	std::shared_ptr<Blob> const & Texture::GetMipmapBuffer(uint8_t const mipLevel) const noexcept
	{
	    MFA_ASSERT(mipLevel < mMipCount);
		return mMipmaps[mipLevel].data;
	}

	//-------------------------------------------------------------------------------------------------

	uint16_t Texture::GetDepth() const noexcept
	{
		return mDepth;
	}

	//-------------------------------------------------------------------------------------------------

}
