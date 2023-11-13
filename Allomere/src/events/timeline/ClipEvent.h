#pragma once

#include "events/Event.h"

#include "editor/audio/clip/Clip.h"

#include "editor/context/ClipContext.h"
#include "editor/context/ContextManager.h"

namespace Allomere {

	using ClipContext = Context::ContextManager<Context::ClipContext>;

	class ClipCutEvent : public Event
	{
	public:
		ClipCutEvent(size_t clipID)
			: m_ClipID(clipID) {}

		size_t GetID() const { return m_ClipID; }


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ClipCutEvent: ClipID=" << GetID();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ClipCut)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};

	class ClipBridgedEvent : public Event
	{
	public:
		ClipBridgedEvent(size_t clipID)
			: m_ClipID(clipID) {}

		size_t GetID() const { return m_ClipID; }


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ClipBridgedEvent: ClipID=" << GetID();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ClipBridged)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};
	
	class ClipBeatsGeneratedEvent : public Event
	{
	public:
		ClipBeatsGeneratedEvent(size_t clipID)
			: m_ClipID(clipID) {}

		size_t GetID() const { return m_ClipID; }


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ClipBeatsGeneratedEvent: ClipID=" << GetID();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ClipBeatsGenerated)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};

	class ClipSimilarityGeneratedEvent : public Event
	{
	public:
		ClipSimilarityGeneratedEvent(size_t clipID)
			: m_ClipID(clipID) {}

		size_t GetID() const { return m_ClipID; }


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ClipSimilarityGeneratedEvent: ClipID=" << GetID();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ClipSimilarityGenerated)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};


	class ClipAddedEvent : public Event
	{
	public:
		ClipAddedEvent(std::weak_ptr<ClipContext> wContext, size_t track, size_t startFrame)
			: m_Context(wContext), m_Track(track), m_StartFrame(startFrame) {}

		std::weak_ptr<ClipContext> GetContext() const { return m_Context; }
		size_t GetTrack() const { return m_Track; }
		size_t GetStartFrame() const { return m_StartFrame; }
		//std::weak_ptr<Audio::Track::Clip> GetClip() { return m_Clip; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ClipAddedEvent: ClipID=" << GetContext().lock()->read()->id;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ClipAdded)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		std::weak_ptr<ClipContext> m_Context;
		size_t m_Track, m_StartFrame;
	};
}