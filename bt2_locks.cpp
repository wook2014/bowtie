#include "bt2_locks.h"

#if (__cplusplus >= 201103L)

void mcs_lock::lock() {
	node.next = nullptr;
	node.unlocked = false;

	mcs_node *pred = q.exchange(&node, std::memory_order_release);
	if (pred) {
		pred->next = &node;
		spin_while_eq(node.unlocked, false);
	}
	node.unlocked.load(std::memory_order_acquire);
}

void mcs_lock::unlock() {
	if (!node.next) {
		mcs_node *node_ptr = &node;
		if (q.compare_exchange_strong(node_ptr,
					      (mcs_node *)nullptr,
					      std::memory_order_release))
			return;
		spin_while_eq(node.next, (mcs_node *)nullptr);
	}
	node.next->unlocked.store(true, std::memory_order_release);
}

thread_local mcs_lock::mcs_node mcs_lock::node;

void spin_lock::lock() {
	cpu_backoff backoff;
	while (flag.test_and_set(std::memory_order_acquire))
		backoff.pause();
}

void spin_lock::unlock() {
	flag.clear(std::memory_order_release);
}
#endif
