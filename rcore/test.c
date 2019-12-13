#include "test.h"
#include "rebbleos.h"
#include "protocol.h"

#ifndef REBBLEOS_TESTING
#error these routines only get compiled in test mode
#endif

extern struct test __tests_start[];
extern struct test __tests_end[];

enum qemu_test_packet_types {
    QEMU_TEST_LIST_REQUEST  = 0x0000,
    QEMU_TEST_RUN_REQUEST   = 0x0001,
    QEMU_TEST_LIST_RESPONSE = 0x8000,
    QEMU_TEST_COMPLETE      = 0x8001,
};

#define RESPONSE_NAME_MAX_SIZE 256
struct qemu_test_list_response {
    uint16_t opcode;
    uint16_t id;
    uint8_t is_last_test;
    uint8_t name_len;
    char name[RESPONSE_NAME_MAX_SIZE];
} __attribute__((__packed__)); 

struct qemu_test_run_request {
    uint16_t opcode;
    uint16_t id;
} __attribute__((__packed__));

struct qemu_test_complete {
    uint16_t opcode;
    uint8_t passed;
    uint32_t artifact;
} __attribute__((__packed__));

union qemu_test_packet {
    uint16_t opcode;
    struct qemu_test_list_response test_list_response;
    struct qemu_test_run_request test_run_request;
    struct qemu_test_complete test_complete;
} __attribute__((__packed__));

uint8_t test_init() {
    struct test *test;
    int i;
    
    for (test = __tests_start; test < __tests_end; test++) {
        SYS_LOG("Test", APP_LOG_LEVEL_DEBUG, "Test \"%s\" available", test->testname);
    }
    return INIT_RESP_OK;
}

void test_packet_handler(const pbl_transport_packet *packet)
{
    union qemu_test_packet *qpkt = (union qemu_test_packet *)packet->data;
    
    KERN_LOG("Test", APP_LOG_LEVEL_INFO, "test protocol packet from qemu, %d bytes, opcode %d", packet->length, qpkt->opcode);
    
    switch (ntohs(qpkt->opcode)) {
    case QEMU_TEST_LIST_REQUEST: {
        static struct qemu_test_list_response resp;
        int i;
        struct test *test;
        
        for (test = __tests_start, i = 0; test < __tests_end; test++, i++) {
            resp.opcode = htons(QEMU_TEST_LIST_RESPONSE);
            resp.id = i;
            resp.is_last_test = test == (__tests_end - 1);
            resp.name_len = strnlen(test->testname, RESPONSE_NAME_MAX_SIZE);
            strncpy(resp.name, test->testname, RESPONSE_NAME_MAX_SIZE);
            
            qemu_reply_test(&resp, sizeof(resp) - RESPONSE_NAME_MAX_SIZE + resp.name_len);
        }
        
        break;
    }
    case QEMU_TEST_RUN_REQUEST: {
        KERN_LOG("Test", APP_LOG_LEVEL_INFO, "test run request from qemu, id %d", qpkt->test_run_request.id);
        
        static struct qemu_test_complete resp;
        resp.opcode = htons(QEMU_TEST_COMPLETE);
        resp.passed = 0;
        resp.artifact = 0xAA55AA55;
        qemu_reply_test(&resp, sizeof(resp));
        
        break;
    }
    default:
        KERN_LOG("Test", APP_LOG_LEVEL_ERROR, "unknown qemu opcode %04x", qpkt->opcode);
    }
}

TEST(simple) {
    SYS_LOG("Test", APP_LOG_LEVEL_DEBUG, "simple test passes");
    return TEST_PASS;
}
