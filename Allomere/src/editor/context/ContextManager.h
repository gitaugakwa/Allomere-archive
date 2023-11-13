#pragma once
#include <vector>
#include <memory>
#include <shared_mutex>

namespace Allomere {
	namespace Context {
		template <typename T>
		class ContextManager
		{
		private:
			class Context;
		
		public:
			const Context read()
			{
				return Context(mContext, std::shared_lock(mMutex));
			}

			Context write()
			{
				return Context(mContext, std::unique_lock(mMutex));
			}

			virtual ~ContextManager() = default;

		private:
			class Context
			{
			public:
				Context(std::shared_ptr<T> context, std::shared_lock<std::shared_mutex> lock)
					: mContext(context), mShared(std::move(lock)) {}
				Context(std::shared_ptr<T> context, std::unique_lock<std::shared_mutex> lock)
					: mContext(context), mUnique(std::move(lock)) {}

				T* operator->()
				{
					return mContext.get();
				}

				const T* operator->() const
				{
					return mContext.get();
				}

				T& operator*()
				{
					return *mContext;
				}

				const T& operator*() const
				{
					return *mContext;
				}

				Context(Context&&) = default;

				/*Context(const Context& rhs)
				{
					this->mShared = std::move(rhs.mShared);
					this->mUnique = std::move(rhs.mUnique);
				}*/

			private:


			private:
				std::shared_ptr<T> mContext;
				std::shared_lock<std::shared_mutex> mShared;
				std::unique_lock<std::shared_mutex> mUnique;
			};

		private:
			std::shared_ptr<T> mContext = std::make_shared<T>();
			std::shared_mutex mMutex;
		};
	}
}