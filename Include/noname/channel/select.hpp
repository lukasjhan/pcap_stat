#pragma once

#include <utility>
#include <functional>
#include <iostream>
#include <random>
#include <algorithm>

#include "channel.hpp"

namespace noname_core {
	namespace channel {

		namespace {
			template<typename...> constexpr bool dependent_error = false;
		}

		class case_option
		{
			std::function<bool()> task;
		public:
			template<typename T, std::size_t buffer_size, typename func>
			case_option(ochannel<T, buffer_size> ch, func f)
			{
				task = [=]() {
					auto val = ch.get_buffer()->try_get_next();
					if (val)
					{
						f(*val);
					};
					return val == nullptr;
				};
			}

			template<typename T, std::size_t buffer_size, typename func>
			case_option(ichannel<T, buffer_size> ch, func f)
			{
				task = [=]() {
					f();
					return true;
				};
			}

			template<typename T, std::size_t buffer_size, typename func>
			case_option(channel<T, buffer_size> ch, func f) : case_option(ochannel<T, buffer_size>(ch), std::forward<func>(f)) {}

			case_option(const case_option&) = default;
			case_option() { task = []() { return true; }; }

			bool operator() ()
			{
				return task();
			}
		};

		class default_option
		{
			std::function<void()> task;
		public:
			template<typename func>
			default_option(func f)
			{
				task = f;
			}

			void operator() ()
			{
				task();
			}
		};

		class select
		{
			std::vector<case_option> cases;

			bool random_exec()
			{
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(begin(cases), end(cases), g);
				for (auto& c : cases)
				{
					if (!c())
						return true;
				}
				return false;
			}

			template<typename ...T>
			void exec(case_option&& c, T&& ... params)
			{
				cases.emplace_back(c);
				exec(std::forward<T>(params)...);
			}

			void exec(case_option&& c)
			{
				cases.emplace_back(c);
				random_exec();
			}

			void exec(default_option&& d)
			{
				if (!random_exec())
					d();
			}
			template<typename ...T>
			void exec(default_option&& c, T&& ... params)
			{
				static_assert(dependent_error<T...>, "at least 1 option required!");
			}

		public:
			template<typename ...T>
			select(T&& ... params)
			{
				cases.reserve(sizeof... (params));
				exec(std::forward<T>(params)...);
			}
		};
	}
}