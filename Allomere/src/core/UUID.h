#pragma once

namespace Allomere
{

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID &) = default;

		operator uint64_t() const { return m_UUID; }

	private:
		uint64_t m_UUID;
	};

}

namespace std
{
	template <typename T>
	struct hash;

	template <>
	struct hash<Allomere::UUID>
	{
		std::size_t operator()(const Allomere::UUID &uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}
