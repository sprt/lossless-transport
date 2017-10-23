#include <stdlib.h>

#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"
#include "../src/window.h"

window_t *w;

void setup_window(void) {
	w = window_create(2, 4, 3); // 3 [0 1] 2
}

void teardown_window(void) {
	window_free(w);
	w = NULL;
}

void test_window_has(void) {
	CU_ASSERT_TRUE(window_has(w, 0));
	CU_ASSERT_TRUE(window_has(w, 1));
	CU_ASSERT_FALSE(window_has(w, 2));
	CU_ASSERT_FALSE(window_has(w, 3));
}

void test_window_slide(void) {
	window_slide(w); // 3 0 [1 2]
	CU_ASSERT_FALSE(window_has(w, 0));
	CU_ASSERT_TRUE(window_has(w, 1));
	CU_ASSERT_TRUE(window_has(w, 2));
	CU_ASSERT_FALSE(window_has(w, 3));

	window_slide(w); // 3] 0 1 [2
	CU_ASSERT_FALSE(window_has(w, 0));
	CU_ASSERT_FALSE(window_has(w, 1));
	CU_ASSERT_TRUE(window_has(w, 2));
	CU_ASSERT_TRUE(window_has(w, 3));

	window_slide(w); // [3 0] 1 2
	CU_ASSERT_TRUE(window_has(w, 0));
	CU_ASSERT_FALSE(window_has(w, 1));
	CU_ASSERT_FALSE(window_has(w, 2));
	CU_ASSERT_TRUE(window_has(w, 3));
}

void test_window_push(void) {
	pkt_t *p = pkt_new();

	CU_ASSERT_EQUAL(window_push(w, p), 0);
	CU_ASSERT_EQUAL(window_buffer_size(w), 1);

	CU_ASSERT_EQUAL(window_push(w, p), 0);
	CU_ASSERT_EQUAL(window_buffer_size(w), 2);

	pkt_del(p);
}

void test_window_resize(void) {
	CU_ASSERT_EQUAL(window_resize(w, 4), 0);
	CU_ASSERT_EQUAL(window_get_size(w), 4);

	CU_ASSERT_EQUAL(window_resize(w, 5), -1);
	CU_ASSERT_NOT_EQUAL(window_get_size(w), 5);

	/* Test resizing below buffer succeeds but doesn't change buffer size */
	pkt_t *p = pkt_new();
	window_push(w, p); /* buffer size = 1 */
	CU_ASSERT_EQUAL(window_resize(w, 0), 0);
	CU_ASSERT_EQUAL(window_get_size(w), 0);
	CU_ASSERT_EQUAL(window_buffer_size(w), 1);
	pkt_del(p);
}

CU_TestInfo window_tests[] = {
	{"window_has", test_window_has},
	{"window_slide", test_window_slide},
	{"window_push", test_window_push},
	{"window_resize", test_window_resize},
	CU_TEST_INFO_NULL,
};
