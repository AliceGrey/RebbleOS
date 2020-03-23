/* blob_db_test.c
 * tests for blob database
 * RebbleOS
 */
#include "rebbleos.h"
#include "protocol_system.h"
#include "pebble_protocol.h"
#include "node_list.h"
#include "timeline.h"
#include "blob_db.h"
#include "test.h"
#include "debug.h"

#define MODULE_NAME "blobdb"
#define MODULE_TYPE "TEST"
#define LOG_LEVEL RBL_LOG_LEVEL_DEBUG //RBL_LOG_LEVEL_ERROR

#include <string.h>

#ifdef REBBLEOS_TESTING

static int _insert(int key, int dsize) {
    const struct blobdb_database *db = blobdb_open(BlobDatabaseID_Test);
    
    uint8_t *val = app_calloc(1, dsize);
    for (int i = 0; i < dsize; i++)
        val[i] = (key & 0xFF) ^ i;
    
    int rv = blobdb_insert(db, (void *)&key, 4, val, dsize);
    
    app_free(val);
    
    return rv != Blob_Success;
}

static int _retrieve(int key, int dsize) {
    const struct blobdb_database *db = blobdb_open(BlobDatabaseID_Test);
    struct blobdb_iter it;
    blobdb_select_result_list head;
    
    list_init_head(&head);
    
    int rv = blobdb_iter_start(db, &it);
    if (!rv)
        return 1;
    
    struct blobdb_selector selectors[] = {
        { BLOBDB_SELECTOR_OFFSET_KEY, 4, Blob_Eq, &key },
        { 0, 0, Blob_Result_FullyLoad },
        { }
    };
    int n = blobdb_select(&it, &head, selectors);
    if (n != 1)
        return 2;
    
    struct blobdb_select_result *res = blobdb_select_result_head(&head);
    for (int i = 0; i < dsize; i++) {
        uint8_t exp = (key & 0xFF) ^ i;
        uint8_t rd = ((uint8_t *)res->result[0])[i];
        if (exp != rd) {
            LOG_ERROR("blobdb_select incorrect readback key %d size %d ofs %d, should be %02x is %02x", key, dsize, i, exp, rd);
            return 3;
        }
    }
    
    blobdb_select_free_all(&head);
    
    return 0; /* xxx check data */
}

TEST(blobdb_basic) {
    LOG_INFO("blobdb_select(1) should fail");
    if (_retrieve(1, 16) == 0) {
        LOG_ERROR("blobdb_select(1) init state returned data");
        return TEST_FAIL;
    }
    
    if (_insert(1, 16) != 0) {
        LOG_ERROR("blobdb_insert(1) failed");
        return TEST_FAIL;
    }
    
    if (_insert(1, 16) == 0) {
        LOG_ERROR("double blobdb_insert(1) succeeded");
        return TEST_FAIL;
    }

    if (_retrieve(1, 16) != 0) {
        LOG_ERROR("blobdb_retrieve(1) failed");
        return TEST_FAIL;
    }

    *artifact = 0;
    return TEST_PASS;
}

#endif
