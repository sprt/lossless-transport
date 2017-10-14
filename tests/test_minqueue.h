#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"
#include "../src/minqueue.h"

#define QUEUE_MAXSIZE 3

pkt_t *p[QUEUE_MAXSIZE];
minqueue_t *q = NULL;

void setup_queue_test(void) {
	q = queue_create(QUEUE_MAXSIZE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(q);

	for (size_t i = 0; i < QUEUE_MAXSIZE; i++) {
		p[i] = pkt_new();
		CU_ASSERT_PTR_NOT_NULL_FATAL(p[i]);
		CU_ASSERT_EQUAL_FATAL(pkt_set_seqnum(p[i], i), PKT_OK);
		CU_ASSERT_EQUAL_FATAL(pkt_set_timestamp(p[i], i), PKT_OK);
	}
}

void teardown_queue_test(void) {
	queue_free(q);
	q = NULL;

	for (size_t i = 0; i < QUEUE_MAXSIZE; i++) {
		pkt_del(p[i]);
		p[i] = NULL;
	}
}

void test_queue_add_peek_remove(void) {
	// We should be able to add only up to QUEUE_MAXSIZE elements
	for (ssize_t i = QUEUE_MAXSIZE - 1; i >= 0; i--) {
		CU_ASSERT_NOT_EQUAL(queue_push(q, p[i]), -1);
		CU_ASSERT_EQUAL(pkt_get_timestamp(queue_peek(q)), i);
	}

	// Reached the maximum; adding should fail
	CU_ASSERT_EQUAL(queue_push(q, p[0]), -1);

	// Empty the queue
	for (size_t i = 0; i < QUEUE_MAXSIZE; i++) {
		CU_ASSERT_EQUAL(pkt_get_timestamp(queue_pop(q)), i);
	}

	// It's empty now, can't remove anything
	CU_ASSERT_PTR_NULL(queue_pop(q));
}

void test_queue_update(void) {
	for (ssize_t i = QUEUE_MAXSIZE - 1; i >= 0; i--) {
		CU_ASSERT_NOT_EQUAL(queue_push(q, p[i]), -1);
	}

	CU_ASSERT_EQUAL(queue_update(q, 0, 5), 0);
	CU_ASSERT_EQUAL(queue_update(q, QUEUE_MAXSIZE, 5), -1);

	CU_ASSERT_EQUAL(pkt_get_seqnum(queue_pop(q)), 1);
	CU_ASSERT_EQUAL(pkt_get_seqnum(queue_pop(q)), 2);
	CU_ASSERT_EQUAL(pkt_get_seqnum(queue_pop(q)), 0);
}

CU_TestInfo queue_tests[] = {
	{"queue_add_peek_remove", test_queue_add_peek_remove},
	{"queue_update", test_queue_update},
	CU_TEST_INFO_NULL,
};
