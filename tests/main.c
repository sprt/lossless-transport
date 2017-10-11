#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "test_packet.h"
#include "test_queue.h"

int main(void) {
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_SuiteInfo suites[] = {
		{"packet", NULL, NULL, pkt_setup, pkt_teardown, packet_tests},
		{"queue", NULL, NULL, setup_queue_test, teardown_queue_test, queue_tests},
		CU_SUITE_INFO_NULL,
	};

	if (CU_register_suites(suites) != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
