#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"

void test_pkt_new(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 0);
	CU_ASSERT_PTR_NULL(pkt_get_payload(pkt));
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 0);
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

	CU_ASSERT_EQUAL(pkt_set_type(pkt, 10), E_TYPE);
	CU_ASSERT_NOT_EQUAL(pkt_get_type(pkt), 10);

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

void test_pkt_set_window(void) {
	pkt_t *pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);

	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE);

	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE + 1), E_WINDOW);
	CU_ASSERT_NOT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE + 1);

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

	CU_TestInfo packet_tests[] = {
		{"pkt_new", test_pkt_new},
		{"pkt_set_type", test_pkt_set_type},
		{"pkt_set_tr", test_pkt_set_tr},
		{"pkt_set_window", test_pkt_set_window},
		{"pkt_set_crc1", test_pkt_set_crc1},
		CU_TEST_INFO_NULL,
	};

	CU_SuiteInfo suites[] = {
		{"packet", NULL, NULL, NULL, NULL, packet_tests},
		CU_SUITE_INFO_NULL,
	};

	if (CU_register_suites(suites) != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
