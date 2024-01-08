#include "Layer.h"

#include "Macros.h"
#include "Enum.h"
#include "PhotoshopFile/LayerAndMaskInformation.h"
#include "PhotoshopFile/AdditionalLayerInfo.h"
#include "Struct/TaggedBlock.h"

#include <vector>
#include <optional>
#include <string>
#include <memory>

PSAPI_NAMESPACE_BEGIN


// Instantiate the template types for Layer
template struct Layer<uint8_t>;
template struct Layer<uint16_t>;
template struct Layer<float32_t>;


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Layer<T>::Layer(const LayerRecord& layerRecord, ChannelImageData& channelImageData)
{
	m_LayerName = layerRecord.m_LayerName.getString();
	// To parse the blend mode we must actually check for the presence of the sectionDivider blendMode as this overrides the layerRecord
	// blendmode if it is present
	{
		auto& additionalLayerInfo = layerRecord.m_AdditionalLayerInfo.value();
		auto sectionDivider = additionalLayerInfo.getTaggedBlock<LrSectionTaggedBlock>(Enum::TaggedBlockKey::lrSectionDivider);
		if (sectionDivider.has_value() && sectionDivider.value()->m_BlendMode.has_value())
		{
			m_BlendMode = sectionDivider.value()->m_BlendMode.value();
		}
		else
		{
			m_BlendMode = layerRecord.m_BlendMode;
		}
	}
	// For now we only parse visibility from the bitflags but this could be expanded to parse other information as well.
	m_IsVisible = layerRecord.m_BitFlags.m_isVisible;
	m_Opacity = layerRecord.m_Opacity;
	m_Width = layerRecord.m_Right - layerRecord.m_Left;
	m_Height = layerRecord.m_Bottom - layerRecord.m_Top;
	m_CenterX = layerRecord.m_Left + m_Width / 2;
	m_CenterY = layerRecord.m_Top + m_Height / 2;

	// Move the layer mask into our layerMask struct, for now this only does pixel masks
	for (int i = 0; i < layerRecord.m_ChannelCount; ++i)
	{
		auto& channelInfo = layerRecord.m_ChannelInformation[i];
		if (channelInfo.m_ChannelID.id == Enum::ChannelID::UserSuppliedLayerMask)
		{
			// Move the compressed image data into our LayerMask struct
			LayerMask<T> lrMask;
			auto channelPtr = channelImageData.extractImagePtr(channelInfo.m_ChannelID);
			ImageChannel<T>* imageChannelPtr = dynamic_cast<ImageChannel<T>*>(channelPtr.get());
			channelPtr = nullptr;

			if (imageChannelPtr)
			{
				lrMask.maskData = std::move(*imageChannelPtr);
			}
			else
			{
				PSAPI_LOG_ERROR("Layer", "Unable to cast mask to ImageChannel")
			}

			// If no mask parameters are present we just use sensible defaults and skip
			if (!layerRecord.m_LayerMaskData.has_value()) continue;
			auto& maskParams = layerRecord.m_LayerMaskData.value();
			if (!maskParams.m_LayerMask.has_value()) continue;

			// Read the mask parameters
			auto& layerMaskParams = maskParams.m_LayerMask.value();

			lrMask.isDisabled = layerMaskParams.m_Disabled;
			lrMask.maskDensity = layerMaskParams.m_UserMaskDensity;
			lrMask.maskFeather = layerMaskParams.m_UserMaskFeather;

			// Set the layer mask by moving our temporary struct
			m_LayerMask = std::optional(std::move(lrMask));
		}
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
std::optional<LayerRecords::LayerMaskData> Layer<T>::generateMaskData()
{
	auto lrMaskData = LayerRecords::LayerMaskData();

	// We dont have support for vector masks so far so we do not consider them
	lrMaskData.m_VectorMask = std::nullopt;
	if (!m_LayerMask.has_value())
	{
		lrMaskData.m_LayerMask = std::nullopt;
	}
	else
	{
		LayerRecords::LayerMask lrMask = LayerRecords::LayerMask{};

		int32_t top, left, bottom, right;
		int32_t centerX = m_LayerMask.value().maskData.getCenterX();
		int32_t centerY = m_LayerMask.value().maskData.getCenterY();
		int32_t width = m_LayerMask.value().maskData.getWidth();
		int32_t height = m_LayerMask.value().maskData.getHeight();
		top		= centerY - height / 2;
		left	= centerX - width / 2;
		bottom	= centerY + height / 2;
		right	= centerX + width / 2;
		lrMaskData.m_Size += 16u;

		uint8_t defaultColor = 0u;
		lrMaskData.m_Size += 1u;

		// This is the size for the mask bitflags
		lrMaskData.m_Size += 1u;
		// This is the size for the mask parameters
		lrMaskData.m_Size += 1u;
		bool hasMaskDensity = m_LayerMask.value().maskDensity.has_value();
		uint8_t maskDensity = 0u;
		if (hasMaskDensity)
		{
			maskDensity = m_LayerMask.value().maskDensity.value();
			lrMaskData.m_Size += 1u;
		}

		bool hasMaskFeather = m_LayerMask.value().maskFeather.has_value();
		float64_t maskFeather = 0.0f;
		if (hasMaskFeather)
		{
			maskFeather = m_LayerMask.value().maskFeather.value();
			lrMaskData.m_Size += 8u;

		}

		lrMask.m_Top = top;
		lrMask.m_Left = left;
		lrMask.m_Bottom = bottom;
		lrMask.m_Right = right;
		lrMask.m_Disabled = m_LayerMask.value().isDisabled;
		lrMask.m_HasMaskParams = hasMaskDensity || hasMaskFeather;
		lrMask.m_HasUserMaskDensity = hasMaskDensity;
		lrMask.m_HasUserMaskFeather = hasMaskFeather;
		if (hasMaskDensity)
		{
			lrMask.m_UserMaskDensity.emplace(maskDensity);
		}
		if (hasMaskFeather)
		{
			lrMask.m_UserMaskFeather.emplace(maskFeather);
		}

		lrMaskData.m_LayerMask.emplace(lrMask);
	}
	

	std::optional<LayerRecords::LayerMaskData> lrMaskDataOpt;
	lrMaskDataOpt.emplace(lrMaskData);
	if (!lrMaskData.m_LayerMask.has_value() && !lrMaskData.m_VectorMask.has_value())
	{
		return std::nullopt;
	}
	return lrMaskDataOpt;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
std::tuple<int32_t, int32_t, int32_t, int32_t> Layer<T>::generateExtents()
{
	int32_t top		= m_CenterY - m_Height / 2;
	int32_t left	= m_CenterX - m_Width / 2;
	int32_t bottom	= m_CenterY + m_Height / 2;
	int32_t right	= m_CenterX + m_Width / 2;

	return std::make_tuple(top, left, bottom, right);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
PascalString Layer<T>::generatePascalString()
{
	return PascalString(m_LayerName, 4u);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
LayerRecords::LayerBlendingRanges Layer<T>::generateBlendingRanges(const Enum::ColorMode colorMode)
{
	using Data = std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>;
	LayerRecords::LayerBlendingRanges blendingRanges{};

	// Note that all of these color modes seem to have an additional blending range for alpha which cannot be accessed
	// through the photoshop ui. This might be for legacy reasons
	if (colorMode == Enum::ColorMode::RGB)
	{
		Data sourceRanges;
		Data destinationRanges;
		for (int i = 0; i < 5u; ++i)
		{
			sourceRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			destinationRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			blendingRanges.m_Size += 8u;
		}
		blendingRanges.m_SourceRanges = sourceRanges;
		blendingRanges.m_DestinationRanges = destinationRanges;
	}
	else if (colorMode == Enum::ColorMode::Grayscale)
	{
		Data sourceRanges;
		Data destinationRanges;
		for (int i = 0; i < 2u; ++i)
		{
			sourceRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			destinationRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			blendingRanges.m_Size += 8u;
		}
		blendingRanges.m_SourceRanges = sourceRanges;
		blendingRanges.m_DestinationRanges = destinationRanges;
	}
	else if (colorMode == Enum::ColorMode::CMYK)
	{
		Data sourceRanges;
		Data destinationRanges;
		for (int i = 0; i < 6u; ++i)
		{
			sourceRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			destinationRanges.push_back(std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0u, 0u, 255u, 255u));
			blendingRanges.m_Size += 8u;
		}
		blendingRanges.m_SourceRanges = sourceRanges;
		blendingRanges.m_DestinationRanges = destinationRanges;
	}
	else
	{
		PSAPI_LOG_ERROR("Layer", "Currently only RGB, Grayscale and CMYK colormodes are supported for layer blending ranges")
	}
	return blendingRanges;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
std::optional<std::tuple<LayerRecords::ChannelInformation, std::unique_ptr<BaseImageChannel>>> Layer<T>::extractLayerMask(bool doCopy)
{
	if (!m_LayerMask.has_value())
	{
		return std::nullopt;
	}

	auto& maskImgChannel = m_LayerMask.value().maskData;
	Enum::ChannelIDInfo maskIdInfo{ Enum::ChannelID::UserSuppliedLayerMask, -2 };
	LayerRecords::ChannelInformation channelInfo{ maskIdInfo, maskImgChannel.m_OrigByteSize };

	// TODO this might not be doing much at all
	if (doCopy)
	{
		std::unique_ptr<BaseImageChannel> imgData = std::make_unique<BaseImageChannel>(maskImgChannel);
		std::tuple<LayerRecords::ChannelInformation, std::unique_ptr<BaseImageChannel>> data = std::make_tuple(channelInfo, std::move(imgData));
		return std::optional(std::move(data));
	}
	else
	{
		std::unique_ptr<BaseImageChannel> imgData = std::make_unique<BaseImageChannel>(std::move(maskImgChannel));
		std::tuple<LayerRecords::ChannelInformation, std::unique_ptr<BaseImageChannel>> data = std::make_tuple(channelInfo, std::move(imgData));
		return std::optional(std::move(data));
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
std::tuple<LayerRecord, ChannelImageData> Layer<T>::toPhotoshop(const Enum::ColorMode colorMode, const bool doCopy)
{
	std::vector<LayerRecords::ChannelInformation> channelInfo{};	// Just have this be empty
	ChannelImageData channelData{};

	auto extents = this->generateExtents();
	int32_t top = std::get<0>(extents);
	int32_t left = std::get<1>(extents);
	int32_t bottom = std::get<2>(extents);
	int32_t right = std::get<3>(extents);

	LayerRecord lrRecord(
		PascalString(m_LayerName, 4u),	// Photoshop does sometimes explicitly write out the name such as '</Group 1>' to indicate what it belongs to 
		top,
		left,
		bottom,
		right,
		0u,		// Number of channels, photoshop does appear to actually write out all the channels with 0 length, we will see later if that is a requirement
		channelInfo,
		m_BlendMode,
		m_Opacity,
		0u,		// Clipping
		LayerRecords::BitFlags(false, m_IsVisible, false),
		std::nullopt,	// LayerMaskData
		Layer<T>::generateBlendingRanges(colorMode),	// Generate some defaults
		std::nullopt	// Additional layer information
	);

	return std::make_tuple(std::move(lrRecord), std::move(channelData));
}



PSAPI_NAMESPACE_END