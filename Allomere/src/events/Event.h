#pragma once

#include "debug/Instrumentor.h"
#include "core/Base.h"

#include <functional>

namespace Allomere {

	// Events in Allomere are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		//ClipClicked,
		ImGuiFocused,

		ImGuiClipClicked, ImGuiClipResized, ImGuiClipMoved, ImGuiClipDragged, ImGuiClipReleased, ImGuiClipFocused,
		ClipCut, ClipBridged, ClipBeatsGenerated, ClipSimilarityGenerated, ClipAdded,
		ImGuiTrackRefresh,
		TrackCreated,
		ImGuiTimelineSeek, ImGuiTimelineSetPlayState,
		TimelineCreated, TimelineSeek,
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4),
		EventCategoryClip = BIT(5),
		EventCategoryTrack = BIT(6),
		EventCategoryTimeline = BIT(7),
		EventCategoryUI = BIT(8),
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		// F will be deduced by the compiler
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	class Dispatcher
	{
	public:
		// Returning true prevents the event from propagating
		template<typename T>
		using EventFn = std::function<bool(T&)>;

		class BaseSubscription {
		public:
			BaseSubscription()
				: mId(sId++) {};

		protected:
			inline static size_t sId = 1;
			size_t mId;
		};

		class Subscription : public BaseSubscription
		{
			friend Dispatcher;
		public:
			EventFn<Event> mEventFunction;
			Subscription(EventFn<Event> eventFunction)
				: mEventFunction(eventFunction)
			{};

			bool Emit(Event& event) {
				return mEventFunction(event);
			}

			bool operator==(const Subscription& rhs) const
			{
				return mId == rhs.mId;
			}
		};
	public:

		template<typename T>
		Subscription& Subscribe(EventFn<Event> func)
		{
			mSubscriptions[T::GetStaticType()].push_back(Subscription(func));
			return mSubscriptions[T::GetStaticType()].back();
		}


		template<typename T>
		bool Unsubscribe(Subscription& subscription) {
			auto pos = std::find(mSubscriptions[T::GetStaticType()].begin(), mSubscriptions[T::GetStaticType()].end(), subscription);
			if (pos != mSubscriptions[T::GetStaticType()].end()) {
				mSubscriptions[T::GetStaticType()].erase(pos);
				return true;
			}
			return false;
		}


		template<typename T>
		bool Emit(T event)
		{
			// Instance Subscriptions
			if (mSubscriptions.find(event.GetEventType()) != mSubscriptions.end())
			{
				auto&& subscriptions = mSubscriptions.at(event.GetEventType());

				for (auto&& subscription : subscriptions)
					if (subscription.Emit(event))
						return true;
			}

			for (auto&& subscription : mAllEventsSubscriptions)
				if (subscription.Emit(event))
					return true;

			return false;
		}

	private:

		std::map<EventType, std::vector<Subscription>> mSubscriptions;
		std::vector<Subscription> mAllEventsSubscriptions;

	};


	class EventEmmiter {


	public:
		EventEmmiter() = default;

	protected:
		template<typename T>
		bool Emit(T event)
		{
			return mDispatcher.Emit<T>(event);
		}


		template<typename T>
		bool Unsubscribe(Dispatcher::Subscription& subscription)
		{
			return mDispatcher.Unsubscribe<T>(subscription);
		}

		template<typename T>
		Dispatcher::Subscription& Subscribe(Dispatcher::EventFn<Event> func)
		{
			return mDispatcher.Subscribe<T>(func);
		}

		inline static Dispatcher mDispatcher;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

#define EVENT_EMMITER_GLOBAL_DEFINITION public:\
											static EventEmmiter::Subscription& GlobalSubscribe(EventEmmiter::EventFn<Event> func)\
											{\
												return sAllEventsSubscriptions.emplace_back(func);\
											}\
											template<typename T>\
											static EventEmmiter::Subscription& GlobalSubscribe(EventEmmiter::EventFn<Event> func)\
											{\
												sSubscriptions[T::GetStaticType()].push_back(EventEmmiter::Subscription(func));\
												return sSubscriptions[T::GetStaticType()].back();\
											}\
											static bool GlobalUnsubscribe(EventEmmiter::Subscription& subscription) {\
												auto pos = std::find(sAllEventsSubscriptions.begin(), sAllEventsSubscriptions.end(), subscription);\
												if (pos != sAllEventsSubscriptions.end()) {\
													sAllEventsSubscriptions.erase(pos);\
													return true;\
												}\
												return false;\
											}\
											template<typename T>\
											static bool GlobalUnsubscribe(EventEmmiter::Subscription& subscription) {\
												auto pos = std::find(sSubscriptions[T::GetStaticType()].begin(), sSubscriptions[T::GetStaticType()].end(), subscription);\
												if (pos != sSubscriptions[T::GetStaticType()].end()) {\
													sSubscriptions[T::GetStaticType()].erase(pos);\
													return true;\
												}\
												return false;\
											}\
										protected:\
											template<typename T>\
											void Emit(T event)\
											{\
												if (mSubscriptions.find(event.GetEventType()) != mSubscriptions.end())\
												{\
													auto&& subscriptions = mSubscriptions.at(event.GetEventType());\
													for (auto&& subscription : subscriptions)\
														if(subscription.Emit(event))\
															return;\
												}\
												if (sSubscriptions.find(event.GetEventType()) != sSubscriptions.end())\
												{\
													auto&& globalSubscriptions = sSubscriptions.at(event.GetEventType());\
													for (auto&& subscription : globalSubscriptions)\
														if(subscription.Emit(event))\
															return;\
												}\
												for (auto&& subscription : sAllEventsSubscriptions)\
													if(subscription.Emit(event))\
														return;\
											}\
										private:\
											inline static std::map<EventType, std::vector<EventEmmiter::Subscription>> sSubscriptions;\
											inline static std::vector<EventEmmiter::Subscription> sAllEventsSubscriptions{};





}