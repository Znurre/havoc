#pragma once

#include <atomic>
#include <memory>
#include <optional>

namespace havoc
{
	static inline std::atomic_uint64_t _counter;

	template <typename T>
	struct optional
	{
		optional() = default;

		optional(T&& value)
			: _storage(std::make_unique<T>(value))
		{
		}

		optional(const T& value)
			: _storage(std::make_unique<T>(value))
		{
		}

		optional(optional&& other)
			: _storage(std::move(other._storage))
		{
		}

		optional(const optional& other)
		{
			if (other)
			{
				get_or_create() = *other;
			}
		}

		optional& operator=(T&& value)
		{
			get_or_create() = value;

			return *this;
		}

		optional& operator=(const T& value)
		{
			get_or_create() = value;

			return *this;
		}

		optional& operator=(const optional& other)
		{
			if (other)
			{
				get_or_create() = *other;
			}

			return *this;
		}

		operator bool() const
		{
			return _storage.get();
		}

		T& operator*() const
		{
			return *_storage;
		}

		T& get_or_create()
		{
			if (!_storage)
			{
				_storage = std::make_unique<T>();
			}

			return *_storage;
		}

		void reset()
		{
			_storage.reset();
		}

	private:
		std::unique_ptr<T> _storage;
	};

	template <typename T, typename... Ts>
	struct option
		: option<T>
		, option<Ts...>
	{
		using option<T>::option;
		using option<Ts...>::option;

		using option<T>::operator=;
		using option<Ts...>::operator=;
	};

	template <typename T>
	struct option<T>
	{
		option()
			: _timestamp(0)
		{
		}

		option(T&& value)
			: _storage(value)
			, _timestamp(++_counter)
		{
		}

		option(option<T>&& other)
			: _storage(std::move(other._storage))
			, _timestamp(other._timestamp)
		{
		}

		option(const option<T>& other)
			: _storage(other._storage)
			, _timestamp(other._timestamp)
		{
		}

		option& operator=(T&& value)
		{
			_timestamp = ++_counter;
			_storage = value;

			return *this;
		}

		option& operator=(const option<T>& other)
		{
			_timestamp = other._timestamp;
			_storage = other._storage;

			return *this;
		}

		T& operator*() const
		{
			return _storage.get_or_create();
		}

		size_t timestamp() const
		{
			return _timestamp;
		}

	private:
		mutable optional<T> _storage;

		size_t _timestamp;
	};

	template <typename... T>
	struct one_of : option<T...>
	{
		using option<T...>::option;
		using option<T...>::operator=;
	};

	template <typename T, typename... Ts>
	struct visitor_helper
	{
		static auto visit(size_t timestamp, auto visitor, const auto& input)
			-> std::optional<typename decltype(visitor)::result>
		{
			if (auto& value = static_cast<const option<T>&>(input); value.timestamp() > timestamp)
			{
				if (auto result = visitor_helper<Ts...>::visit(value.timestamp(), visitor, input))
				{
					return *result;
				}
				else
				{
					return visitor.visit(*value);
				}
			}
			else
			{
				return visitor_helper<Ts...>::visit(timestamp, visitor, input);
			}
		}
	};

	template <typename T>
	struct visitor_helper<T>
	{
		static auto visit(size_t timestamp, auto visitor, const auto& input)
			-> std::optional<typename decltype(visitor)::result>
		{
			if (auto& value = static_cast<const option<T>&>(input); value.timestamp() > timestamp)
			{
				return visitor.visit(*value);
			}

			return {};
		}
	};

	template <typename... T>
	auto visit(auto visitor, const one_of<T...>& input) -> decltype(visitor)::result
	{
		if (auto result = visitor_helper<T...>::visit(0, visitor, input))
		{
			return *result;
		}

		return {};
	}

	template <typename T>
	auto visit(auto visitor, const optional<T>& input) -> std::optional<typename decltype(visitor)::result>
	{
		if (input)
		{
			return visitor.visit(*input);
		}

		return {};
	}
};
