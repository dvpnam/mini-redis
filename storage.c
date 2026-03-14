/*
 * storage.c -- In-memory key-value store
 *
 * Implements a flat array of KV structs, each holding a key, a value,
 * and an optional expiry timestamp. Expiration follows a lazy strategy:
 * expired entries are removed only when they are accessed, avoiding the
 * overhead of a background sweep.
 *
 * Supported commands:
 *   SET   key value [EX seconds]
 *   GET   key
 *   DEL   key
 *   KEYS
 *   TTL   key
 *   PING
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_STORE          1024
#define KEY_SIZE           64
#define VALUE_SIZE         256
#define KEYS_RESPONSE_SIZE (MAX_STORE * KEY_SIZE)

typedef struct {
    char   key[KEY_SIZE];
    char   value[VALUE_SIZE];
    time_t expires_at; /* Unix timestamp of expiry; 0 means no expiry */
} KV;

static KV  store[MAX_STORE];
static int store_count = 0;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/* Returns 1 if the entry at index i has a TTL and it has expired. */
static int is_expired(int i)
{
    return store[i].expires_at > 0 && time(NULL) >= store[i].expires_at;
}

/*
 * Removes the entry at index i by overwriting it with the last entry.
 * O(1) deletion; does not preserve insertion order.
 */
static void remove_entry(int i)
{
    store[i] = store[store_count - 1];
    store_count--;
}

/* -------------------------------------------------------------------------
 * Command dispatcher
 * ---------------------------------------------------------------------- */

char *execute_command(char *cmd, char *key, char *value)
{
    static char response[VALUE_SIZE];

    /* ------------------------------------------------------------------
     * SET key value [EX seconds]
     *
     * Optional "EX <n>" suffix sets a TTL. The parser passes the full
     * remainder of the input as `value`, e.g. "Hanoi EX 60", so we use
     * strstr to detect and split off the EX clause.
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "SET") == 0) {

        char   actual_value[VALUE_SIZE];
        time_t expires_at = 0; /* default: no expiry */

        char *ex_pos = strstr(value, " EX ");
        if (ex_pos != NULL) {
            /* Copy only the value portion before " EX " */
            int val_len = (int)(ex_pos - value);
            strncpy(actual_value, value, val_len);
            actual_value[val_len] = '\0';

            int ttl = atoi(ex_pos + 4); /* skip the 4-char " EX " prefix */
            if (ttl > 0)
                expires_at = time(NULL) + ttl;
        } else {
            strncpy(actual_value, value, VALUE_SIZE - 1);
            actual_value[VALUE_SIZE - 1] = '\0';
        }

        /* Update in place if the key already exists */
        for (int i = 0; i < store_count; i++) {
            if (strcmp(store[i].key, key) == 0) {
                strncpy(store[i].value, actual_value, VALUE_SIZE - 1);
                store[i].value[VALUE_SIZE - 1] = '\0';
                store[i].expires_at = expires_at;
                return "OK";
            }
        }

        if (store_count >= MAX_STORE)
            return "ERR store is full";

        /* Insert new entry */
        strncpy(store[store_count].key, key, KEY_SIZE - 1);
        store[store_count].key[KEY_SIZE - 1] = '\0';

        strncpy(store[store_count].value, actual_value, VALUE_SIZE - 1);
        store[store_count].value[VALUE_SIZE - 1] = '\0';

        store[store_count].expires_at = expires_at;
        store_count++;
        return "OK";
    }

    /* ------------------------------------------------------------------
     * GET key
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "GET") == 0) {

        for (int i = 0; i < store_count; i++) {
            if (strcmp(store[i].key, key) == 0) {
                if (is_expired(i)) {
                    remove_entry(i); /* lazy expiration */
                    return "NULL";
                }
                return store[i].value;
            }
        }

        return "NULL";
    }

    /* ------------------------------------------------------------------
     * DEL key
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "DEL") == 0) {

        for (int i = 0; i < store_count; i++) {
            if (strcmp(store[i].key, key) == 0) {
                remove_entry(i);
                return "OK";
            }
        }

        return "NULL";
    }

    /* ------------------------------------------------------------------
     * PING
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "PING") == 0)
        return "PONG";

    /* ------------------------------------------------------------------
     * KEYS
     *
     * Returns all non-expired keys, one per line.
     * Expired keys encountered during the scan are removed on the spot
     * (lazy expiration for the full keyspace).
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "KEYS") == 0) {

        static char keys_response[KEYS_RESPONSE_SIZE];
        keys_response[0] = '\0';
        int found = 0;

        for (int i = 0; i < store_count; i++) {
            if (is_expired(i)) {
                remove_entry(i);
                i--; /* re-check this index after the swap */
                continue;
            }

            if (found > 0)
                strncat(keys_response, "\n",
                        KEYS_RESPONSE_SIZE - strlen(keys_response) - 1);

            strncat(keys_response, store[i].key,
                    KEYS_RESPONSE_SIZE - strlen(keys_response) - 1);
            found++;
        }

        return found > 0 ? keys_response : "(empty)";
    }

    /* ------------------------------------------------------------------
     * TTL key
     *
     * Returns the remaining time-to-live in seconds.
     *   >= 0  seconds remaining
     *     -1  key exists but has no expiry
     *     -2  key does not exist or has already expired
     * ------------------------------------------------------------------ */
    if (strcmp(cmd, "TTL") == 0) {

        for (int i = 0; i < store_count; i++) {
            if (strcmp(store[i].key, key) == 0) {
                if (is_expired(i)) {
                    remove_entry(i);
                    return "-2";
                }
                if (store[i].expires_at == 0) {
                    return "-1"; /* persistent key */
                }
                long remaining = (long)(store[i].expires_at - time(NULL));
                snprintf(response, sizeof(response), "%ld", remaining);
                return response;
            }
        }

        return "-2"; /* key not found */
    }

    snprintf(response, sizeof(response), "ERR unknown command '%s'", cmd);
    return response;
}
