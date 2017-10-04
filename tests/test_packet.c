#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"

void test_pkt_new(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 0);
	// TODO: complete this
	pkt_del(pkt);
}

void test_pkt_set_type(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_ACK), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_ACK);

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_NACK), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_NACK);

	CU_ASSERT_EQUAL(pkt_set_type(pkt, 0), E_TYPE);
	CU_ASSERT_NOT_EQUAL(pkt_get_type(pkt), 0);

	pkt_del(pkt);
}

void test_pkt_set_tr(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);

	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 1);

	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);

	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 3), E_TR);
	CU_ASSERT_NOT_EQUAL(pkt_get_tr(pkt), 3);

	pkt_del(pkt);
}

void test_pkt_set_crc1(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);

	CU_ASSERT_EQUAL(pkt_set_crc1(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 1);

	pkt_del(pkt);
}

int main(void) {
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_pSuite suite = CU_add_suite("packet", NULL, NULL);
	if (suite == NULL) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (!CU_add_test(suite, "pkt_new", test_pkt_new) ||
	    !CU_add_test(suite, "pkt_set_type", test_pkt_set_type) ||
	    !CU_add_test(suite, "pkt_set_tr", test_pkt_set_tr) ||
	    !CU_add_test(suite, "pkt_set_crc1", test_pkt_set_crc1)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_run_tests();
	CU_cleanup_registry();

	return 0;
}
