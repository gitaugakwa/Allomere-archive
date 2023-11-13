#pragma once
#include <string>
#include <vector>
#include <memory>

namespace Allomere {
	namespace Context {

		class BaseClipContext
		{
		public:
			BaseClipContext() :mId{ sId++ } {};
			~BaseClipContext() {};

			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseClipContext(const BaseClipContext&)
				: mId(sId++) {};
			BaseClipContext(BaseClipContext&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseClipContext& operator=(const BaseClipContext&) {
				//mId = sId++;
				return *this;
			};
			BaseClipContext& operator=(BaseClipContext&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 0;
			size_t mId;
		};

		struct Similarity
		{
		private:
			class Iterator;
		public:
			bool calculated{ false };
			float distance;
			size_t reference;
			size_t query;

			typedef Iterator iterator;

			bool operator<(const Similarity& rhs) const {
				if (calculated && rhs.calculated) {
					return distance < rhs.distance;
				}
				return calculated;
			}

		private:
			class Iterator {
			public:
				typedef std::contiguous_iterator_tag iterator_category;
				typedef Similarity value_type;
				typedef size_t difference_type;
				typedef Similarity* pointer;
				typedef Similarity reference;

				Iterator(Similarity* value, size_t stride = 1) : mValue(value), mStride(stride)
				{ }

				Iterator& operator++()
				{
					mValue += mStride;
					return *this;
				}

				Iterator operator+(size_t value)
				{
					return Iterator(mValue + (mStride * value), mStride);
				}

				Similarity& operator*()
				{
					return *mValue;
				}

				bool operator!=(const Iterator& it)
				{
					return mValue != it.mValue || mStride != it.mStride;
				}

			private:
				Similarity* mValue;
				size_t mStride;
			};

		
		};

		struct PartContext
		{
			size_t startFrame, endFrame;
			bool looping;
		};

		struct ClipContext : public BaseClipContext
		{
			size_t id;

			std::string filePath;
			std::vector<PartContext> parts;
			std::shared_ptr<Similarity[]> similarityMatrix;

			std::vector<size_t> beats;
			bool muted;

			size_t playLength() const
			{
				size_t length{ 0 };
				for (auto& part : parts)
				{
					if (part.looping) {
						return SIZE_MAX;
					}
					length += part.endFrame - part.startFrame;
				}
				return length;
			}
			size_t length() const
			{
				return parts.back().endFrame - parts.front().startFrame;
			}
			size_t partLength(size_t index) const
			{
				if (index < parts.size())
				{
					auto& part = parts[index];
					return part.endFrame - part.startFrame;
				}
				return 0;
			}
			std::pair<ma_uint64, ma_uint64> clipPoints() const
			{
				return std::make_pair(parts.front().startFrame, parts.back().endFrame);
			}

			virtual ~ClipContext() = default;
		};
	}
}