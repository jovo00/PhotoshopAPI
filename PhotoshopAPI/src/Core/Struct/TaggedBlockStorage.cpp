#include "TaggedBlockStorage.h"

#include "Macros.h"
#include "Core/FileIO/Read.h"
#include "Core/FileIO/Write.h"
#include "Logger.h"
#include "StringUtil.h"
#include "TaggedBlock.h"

PSAPI_NAMESPACE_BEGIN


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
TaggedBlockStorage::TaggedBlockStorage(std::vector<std::shared_ptr<TaggedBlock>> taggedBlocks)
{
	m_TaggedBlocks = std::move(taggedBlocks);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
uint64_t TaggedBlockStorage::calculateSize(std::shared_ptr<FileHeader> header /*= nullptr*/) const
{
	uint64_t size = 0u;
	for (const auto& block : m_TaggedBlocks)
	{
		// Size gets initialized upon creation of the tagged block so we can just read the value back
		size += block->totalSize<uint64_t>();
	}
	return size;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
std::shared_ptr<T> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const
{
	// iterate all tagged blocks and compare keys as well as trying to cast the pointer
	for (auto& taggedBlock : m_TaggedBlocks)
	{
		if (taggedBlock->getKey() == key)
		{
			if (std::shared_ptr<T> downcastedPtr = std::dynamic_pointer_cast<T>(taggedBlock))
			{
				return downcastedPtr;
			}
		}
	}
	return nullptr;
}

// Instantiate the templates for all the tagged block types 
template std::shared_ptr<TaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<Lr16TaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<Lr32TaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<LrSectionTaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<ReferencePointTaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<UnicodeLayerNameTaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;
template std::shared_ptr<ProtectedSettingTaggedBlock> TaggedBlockStorage::getTaggedBlockView(const Enum::TaggedBlockKey key) const;

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<TaggedBlock> TaggedBlockStorage::readTaggedBlock(File& document, const FileHeader& header, ProgressCallback& callback, const uint16_t padding)
{
	const uint64_t offset = document.getOffset();
	Signature signature = Signature(ReadBinaryData<uint32_t>(document));
	if (signature != Signature("8BIM") && signature != Signature("8B64"))
	{
		PSAPI_LOG_ERROR("LayerRecord", "Signature does not match '8BIM' or '8B64', got '%s' instead",
			uint32ToString(signature.m_Value).c_str());
	}
	std::string keyStr = uint32ToString(ReadBinaryData<uint32_t>(document));
	std::optional<Enum::TaggedBlockKey> taggedBlock = Enum::getTaggedBlockKey<std::string, Enum::TaggedBlockKey>(keyStr);

	if (taggedBlock.has_value())
	{
		if (taggedBlock.value() == Enum::TaggedBlockKey::Lr16)
		{
			auto lr16TaggedBlock = std::make_shared<Lr16TaggedBlock>();
			lr16TaggedBlock->read(document, header, callback, offset, signature, padding);
			this->m_TaggedBlocks.push_back(lr16TaggedBlock);
			return lr16TaggedBlock;
		}
		else if (taggedBlock.value() == Enum::TaggedBlockKey::Lr32)
		{
			auto lr32TaggedBlock = std::make_shared<Lr32TaggedBlock>();
			lr32TaggedBlock->read(document, header, callback, offset, signature, padding);
			this->m_TaggedBlocks.push_back(lr32TaggedBlock);
			return lr32TaggedBlock;
		}
		else if (taggedBlock.value() == Enum::TaggedBlockKey::lrSectionDivider)
		{
			auto lrSectionTaggedBlock = std::make_shared<LrSectionTaggedBlock>();
			lrSectionTaggedBlock->read(document, offset, signature, padding);
			this->m_TaggedBlocks.push_back(lrSectionTaggedBlock);
			return lrSectionTaggedBlock;
		}
		else if (taggedBlock.value() == Enum::TaggedBlockKey::lrReferencePoint)
		{
			auto lrReferencePointBlock = std::make_shared<ReferencePointTaggedBlock>();
			lrReferencePointBlock->read(document, offset, signature);
			this->m_TaggedBlocks.push_back(lrReferencePointBlock);
			return lrReferencePointBlock;
		}
		else if (taggedBlock.value() == Enum::TaggedBlockKey::lrUnicodeName)
		{
			auto lrUnicodeNameBlock = std::make_shared<UnicodeLayerNameTaggedBlock>();
			lrUnicodeNameBlock->read(document, offset, signature, padding);
			this->m_TaggedBlocks.push_back(lrUnicodeNameBlock);
			return lrUnicodeNameBlock;
		}
		else if (taggedBlock.value() == Enum::TaggedBlockKey::lrProtectedSetting)
		{
			auto lrProtectionTaggedBlock = std::make_shared<ProtectedSettingTaggedBlock>();
			lrProtectionTaggedBlock->read(document, offset, signature);
			this->m_TaggedBlocks.push_back(lrProtectionTaggedBlock);
			return lrProtectionTaggedBlock;
		}
		else
		{
			auto baseTaggedBlock = std::make_shared<TaggedBlock>();
			baseTaggedBlock->read(document, header, offset, signature, taggedBlock.value(), padding);
			this->m_TaggedBlocks.push_back(baseTaggedBlock);
			return baseTaggedBlock;
		}
	}
	else
	{
		PSAPI_LOG_ERROR("TaggedBlock", "Could not find tagged block from key '%s'", keyStr.c_str());
		return nullptr;
	}
}

void TaggedBlockStorage::write(File& document, const FileHeader& header, ProgressCallback& callback, const uint16_t padding) const
{
	for (const auto& block : m_TaggedBlocks)
	{
		block->write(document, header, callback, padding);
	}
	// Since the tagged blocks themselves are aligned to padding we dont need to pad the rest of this section manually
}

PSAPI_NAMESPACE_END