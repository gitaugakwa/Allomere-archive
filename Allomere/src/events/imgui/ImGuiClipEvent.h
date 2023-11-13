#pragma once

#include "events/Event.h"

#include "editor/GUI/components/clip/ImGuiClip.h"
#include "ImGuiFocusedEvent.h"

namespace Allomere {

	class ImGuiClipClickedEvent : public Event
	{
	public:
		ImGuiClipClickedEvent(size_t clipID, std::weak_ptr<GUI::ImGuiClip> clip)
			: m_ClipID(clipID), m_Clip(clip) {}

		size_t GetID() const { return m_ClipID; }
		std::weak_ptr<GUI::ImGuiClip> GetClip() const { return m_Clip; }

		//size_t GetStartFrame() const { return m_StartFrame; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipClickedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipClicked)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
		std::weak_ptr<GUI::ImGuiClip> m_Clip;
	};

	class ImGuiClipResizedEvent : public Event
	{
	public:
		ImGuiClipResizedEvent(size_t clipID, float left, float right)
			: m_ClipID(clipID), m_Left(left), m_Right(right) {}


		size_t GetID() const { return m_ClipID; }

		float GetLeft() const { return m_Left; }
		float GetRight() const { return m_Right; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipResizedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipResized)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
		float m_Left, m_Right;
	};

	class ImGuiClipMovedEvent : public Event
	{
	public:
		ImGuiClipMovedEvent(size_t clipID)
			: m_ClipID(clipID) {}


		size_t GetID() const { return m_ClipID; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipMovedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipMoved)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};

	class ImGuiClipDraggedEvent : public Event
	{
	public:
		ImGuiClipDraggedEvent(size_t clipID)
			: m_ClipID(clipID) {}


		size_t GetID() const { return m_ClipID; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipDraggedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipDragged)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};

	class ImGuiClipReleasedEvent : public Event
	{
	public:
		ImGuiClipReleasedEvent(size_t clipID)
			: m_ClipID(clipID) {}


		size_t GetID() const { return m_ClipID; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipReleasedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipReleased)
		EVENT_CLASS_CATEGORY(EventCategoryClip)
	private:
		size_t m_ClipID;
	};

	class ImGuiClipFocusedEvent : public ImGuiFocusedEvent
	{
	public:
		ImGuiClipFocusedEvent(size_t clipID, std::weak_ptr<Focusable> focusable, size_t startFrame)
			: ImGuiFocusedEvent(focusable), m_ClipID(clipID), m_StartFrame(startFrame) {}


		size_t GetID() const { return m_ClipID; }

		size_t GetStartFrame() const { return m_StartFrame; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiClipFocusedEvent: ClipID=" << m_ClipID;
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiClipFocused)
		EVENT_CLASS_CATEGORY(EventCategoryClip)

	private:
		size_t m_ClipID, m_StartFrame;
	};
}