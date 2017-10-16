#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "test_packet.h"
#include "test_window.h"

int main(void) {
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_SuiteInfo suites[] = {
		{"packet", NULL, NULL, pkt_setup, pkt_teardown, packet_tests},
		{"window", NULL, NULL, setup_window, teardown_window, window_tests},
		CU_SUITE_INFO_NULL,
	};

	if (CU_register_suites(suites) != CUE_SUCCESS) {
		return CU_get_error();
	}

	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
