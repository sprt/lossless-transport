#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include "../src/packet_interface.h"

#define MAX_PACKET_SIZE 1 + 1 + 2 + 4 + 4 + MAX_PAYLOAD_SIZE + 4

static pkt_t *pkt = NULL;

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

	CU_ASSERT_EQUAL(pkt_set_length(pkt, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);
	CU_ASSERT_PTR_NULL(pkt_get_payload(pkt));
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
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, NULL, 10), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);
	CU_ASSERT_PTR_NULL(pkt_get_payload(pkt));

	char hello[] = "hello";
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello, strlen(hello)), PKT_OK);
	CU_ASSERT_NSTRING_EQUAL(pkt_get_payload(pkt), hello, strlen(hello));
	CU_ASSERT_EQUAL(pkt_get_length(pkt), strlen(hello));

	char too_large[MAX_PAYLOAD_SIZE + 1] = "foo";
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, too_large, MAX_PAYLOAD_SIZE + 1), E_LENGTH);
	CU_ASSERT_NSTRING_NOT_EQUAL(pkt_get_payload(pkt), too_large, MAX_PAYLOAD_SIZE);
	CU_ASSERT_NOT_EQUAL(pkt_get_length(pkt), MAX_PAYLOAD_SIZE + 1);

	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);
	CU_ASSERT_PTR_NULL(pkt_get_payload(pkt));
}

void test_pkt_set_crc2(void) {
	CU_ASSERT_EQUAL(pkt_set_crc2(pkt, 0xffffffff), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 0xffffffff);
}

void test_pkt_decode(void) {
	/*
	 * CRC1 example
	 * ------------
	 *
	 * Original CRC:			0x33dae306
	 * Representation in memory:		06 e3 da 33 (little-endian)
	 * Converted to network byte order:	33 da e3 06 (big-endian)
	 * Converted back to host byte order:	06 e3 da 33 (little-endian)
	 * Decoded CRC:				0x33dae306
	 */

	unsigned char raw[] = {
		0x5c, // Window, TR, Type
		0x7b, // Seqnum
		0x00, 0x0b, // Length
		0x17, 0x00, 0x00, 0x00, // Timestamp
		0x33, 0xda, 0xe3, 0x06, // CRC1
		0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, // Payload
		0x6c, 0x64, // Payload
		0x0d, 0x4a, 0x11, 0x85, // CRC2
	};

	CU_ASSERT_EQUAL_FATAL(pkt_decode((const char *) raw, sizeof (raw) / sizeof (*raw), pkt), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), 28);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 0x7b);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 0x17);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 0x33dae306);
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 0x0d4a1185);

	char hello_world[] = "hello world";
	CU_ASSERT_EQUAL_FATAL(pkt_get_length(pkt), strlen(hello_world));
	CU_ASSERT_NSTRING_EQUAL(pkt_get_payload(pkt), hello_world, strlen(hello_world));
}

void test_pkt_decode_len(void) {
	char buf[MAX_PACKET_SIZE] = {0};

	// Test it returns E_NOHEADER with an empty buffer
	size_t n = MAX_PACKET_SIZE;
	CU_ASSERT_EQUAL(pkt_encode(pkt, buf, &n), PKT_OK);
	CU_ASSERT_EQUAL(pkt_decode(NULL, MAX_PACKET_SIZE, pkt), E_NOHEADER);
	CU_ASSERT_EQUAL(pkt_decode(buf, 0, pkt), E_NOHEADER);

	// Test it returns E_UNCONSISTENT if the buffer is less than the size of
	// the packet (indicated by the Length field)
	n = MAX_PACKET_SIZE;
	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE), PKT_OK);
	CU_ASSERT_EQUAL(pkt_encode(pkt, buf, &n), PKT_OK);
	CU_ASSERT_EQUAL(pkt_decode(buf, MAX_PACKET_SIZE - 1, pkt), E_UNCONSISTENT);

	// Test it returns E_UNCONSISTENT if the buffer is greater than the size
	// of the packet (indicated by the Length field)
	n = MAX_PACKET_SIZE - 1;
	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE - 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_encode(pkt, buf, &n), PKT_OK);
	CU_ASSERT_EQUAL(pkt_decode(buf, MAX_PACKET_SIZE, pkt), E_UNCONSISTENT);
}

void test_pkt_encode_decode(void) {
	char encoded[MAX_PACKET_SIZE] = {0};
	char hello_world[] = "hello world";
	size_t n = MAX_PACKET_SIZE;

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_window(pkt, 28), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 0x7b), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 0x17), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello_world, strlen(hello_world)), PKT_OK);

	CU_ASSERT_EQUAL(pkt_encode(pkt, encoded, &n), PKT_OK);

	pkt_t *pkt2 = pkt_new();
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkt2);
	CU_ASSERT_EQUAL(pkt_decode(encoded, n, pkt2), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt2), pkt_get_type(pkt));
	CU_ASSERT_EQUAL(pkt_get_tr(pkt2), pkt_get_tr(pkt));
	CU_ASSERT_EQUAL(pkt_get_window(pkt2), pkt_get_window(pkt));
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt2), pkt_get_seqnum(pkt));
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt2), pkt_get_timestamp(pkt));
	uint16_t length = pkt_get_length(pkt2);
	CU_ASSERT_EQUAL_FATAL(length, pkt_get_length(pkt));
	CU_ASSERT_NSTRING_EQUAL(pkt_get_payload(pkt2), pkt_get_payload(pkt), length);

	pkt_del(pkt2);
}

void test_pkt_encode_tr0(void) {
	char actual[MAX_PACKET_SIZE] = {0};
	char hello_world[] = "hello world";
	size_t n = MAX_PACKET_SIZE;

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK); // relevant
	CU_ASSERT_EQUAL(pkt_set_window(pkt, 28), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 0x7b), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 0x17), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello_world, strlen(hello_world)), PKT_OK);
	CU_ASSERT_EQUAL(pkt_encode(pkt, actual, &n), PKT_OK);

	unsigned char expected[] = {
		0x5c, // Window, TR, Type
		0x7b, // Seqnum
		0x00, 0x0b, // Length
		0x17, 0x00, 0x00, 0x00, // Timestamp
		0x33, 0xda, 0xe3, 0x06, // CRC1
		0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, // Payload
		0x6c, 0x64, // Payload
		0x0d, 0x4a, 0x11, 0x85, // CRC2
	};

	CU_ASSERT_EQUAL_FATAL(n, sizeof (expected) / sizeof (*expected));
	CU_ASSERT_NSTRING_EQUAL(actual, expected, n);
}

void test_pkt_encode_tr1(void) {
	char actual[MAX_PACKET_SIZE] = {0};
	char hello_world[] = "hello world";
	size_t n = MAX_PACKET_SIZE;

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK); // relevant
	CU_ASSERT_EQUAL(pkt_set_window(pkt, 28), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 0x7b), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 0x17), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello_world, strlen(hello_world)), PKT_OK);
	CU_ASSERT_EQUAL(pkt_encode(pkt, actual, &n), PKT_OK);

	unsigned char expected[] = {
		0x7c, // Window, TR, Type
		0x7b, // Seqnum
		0x00, 0x0b, // Length
		0x17, 0x00, 0x00, 0x00, // Timestamp
		0x33, 0xda, 0xe3, 0x06, // CRC1
	};

	CU_ASSERT_EQUAL_FATAL(n, sizeof (expected) / sizeof (*expected));
	CU_ASSERT_NSTRING_EQUAL(actual, expected, n);
}

void test_pkt_encode_length0(void) {
	char actual[MAX_PACKET_SIZE] = {0};
	char hello_world[] = "hello world";
	size_t n = MAX_PACKET_SIZE;

	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK); // relevant
	CU_ASSERT_EQUAL(pkt_set_window(pkt, 28), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 0x7b), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 0x17), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, hello_world, strlen(hello_world)), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_length(pkt, 0), PKT_OK); // relevant
	CU_ASSERT_EQUAL(pkt_encode(pkt, actual, &n), PKT_OK);

	unsigned char expected[] = {
		0x5c, // Window, TR, Type
		0x7b, // Seqnum
		0x00, 0x00, // Length
		0x17, 0x00, 0x00, 0x00, // Timestamp
		0x33, 0xda, 0xe3, 0x06, // CRC1
	};

	CU_ASSERT_EQUAL_FATAL(n, sizeof (expected) / sizeof (*expected));
	CU_ASSERT_NSTRING_EQUAL(actual, expected, n);
}

void test_pkt_encode_computes_crc1_with_tr0(void) {
	char buf_tr0[MAX_PACKET_SIZE] = {0};
	size_t n_tr0 = MAX_PACKET_SIZE;
	CU_ASSERT_EQUAL(pkt_encode(pkt, buf_tr0, &n_tr0), PKT_OK);

	char buf_tr1[MAX_PACKET_SIZE] = {0};
	size_t n_tr1 = MAX_PACKET_SIZE;
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_encode(pkt, buf_tr1, &n_tr1), PKT_OK);

	CU_ASSERT_EQUAL_FATAL(n_tr0, n_tr1);

	// Test CRCs are the same regardless of the TR field
	size_t offset = 1 + 1 + 2 + 4;
	CU_ASSERT_NSTRING_EQUAL(buf_tr0 + offset, buf_tr1 + offset, sizeof (uint32_t));

	// Test CRCs are correct and stored in network byte order
	unsigned char crc[] = {0x4c, 0xbf, 0x1d, 0x84};
	CU_ASSERT_NSTRING_EQUAL(buf_tr0 + offset, crc, sizeof (uint32_t));
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
		{"pkt_decode", test_pkt_decode},
		{"pkt_decode_len", test_pkt_decode_len},
		{"pkt_encode_decode", test_pkt_encode_decode},
		{"pkt_encode_tr0", test_pkt_encode_tr0},
		{"pkt_encode_tr1", test_pkt_encode_tr1},
		{"pkt_encode_length0", test_pkt_encode_length0},
		{"pkt_encode_computes_crc1_with_tr0", test_pkt_encode_computes_crc1_with_tr0},
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
