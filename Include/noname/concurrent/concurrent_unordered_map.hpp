#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <iterator>

namespace noname_core {
	namespace concurrent {
		namespace {

			constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.75f;
			constexpr float FIRST_SUBMAP_CAPACITY_MULTIPLIER = 1.0f;

			constexpr std::size_t DEFAULT_MAX_NUM_SUBMAPS = 65536;
			constexpr std::size_t NEW_SUBMAPS_CAPACITY_MULTIPLIER = 2;
			constexpr std::size_t FIRST_SUBMAP_MIN_CAPACITY = 11;

			template<typename T>
			bool is_prime(const T& n, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr)
			{
				if (n < 2)		return false;
				if (n == 2)		return true;
				if (n % 2 == 0) return false;

				T div = 3;

				while (div * div <= n) {
					if (n % div == 0)
						return false;
					div += 2;
				}
				return true;
			}

			template<typename T>
			T next_prime(T n, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr)
			{
				if (n <= 2) return 2;
				if (n % 2 == 0) n++;

				while (!is_prime(n)) {
					n += 2;
				}
				return n;
			}
		}

		struct Submap_stats {
			std::size_t capacity;
			std::size_t num_valid_buckets;
			float load_factor;
		};

		struct Stats {
			std::size_t num_submaps;
			std::size_t num_entries;
			std::vector<Submap_stats> submaps_stats;
		};

		template<typename Key, typename Enable = void>
		class SecondHash;

		template<typename Key>
		class SecondHash<Key, typename std::enable_if<std::is_integral<Key>::value>::type> {
		private:
			std::hash<Key> hash;
		public:
			std::size_t operator()(const Key& key) const {
				return hash(~key);
			}
		};

		template <>
		class SecondHash<std::string> {
		private:
			std::hash<std::string> hash;
		public:
			std::size_t operator()(const std::string& key) const {
				std::string tmp(key);
				std::reverse(tmp.begin(), tmp.end());
				return hash(tmp);
			}
		};

		template<typename Key, typename Value,
			typename KeyHash1 = std::hash<Key>,
			typename KeyHash2 = SecondHash<Key>,
			typename key_equal = std::equal_to<Key>>
		class concurrent_unordered_map {
		
		static_assert(std::is_default_constructible<Key>::value, "Key is not default constructible");
		static_assert(std::is_default_constructible<Value>::value, "Value is not default constructible");
		static_assert(std::is_copy_constructible<Key>::value || std::is_move_constructible<Key>::value, "Key is not copy constructible or move constructible");
		static_assert(std::is_copy_constructible<Value>::value || std::is_move_constructible<Value>::value, "Value is not copy constructible or move constructible");
		static_assert(std::is_copy_assignable<Key>::value || std::is_move_assignable<Key>::value, "Key is not copy assignable or move assignable");
		static_assert(std::is_copy_assignable<Value>::value || std::is_move_assignable<Value>::value, "Value is not copy assignable or move assignable");

		public:
			typedef Key key_type;
			typedef Value mapped_type;
			typedef std::pair<Key, Value> entry;

		private:
			KeyHash1 keyHash1;
			KeyHash2 keyHash2;

			struct Bucket {

				enum class State { EMPTY, BUSY, VALID };

				std::atomic<State> state;
				entry entry;

				Bucket() : state(State::EMPTY) { }
			};

			struct Submap {

				key_equal key_equal;
				std::vector<Bucket> buckets;
				float maxload_factor;

				std::atomic<std::size_t> num_valid_buckets;

				Submap(std::size_t capacity, float maxload_factor)
					: buckets(capacity)
					, maxload_factor(maxload_factor)
					, num_valid_buckets(0) { }

				std::size_t get_capacity() const noexcept {
					return buckets.size();
				}

				Bucket& get_bucket(std::size_t index) {
					return buckets[index];
				}

				const Bucket& get_bucket(std::size_t index) const {
					return buckets[index];
				}

				std::size_t get_num_valid_buckets() const noexcept {
					return num_valid_buckets.load(std::memory_order_relaxed);
				}

				void increment_num_valid_buckets() noexcept {
					num_valid_buckets.fetch_add(1, std::memory_order_relaxed);
				}

				std::size_t calculate_probe_increment(std::size_t hash2) const noexcept {
					const std::size_t modulus = get_capacity() - 1;
					return 1 + hash2 % modulus;
				}

				std::pair<std::size_t, bool> find(const Key& key, std::size_t hash1, std::size_t hash2) const 
				{
					const std::size_t startIndex = hash1 % get_capacity();
					const std::size_t probeIncrement = calculate_probe_increment(hash2);
					std::size_t index = startIndex;

					do {
						const Bucket& bucket = get_bucket(index);
						typename Bucket::State bucket_state = bucket.state.load(std::memory_order_relaxed);

						if (bucket_state == Bucket::State::VALID) {
							std::atomic_thread_fence(std::memory_order_acquire);

							if (key_equal(bucket.entry.first, key)) {
								return std::make_pair(index, true);
							}
						}
						else if (bucket_state == Bucket::State::EMPTY) {
							return std::make_pair(0, false);
						}
						index = (index + probeIncrement) % get_capacity();

					} while (index != startIndex);

					return std::make_pair(0, false);
				}

				bool seek(std::size_t& index) const
				{
					while (index < get_capacity()) {
						const Bucket& bucket = get_bucket(index);

						if (bucket.state.load(std::memory_order_relaxed) == Bucket::State::VALID) {
							std::atomic_thread_fence(std::memory_order_acquire); // memory fence
							return true;
						}
						index++;
					}
					return false;
				}

				struct FullSubmapException { };

				template<typename KeyType, typename ValueType>
				std::pair<std::size_t, bool> insert(KeyType&& key, std::size_t hash1, std::size_t hash2, ValueType ivalue) {

					Value value = Value();
					bool valueComputed = false;

					const std::size_t startIndex = hash1 % get_capacity();
					const std::size_t probeIncrement = calculate_probe_increment(hash2);
					std::size_t index = startIndex;

					do {
						Bucket& bucket = get_bucket(index);
						typename Bucket::State bucket_state = bucket.state.load(std::memory_order_relaxed);

						if (bucket_state == Bucket::State::EMPTY) {
							if (!valueComputed) {
								value = ivalue;
								valueComputed = true;
							}

							if (bucket.state.compare_exchange_strong(bucket_state, Bucket::State::BUSY, std::memory_order_relaxed)) {

								bucket.entry.first = std::move(key);
								bucket.entry.second = std::move(value);
								bucket.state.store(Bucket::State::VALID, std::memory_order_release);

								increment_num_valid_buckets();

								return std::make_pair(index, true);
							}
						}

						if (bucket_state == Bucket::State::VALID || bucket.state.load(std::memory_order_relaxed) == Bucket::State::VALID) {
							std::atomic_thread_fence(std::memory_order_acquire);

							if (key_equal(bucket.entry.first, key)) {
								return std::make_pair(index, false);
							}
						}
						index = (index + probeIncrement) % get_capacity();
					} while (index != startIndex);

					throw FullSubmapException();
				}

				bool is_overloaded() const noexcept {
					return (float)get_num_valid_buckets() / get_capacity() >= maxload_factor;
				}

				Submap_stats get_stats() const 
				{
					Submap_stats stats;

					stats.capacity = get_capacity();
					stats.num_valid_buckets = get_num_valid_buckets();
					stats.load_factor = (float)stats.num_valid_buckets / stats.capacity;

					return stats;
				}
			};

			float maxload_factor;
			std::atomic<std::size_t> num_submaps;
			std::vector<std::unique_ptr<Submap> > submaps;
			std::atomic<std::size_t> num_entries;
			std::atomic_flag expanding;

			std::size_t get_maxnum_submaps() const noexcept {
				return submaps.size();
			}

			std::unique_ptr<Submap>& get_submap(std::size_t index) {
				return submaps[index];
			}
			const std::unique_ptr<Submap>& get_submap(std::size_t index) const {
				return submaps[index];
			}

			std::size_t get_num_submaps() const noexcept {
				return num_submaps.load(std::memory_order_acquire);
			}

			std::size_t get_last_submap_index() const noexcept {
				return get_num_submaps() - 1;
			}

			void increment_num_submaps() noexcept {
				num_submaps.fetch_add(1, std::memory_order_release);
			}

			void incrementnum_entries() noexcept {
				num_entries.fetch_add(1, std::memory_order_relaxed);
			}

			bool expand() 
			{
				while (expanding.test_and_set(std::memory_order_acquire)) {
					std::this_thread::yield();
				}

				bool result = false;
				const std::size_t num_submapsSnapshot = get_num_submaps();

				if (num_submapsSnapshot == get_maxnum_submaps()) {
					throw std::runtime_error("Error: reached the maximum number of submaps: " + std::to_string(get_maxnum_submaps()));
				}

				const std::size_t lastSubmapIndex = num_submapsSnapshot - 1;
				const Submap& lastSubmap = *get_submap(lastSubmapIndex);

				if (lastSubmap.is_overloaded()) {
					const std::size_t newSubmapCapacity = next_prime(lastSubmap.get_capacity() * NEW_SUBMAPS_CAPACITY_MULTIPLIER);
					get_submap(lastSubmapIndex + 1).reset(new Submap(newSubmapCapacity, maxload_factor));
					increment_num_submaps();
					result = true;
				}

				expanding.clear(std::memory_order_release);
				return result;
			}

		public:
			class const_iterator : public std::iterator<std::input_iterator_tag, const entry> {
				friend class concurrent_unordered_map;

			private:
				const concurrent_unordered_map* map;
				std::size_t submapIndex;
				std::size_t bucketIndex;
				bool end;

				const Submap& get_submap() const {
					return *map->get_submap(submapIndex);
				}

				const Bucket& get_bucket() const {
					return get_submap().get_bucket(bucketIndex);
				}

				const entry& get_entry() const {
					return get_bucket().entry;
				}

				void seek()
				{
					while (!end) {
						if (get_submap().seek(bucketIndex)) {
							return;
						}
						else {
							submapIndex++;
							bucketIndex = 0;
							if (submapIndex > map->get_last_submap_index()) {
								end = true;
								submapIndex = 0;
							}
						}
					}
				}

				void next()
				{
					bucketIndex++;
					seek();
				}

				const_iterator(const concurrent_unordered_map* map, bool end = false) :
					map(map),
					submapIndex(0),
					bucketIndex(0),
					end(end) {
					seek();
				}

				const_iterator(const concurrent_unordered_map* map, std::size_t submapIndex, std::size_t bucketIndex) :
					map(map),
					submapIndex(submapIndex),
					bucketIndex(bucketIndex),
					end(false) {
				}

			public:
				const_iterator()
					: map(nullptr)
					, submapIndex(0)
					, bucketIndex(0)
					, end(true) { }

				bool operator==(const const_iterator& other) const
				{
					return map == other.map &&
						((end && other.end) || (!end && !other.end && submapIndex == other.submapIndex && bucketIndex == other.bucketIndex));
				}

				bool operator!=(const const_iterator& other) const
				{
					return !(*this == other);
				}

				const_iterator& operator++(void)
				{
					next();
					return *this;
				}

				const_iterator operator++(/* not using */ int)
				{
					const_iterator old(*this);
					++(*this);
					return old;
				}

				const entry& operator*() const
				{
					return get_entry();
				}

				const entry* operator->() const
				{
					return &get_entry();
				}
			};

		private:
			const_iterator find_helper(const Key& key, std::size_t hash1, std::size_t hash2, std::size_t lastSubmapIndex) const
			{
				for (long submapIndex = lastSubmapIndex; submapIndex >= 0; submapIndex--) {
					const Submap& submap = *get_submap(submapIndex);
					const std::pair<std::size_t, bool> findResult = submap.find(key, hash1, hash2);

					if (findResult.second) {
						return const_iterator(this, submapIndex, findResult.first);
					}
				}
				return end();
			}

			template<typename KeyType, typename ValueType>
			std::pair<const_iterator, bool> insert_helper(KeyType&& key, std::size_t hash1, std::size_t hash2, ValueType ivalue)
			{

				while (1) {
					const std::size_t lastSubmapIndex = get_last_submap_index();

					if (lastSubmapIndex > 0) {
						const const_iterator findIterator = find_helper(key, hash1, hash2, lastSubmapIndex - 1);
						if (findIterator != end()) {
							return std::make_pair(findIterator, false);
						}
					}

					Submap& lastSubmap = *get_submap(lastSubmapIndex);

					if (lastSubmap.is_overloaded()) {
						expand();
						continue;
					}

					try {
						const std::pair<std::size_t, bool> insertResult = lastSubmap.insert(std::forward<KeyType>(key), hash1, hash2, ivalue);

						if (insertResult.second) {
							incrementnum_entries();
						}

						const const_iterator insertIterator(this, lastSubmapIndex, insertResult.first);
						return std::make_pair(insertIterator, insertResult.second);
					}
					catch (typename Submap::FullSubmapException&) {
						expand();
						continue;
					}

				}

			}

		public:
			concurrent_unordered_map(std::size_t estimatednum_entries = 0,
				float maxload_factor = DEFAULT_MAX_LOAD_FACTOR,
				std::size_t maxnum_submaps = DEFAULT_MAX_NUM_SUBMAPS) :
				maxload_factor(maxload_factor),
				num_submaps(1),
				submaps(maxnum_submaps),
				num_entries(0) {

				expanding.clear();

				if (maxload_factor <= 0.0f || maxload_factor >= 1.0f) {
					throw std::logic_error("Invalid maximum load factor");
				}

				if (maxnum_submaps < 1) {
					throw std::logic_error("Invalid maximum number of submaps");
				}

				const std::size_t firstSubmapCapacity = std::max(
					FIRST_SUBMAP_MIN_CAPACITY,
					next_prime((std::size_t) (FIRST_SUBMAP_CAPACITY_MULTIPLIER * estimatednum_entries / maxload_factor)));

				get_submap(0).reset(new Submap(firstSubmapCapacity, maxload_factor));
			}

			const_iterator find(const Key& key) const {
				return find_helper(key, keyHash1(key), keyHash2(key), get_last_submap_index());
			}

			const Value& at(const Key& key) const {
				const const_iterator findIterator = find(key);
				if (findIterator == end()) {
					throw std::out_of_range("entry not found");
				}
				return findIterator->second;
			}

			const Value& operator[](const Key& key) const {
				return at(key);
			}

			template<typename Value>
			std::pair<const_iterator, bool> insert(const Key& key, Value ivalue) {
				return insert_helper(key, keyHash1(key), keyHash2(key), ivalue);
			}

			template<typename Value>
			std::pair<const_iterator, bool> insert(Key&& key, Value ivalue) {
				return insert_helper(std::move(key), keyHash1(key), keyHash2(key), ivalue);
			}

			std::pair<const_iterator, bool> insert(const entry& entry) {
				return insert(entry.first, [&entry](const Key&) -> const Value & { return entry.second; });
			}

			std::pair<const_iterator, bool> insert(entry&& entry) {
				return insert(std::move(entry.first), [&entry](const Key&) -> Value && { return std::move(entry.second); });
			}

			template<typename... Args>
			std::pair<const_iterator, bool> emplace(Args&& ... args) {
				return insert(entry(std::forward<Args>(args)...));
			}

			std::size_t get_num_entries() const noexcept {
				return num_entries.load(std::memory_order_relaxed);
			}

			std::size_t size() const noexcept {
				return get_num_entries();
			}

			bool empty() const noexcept {
				return get_num_entries() == 0;
			}

			const_iterator begin() const noexcept {
				return const_iterator(this);
			}

			const_iterator cbegin() const noexcept {
				return begin();
			}

			const_iterator end() const noexcept {
				return const_iterator(this, true);
			}

			const_iterator cend() const noexcept {
				return end();
			}

			template<typename FilterFunction>
			concurrent_unordered_map* filter(FilterFunction filterFunction) const
			{
				concurrent_unordered_map* map = new concurrent_unordered_map(get_num_entries());

				for (const entry& entry : *this) {
					if (filterFunction(entry)) {
						map->insert(entry);
					}
				}
				return map;
			}

			concurrent_unordered_map* clone() const {
				return filter([](const entry&) { return true; });
			}

			Stats get_stats() const 
			{
				Stats stats;

				stats.num_entries = get_num_entries();
				stats.num_submaps = get_num_submaps();
				for (std::size_t submapIndex = 0; submapIndex < stats.num_submaps; submapIndex++) {
					const Submap& submap = *get_submap(submapIndex);
					stats.submaps_stats.push_back(submap.get_stats());
				}
				return stats;
			}

			// TODO: make move and copy
			concurrent_unordered_map(concurrent_unordered_map&&) = delete;
			concurrent_unordered_map(const concurrent_unordered_map&) = delete;
		};
	}
}