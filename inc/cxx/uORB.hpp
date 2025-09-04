#ifndef UORB_CXX_UORB_HPP_
#define UORB_CXX_UORB_HPP_

#include <stdint.h>
#include <rtthread.h>

#include "uORB.h"

namespace uORB {

class NonCopyable {
protected:
	NonCopyable() = default;
	~NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

// Publication for single-instance topics
// Usage:
//   uORB::Publication<topic_s> pub{ORB_ID(topic)};
//   topic_s t{}; pub.publish(t);

template<typename T>
class Publication : public NonCopyable {
public:
	explicit Publication(orb_id_t meta, unsigned queue_size = 1)
		: _meta(meta), _handle(nullptr), _queue_size(queue_size) {}

	Publication(Publication&& other) noexcept
		: _meta(other._meta), _handle(other._handle), _queue_size(other._queue_size) { other._handle = nullptr; }

	Publication& operator=(Publication&& other) noexcept {
		if (this != &other) {
			_meta = other._meta;
			_handle = other._handle;
			_queue_size = other._queue_size;
			other._handle = nullptr;
		}
		return *this;
	}

	~Publication() = default;

	int advertise(const T& initial) {
		if (_handle) return RT_EOK;
		_handle = (_queue_size > 1)
			? orb_advertise_queue(_meta, &initial, _queue_size)
			: orb_advertise(_meta, &initial);
		return _handle ? RT_EOK : -RT_ERROR;
	}

	int publish(const T& data) {
		if (!_handle) {
			// lazy advertise with provided data as initial
			_handle = (_queue_size > 1)
				? orb_advertise_queue(_meta, &data, _queue_size)
				: orb_advertise(_meta, &data);
			if (!_handle) return -RT_ERROR;
		}
		return orb_publish(_meta, _handle, &data);
	}

	orb_advert_t handle() const { return _handle; }

private:
	orb_id_t _meta{nullptr};
	orb_advert_t _handle{nullptr};
	unsigned _queue_size{1};
};

// Publication for multi-instance topics
// Usage:
//   uORB::PublicationMulti<topic_s> pub{ORB_ID(topic)};
//   pub.publish(t);

template<typename T>
class PublicationMulti : public NonCopyable {
public:
	explicit PublicationMulti(orb_id_t meta, unsigned queue_size = 1)
		: _meta(meta), _handle(nullptr), _queue_size(queue_size), _instance(-1) {}

	PublicationMulti(PublicationMulti&& other) noexcept
		: _meta(other._meta), _handle(other._handle), _queue_size(other._queue_size), _instance(other._instance) {
		other._handle = nullptr;
		other._instance = -1;
	}

	PublicationMulti& operator=(PublicationMulti&& other) noexcept {
		if (this != &other) {
			_meta = other._meta;
			_handle = other._handle;
			_queue_size = other._queue_size;
			_instance = other._instance;
			other._handle = nullptr;
			other._instance = -1;
		}
		return *this;
	}

	~PublicationMulti() = default;

	int advertise(const T& initial) {
		if (_handle) return RT_EOK;
		int inst = -1;
		_handle = (_queue_size > 1)
			? orb_advertise_multi_queue(_meta, &initial, &inst, _queue_size)
			: orb_advertise_multi(_meta, &initial, &inst);
		if (_handle) {
			_instance = inst;
			return RT_EOK;
		}
		return -RT_ERROR;
	}

	int publish(const T& data) {
		if (!_handle) {
			int inst = -1;
			_handle = (_queue_size > 1)
				? orb_advertise_multi_queue(_meta, &data, &inst, _queue_size)
				: orb_advertise_multi(_meta, &data, &inst);
			if (!_handle) return -RT_ERROR;
			_instance = inst;
		}
		return orb_publish(_meta, _handle, &data);
	}

	int instance() const { return _instance; }
	orb_advert_t handle() const { return _handle; }

private:
	orb_id_t _meta{nullptr};
	orb_advert_t _handle{nullptr};
	unsigned _queue_size{1};
	int _instance{-1};
};

// Subscription wrapper
class Subscription : public NonCopyable {
public:
	explicit Subscription(orb_id_t meta, uint8_t instance = 0)
		: _meta(meta), _instance(instance) {
		_handle = orb_subscribe_multi(_meta, _instance);
	}

	Subscription(Subscription&& other) noexcept
		: _meta(other._meta), _instance(other._instance), _handle(other._handle) {
		other._handle = nullptr;
	}

	Subscription& operator=(Subscription&& other) noexcept {
		if (this != &other) {
			_meta = other._meta;
			_instance = other._instance;
			_handle = other._handle;
			other._handle = nullptr;
		}
		return *this;
	}

	~Subscription() {
		if (_handle) {
			orb_unsubscribe(_handle);
		}
	}

	bool updated() const {
		if (!_handle) return false;
		rt_bool_t u = RT_FALSE;
		// 注意：orb_check 内部会推进订阅者 generation，以避免重复通知
		return (orb_check(_handle, &u) == RT_EOK) && (u == RT_TRUE);
	}

	template<typename U>
	int copy(U* out) const {
		if (!_handle) return -RT_EINVAL;
		return orb_copy(_meta, _handle, static_cast<void*>(out));
	}

	template<typename U>
	bool update(U* out) const {
		// 进行一次检查，如有更新则 copy 消费
		if (!_handle) return false;
		rt_bool_t u = RT_FALSE;
		if (orb_check(_handle, &u) != RT_EOK || !u) return false;
		return copy(out) == RT_EOK;
	}

	int set_interval(unsigned interval_ms) {
		if (!_handle) return -RT_EINVAL;
		return orb_set_interval(_handle, interval_ms);
	}

	int get_interval(unsigned* interval_ms) const {
		if (!_handle) return -RT_EINVAL;
		return orb_get_interval(_handle, interval_ms);
	}

	int wait(int timeout_ms) const {
		if (!_handle) return -RT_EINVAL;
		return orb_wait(_handle, timeout_ms);
	}

	bool exists() const {
		return orb_exists(_meta, _instance) == RT_EOK;
	}

	orb_subscr_t handle() const { return _handle; }
	uint8_t instance() const { return _instance; }

private:
	orb_id_t _meta{nullptr};
	uint8_t _instance{0};
	orb_subscr_t _handle{nullptr};
};

// Subscription with interval preset
class SubscriptionInterval : public Subscription {
public:
	SubscriptionInterval(orb_id_t meta, unsigned interval_ms, uint8_t instance = 0)
		: Subscription(meta, instance) {
		(void)set_interval(interval_ms);
	}
};

} // namespace uORB

#endif // UORB_CXX_UORB_HPP_ 