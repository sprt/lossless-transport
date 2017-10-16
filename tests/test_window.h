#include <stdlib.h>

#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"
#include "../src/window.h"

window_t *w;

void setup_window(void) {
}

void teardown_window(void) {
	window_free(w);
	w = NULL;
}

void test_window_has(void) {
	w = window_create(2, 4); // 3 [0 1] 2
	CU_ASSERT_TRUE(window_has(w, 0));
	CU_ASSERT_TRUE(window_has(w, 1));
	CU_ASSERT_FALSE(window_has(w, 2));
	CU_ASSERT_FALSE(window_has(w, 3));
}

void test_window_slide(void) {
	w = window_create(2, 4); // 3 [0 1] 2

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
}

CU_TestInfo window_tests[] = {
	{"window_has", test_window_has},
	{"window_slide", test_window_slide},
	CU_TEST_INFO_NULL,
};
