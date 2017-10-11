#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"
#include "../src/queue.h"

const size_t QUEUE_MAXSIZE = 3;

pkt_t *p = NULL;
queue_t *q = NULL;

void setup_queue_test(void) {
	q = queue_create(QUEUE_MAXSIZE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(q);

	p = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(p);
}

void teardown_queue_test(void) {
	queue_free(q);
	q = NULL;

	pkt_del(p);
	p = NULL;
}

void test_queue_add_then_remove(void) {
	// We should be able to add only up to QUEUE_MAXSIZE elements
	for (size_t i = 0; i < QUEUE_MAXSIZE; i++) {
		CU_ASSERT_NOT_EQUAL(queue_add(q, p), -1);
	}

	// Reached the maximum; adding should fail
	CU_ASSERT_EQUAL(queue_add(q, p), -1);

	// Empty the queue
	for (size_t i = 0; i < QUEUE_MAXSIZE; i++) {
		CU_ASSERT_PTR_NOT_NULL_FATAL(queue_remove(q));
	}

	// It's empty now, can't remove anything
	CU_ASSERT_PTR_NULL(queue_remove(q));
}

void test_queue_add_remove_interlaced(void) {
	CU_ASSERT_NOT_EQUAL(queue_add(q, p), -1); // 1
	CU_ASSERT_NOT_EQUAL(queue_add(q, p), -1); // 2
	CU_ASSERT_PTR_NOT_NULL(queue_remove(q)); // 1
	CU_ASSERT_NOT_EQUAL(queue_add(q, p), -1); // 2
	CU_ASSERT_PTR_NOT_NULL(queue_remove(q)); // 1
	CU_ASSERT_NOT_EQUAL(queue_add(q, p), -1); // 2
	CU_ASSERT_PTR_NOT_NULL(queue_remove(q)); // 1
	CU_ASSERT_PTR_NOT_NULL(queue_remove(q)); // 0
	CU_ASSERT_PTR_NULL(queue_remove(q));
}

CU_TestInfo queue_tests[] = {
	{"queue_add_then_remove", test_queue_add_then_remove},
	{"queue_add_remove_interlaced", test_queue_add_remove_interlaced},
	CU_TEST_INFO_NULL,
};
