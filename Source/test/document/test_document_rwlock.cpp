/*
** @file test_document_rwlock.cpp
** @author XueShuming
** @date 2025/08/10
** @brief The program to test document read-write lock APIs.
**
** Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <purc/purc-document.h>
#include <purc/purc-ports.h>
#include "private/document.h"
#include "private/debug.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <gtest/gtest.h>

static const char *html_contents = ""
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
""
"<html lang=\"en\">"
"<head id=\"foo\">"
"<title>Test Document</title>"
"</head>"
""
"<body id=\"bar\">"
"<div>Test Content</div>"
"</body>"
"</html>"
;

// Test basic lock initialization
TEST(document_rwlock, init) {
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Explicitly initialize the rwlock
    int ret = pcdoc_document_lock_init(doc);
    ASSERT_EQ(ret, 0);

    // Test locking and unlocking
    ret = pcdoc_document_lock_for_read(doc);
    ASSERT_EQ(ret, 0);

    ret = pcdoc_document_unlock(doc);
    ASSERT_EQ(ret, 0);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

// Test read lock
TEST(document_rwlock, read_lock) {
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Explicitly initialize the rwlock
    int ret = pcdoc_document_lock_init(doc);
    ASSERT_EQ(ret, 0);

    // Multiple read locks should succeed
    ret = pcdoc_document_lock_for_read(doc);
    ASSERT_EQ(ret, 0);

    ret = pcdoc_document_lock_for_read(doc);
    ASSERT_EQ(ret, 0);

    // Unlock twice
    ret = pcdoc_document_unlock(doc);
    ASSERT_EQ(ret, 0);

    ret = pcdoc_document_unlock(doc);
    ASSERT_EQ(ret, 0);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

// Test write lock
TEST(document_rwlock, write_lock) {
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Explicitly initialize the rwlock
    int ret = pcdoc_document_lock_init(doc);
    ASSERT_EQ(ret, 0);

    // Write lock should succeed
    ret = pcdoc_document_lock_for_write(doc);
    ASSERT_EQ(ret, 0);

    // Unlock
    ret = pcdoc_document_unlock(doc);
    ASSERT_EQ(ret, 0);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

// Test NULL document handling
TEST(document_rwlock, null_document) {
    // All lock functions should return -1 for NULL document
    int ret = pcdoc_document_lock_init(NULL);
    ASSERT_EQ(ret, -1);

    ret = pcdoc_document_lock_for_read(NULL);
    ASSERT_EQ(ret, -1);

    ret = pcdoc_document_lock_for_write(NULL);
    ASSERT_EQ(ret, -1);

    ret = pcdoc_document_unlock(NULL);
    ASSERT_EQ(ret, -1);
}

// Structure to pass data to threads
struct thread_data {
    purc_document_t doc;
    int thread_id;
    int iterations;
    bool use_write_lock;
};

// Thread function for read lock test
void* read_lock_thread(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;

    for (int i = 0; i < data->iterations; i++) {
        PC_WARN("Thread %d: waiting for read lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
        int ret = pcdoc_document_lock_for_read(data->doc);
        EXPECT_EQ(ret, 0);

        PC_WARN("Thread %d: acquired read lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);

        // Simulate some read operation
        usleep(1000); // 1ms
        PC_WARN("Thread %d: reading document (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);

        PC_WARN("Thread %d: releasing read lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
        ret = pcdoc_document_unlock(data->doc);
        EXPECT_EQ(ret, 0);
        PC_WARN("Thread %d: released read lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
    }

    return NULL;
}

// Thread function for write lock test
void* write_lock_thread(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;

    for (int i = 0; i < data->iterations; i++) {
        PC_WARN("Thread %d: waiting for write lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
        int ret = pcdoc_document_lock_for_write(data->doc);
        EXPECT_EQ(ret, 0);

        PC_WARN("Thread %d: acquired write lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);

        // Simulate some write operation
        usleep(2000); // 2ms
        PC_WARN("Thread %d: writing to document (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);

        PC_WARN("Thread %d: releasing write lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
        ret = pcdoc_document_unlock(data->doc);
        EXPECT_EQ(ret, 0);
        PC_WARN("Thread %d: released write lock (iteration %d/%d)\n",
                data->thread_id, i+1, data->iterations);
    }

    return NULL;
}

// Test multiple readers
TEST(document_rwlock, multiple_readers) {
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Explicitly initialize the rwlock
    int ret = pcdoc_document_lock_init(doc);
    ASSERT_EQ(ret, 0);

    const int num_threads = 4;
    const int iterations = 100;

    pthread_t threads[num_threads];
    struct thread_data thread_data_array[num_threads];

    // Create reader threads
    for (int i = 0; i < num_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = iterations;
        thread_data_array[i].use_write_lock = false;

        int ret = pthread_create(&threads[i], NULL, read_lock_thread, &thread_data_array[i]);
        ASSERT_EQ(ret, 0);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

// Test readers and writers
TEST(document_rwlock, readers_and_writers) {
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Explicitly initialize the rwlock
    int ret = pcdoc_document_lock_init(doc);
    ASSERT_EQ(ret, 0);

    const int num_threads = 6;
    const int iterations = 50;

    pthread_t threads[num_threads];
    struct thread_data thread_data_array[num_threads];

    // Create 4 reader threads and 2 writer threads
    for (int i = 0; i < num_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = iterations;
        thread_data_array[i].use_write_lock = (i >= 4); // Last 2 threads are writers

        int ret;
        if (thread_data_array[i].use_write_lock) {
            ret = pthread_create(&threads[i], NULL, write_lock_thread, &thread_data_array[i]);
        } else {
            ret = pthread_create(&threads[i], NULL, read_lock_thread, &thread_data_array[i]);
        }
        ASSERT_EQ(ret, 0);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}
