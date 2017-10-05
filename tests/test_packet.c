#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"

pkt_t *pkt = NULL;

void pkt_setup(void) {
	pkt = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);
}

void pkt_teardown(void) {
	pkt_del(pkt);
	pkt = NULL;
}

void test_pkt_new(void) {
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
}

void test_pkt_set_type(void) {
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
}

void test_pkt_set_tr(void) {
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 1);

	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);

	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 3), E_TR);
	CU_ASSERT_NOT_EQUAL(pkt_get_tr(pkt), 3);
}

void test_pkt_set_window(void) {
	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE);

	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE + 1), E_WINDOW);
	CU_ASSERT_NOT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE + 1);
}

void test_pkt_set_seqnum(void) {
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 100), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 100);
}

void test_pkt_set_length(void) {
	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), MAX_PAYLOAD_SIZE);

	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE + 1), E_LENGTH);
	CU_ASSERT_NOT_EQUAL(pkt_get_length(pkt), MAX_PAYLOAD_SIZE + 1);
}

void test_pkt_get_length(void) {
	// Test pkt_get_length returns 0 when TR is 1
	char payload[] = "hello";
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, payload, strlen(payload)), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), strlen(payload));
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);
}

void test_pkt_set_timestamp(void) {
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 0xffffffff), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 0xffffffff);
}

void test_pkt_set_crc1(void) {
	CU_ASSERT_EQUAL(pkt_set_crc1(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 1);
}

void test_pkt_set_payload(void) {
	char hello[] = "hello";
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello, strlen(hello)), PKT_OK);
	CU_ASSERT_NSTRING_EQUAL(pkt_get_payload(pkt), hello, strlen(hello));
	CU_ASSERT_EQUAL(pkt_get_length(pkt), strlen(hello));

	char too_large[MAX_PAYLOAD_SIZE + 1] = "foo";
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, too_large, MAX_PAYLOAD_SIZE + 1), E_LENGTH);
	CU_ASSERT_NSTRING_NOT_EQUAL(pkt_get_payload(pkt), too_large, MAX_PAYLOAD_SIZE);
	CU_ASSERT_NOT_EQUAL(pkt_get_length(pkt), MAX_PAYLOAD_SIZE + 1);

	// TODO: test CRC1 is computed with TR set to 0
}

void test_pkt_set_crc2(void) {
	CU_ASSERT_EQUAL(pkt_set_crc2(pkt, 0xffffffff), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 0xffffffff);
}

int main(void) {
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_TestInfo packet_tests[] = {
		{"pkt_new", test_pkt_new},
		{"pkt_set_type", test_pkt_set_type},
		{"pkt_set_tr", test_pkt_set_tr},
		{"pkt_set_window", test_pkt_set_window},
		{"pkt_set_seqnum", test_pkt_set_seqnum},
		{"pkt_set_length", test_pkt_set_length},
		{"pkt_get_length", test_pkt_set_length},
		{"pkt_set_timestamp", test_pkt_set_timestamp},
		{"pkt_set_crc1", test_pkt_set_crc1},
		{"pkt_set_payload", test_pkt_set_payload},
		{"pkt_set_crc2", test_pkt_set_crc2},
		CU_TEST_INFO_NULL,
	};

	CU_SuiteInfo suites[] = {
		{"packet", NULL, NULL, pkt_setup, pkt_teardown, packet_tests},
		CU_SUITE_INFO_NULL,
	};

	if (CU_register_suites(suites) != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
