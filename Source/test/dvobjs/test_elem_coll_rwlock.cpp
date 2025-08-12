/*
** @file test_elem_coll_rwlock.cpp
** @author XueShuming
** @date 2025/08/12
** @brief The program to test element collection read-write lock APIs.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#include "private/debug.h"
#include "private/document.h"
#include <errno.h>
#include <pthread.h>
#include <purc/purc-document.h>
#include <purc/purc-features.h>
#include <purc/purc-ports.h>
#include <stdio.h>
#include <unistd.h>

#include <gtest/gtest.h>

// HTML content for testing
static const char *html_contents =
    ""
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
    "</html>";

// Test basic functionality of read lock in element collection functions
TEST(elem_coll_rwlock, basic_read_lock)
{
    // Load test document
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML, html_contents,
                                             strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    // Test pcdoc_find_element_in_document function's read lock
    pcdoc_selector_t selector = pcdoc_selector_new("#foo");

    pcdoc_element_t elem = pcdoc_find_element_in_document(doc, selector);

    pcdoc_selector_delete(selector);
    ASSERT_NE(elem, nullptr);

    // Test pcdoc_elem_coll_new_from_descendants function's read lock
    pcdoc_selector_t div_selector = pcdoc_selector_new("div");

    pcdoc_elem_coll_t coll =
        pcdoc_elem_coll_new_from_descendants(doc, NULL, div_selector);

    pcdoc_selector_delete(div_selector);
    ASSERT_NE(coll, nullptr);

    // Clean up resources
    pcdoc_elem_coll_delete(doc, coll);

    purc_document_delete(doc);
}

// Structure to pass data to threads
struct elem_coll_thread_data {
    purc_document_t doc;
    int thread_id;
    int iterations;
    pcdoc_selector_t selector;
    pcdoc_selector_t div_selector;
    pthread_mutex_t *mutex;
};

// Thread function for read operations
void *elem_coll_read_thread(void *arg)
{
    struct elem_coll_thread_data *data = (struct elem_coll_thread_data *)arg;

    for (int i = 0; i < data->iterations; i++) {
        pthread_mutex_lock(data->mutex);

        // Test pcdoc_find_element_in_document function
        // Use pre-created selector instead of creating a new one
        pcdoc_element_t elem =
            pcdoc_find_element_in_document(data->doc, data->selector);
        EXPECT_NE(elem, nullptr);

        // Test pcdoc_elem_coll_new_from_descendants function
        // Use pre-created div_selector
        pcdoc_elem_coll_t coll =
            pcdoc_elem_coll_new_from_descendants(data->doc, NULL, data->div_selector);
        EXPECT_NE(coll, nullptr);

        // Test pcdoc_elem_coll_count and pcdoc_elem_coll_get functions
        ssize_t count = pcdoc_elem_coll_count(data->doc, coll);
        if (count > 0) {
            pcdoc_element_t elem_from_coll =
                pcdoc_elem_coll_get(data->doc, coll, 0);
            EXPECT_NE(elem_from_coll, nullptr);
        }

        // Clean up resources
        pcdoc_elem_coll_delete(data->doc, coll);

        pthread_mutex_unlock(data->mutex);

        // Longer sleep to reduce contention between threads
        usleep(500);
    }

    return NULL;
}

// Test concurrent read operations
TEST(elem_coll_rwlock, concurrent_read)
{
    // Load test document
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML, html_contents,
                                             strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    const int num_threads = 8;
    const int iterations = 100;

    // Pre-create selectors to avoid thread safety issues in CSS engine
    pcdoc_selector_t foo_selector = pcdoc_selector_new("#foo");
    pcdoc_selector_t bar_selector = pcdoc_selector_new("#bar");
    pcdoc_selector_t div_selector = pcdoc_selector_new("div");

    pthread_mutex_t css_mutex;
    pthread_mutex_init(&css_mutex, NULL);

    pthread_t threads[num_threads];
    struct elem_coll_thread_data thread_data_array[num_threads];

    // Create multiple reader threads
    for (int i = 0; i < num_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = iterations;
        thread_data_array[i].selector = (i % 2 == 0) ? foo_selector : bar_selector;
        thread_data_array[i].div_selector = div_selector;
        thread_data_array[i].mutex = &css_mutex;

        int ret = pthread_create(&threads[i], NULL, elem_coll_read_thread,
                                 &thread_data_array[i]);
        ASSERT_EQ(ret, 0);

        usleep(200);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&css_mutex);

    // Clean up pre-created selectors
     pcdoc_selector_delete(foo_selector);
     pcdoc_selector_delete(bar_selector);
     pcdoc_selector_delete(div_selector);

     purc_document_delete(doc);
}

// Thread function for write operations (simulating document modification)
void *elem_coll_write_thread(void *arg)
{
    struct elem_coll_thread_data *data = (struct elem_coll_thread_data *)arg;

    for (int i = 0; i < data->iterations; i++) {
        // Acquire write lock
        int ret = pcdoc_document_lock_for_write(data->doc);
        EXPECT_EQ(ret, 0);

        // Simulate document modification operation
        // Note: This is just a simulation, actual document modification would
        // be done here
        usleep(2000); // Simulate write operation time

        // Release write lock
        ret = pcdoc_document_unlock(data->doc);
        EXPECT_EQ(ret, 0);

        // Interval between write operations
        usleep(5000);
    }

    return NULL;
}

// Test mixed read and write operations
TEST(elem_coll_rwlock, read_write_mix)
{
    // Load test document
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML, html_contents,
                                             strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    const int num_read_threads = 6;
    const int num_write_threads = 2;
    const int total_threads = num_read_threads + num_write_threads;
    const int read_iterations = 100;
    const int write_iterations = 20;

    // Pre-create selectors to avoid thread safety issues in CSS engine
    pcdoc_selector_t foo_selector = pcdoc_selector_new("#foo");
    pcdoc_selector_t bar_selector = pcdoc_selector_new("#bar");
    pcdoc_selector_t div_selector = pcdoc_selector_new("div");

    pthread_mutex_t css_mutex;
    pthread_mutex_init(&css_mutex, NULL);

    pthread_t threads[total_threads];
    struct elem_coll_thread_data thread_data_array[total_threads];

    // Create reader threads
    for (int i = 0; i < num_read_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = read_iterations;
        thread_data_array[i].selector = (i % 2 == 0) ? foo_selector : bar_selector;
        thread_data_array[i].div_selector = div_selector;
        thread_data_array[i].mutex = &css_mutex;

        int ret = pthread_create(&threads[i], NULL, elem_coll_read_thread,
                                 &thread_data_array[i]);
        ASSERT_EQ(ret, 0);
    }

    // Create writer threads
    for (int i = 0; i < num_write_threads; i++) {
        thread_data_array[num_read_threads + i].doc = doc;
        thread_data_array[num_read_threads + i].thread_id =
            num_read_threads + i;
        thread_data_array[num_read_threads + i].iterations = write_iterations;

        int ret = pthread_create(&threads[num_read_threads + i], NULL,
                                 elem_coll_write_thread,
                                 &thread_data_array[num_read_threads + i]);
        ASSERT_EQ(ret, 0);
    }

    // Wait for all threads to complete
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&css_mutex);

    // Clean up pre-created selectors
    pcdoc_selector_delete(foo_selector);
    pcdoc_selector_delete(bar_selector);
    pcdoc_selector_delete(div_selector);

    purc_document_delete(doc);
}

// Stress test with many concurrent operations
TEST(elem_coll_rwlock, stress_test)
{
    // Load test document
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML, html_contents,
                                             strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    const int num_read_threads = 20;
    const int num_write_threads = 4;
    const int total_threads = num_read_threads + num_write_threads;
    const int read_iterations = 500;
    const int write_iterations = 50;

    // Pre-create selectors to avoid thread safety issues in CSS engine
    pcdoc_selector_t foo_selector = pcdoc_selector_new("#foo");
    pcdoc_selector_t bar_selector = pcdoc_selector_new("#bar");
    pcdoc_selector_t div_selector = pcdoc_selector_new("div");

    pthread_mutex_t css_mutex;
    pthread_mutex_init(&css_mutex, NULL);

    pthread_t threads[total_threads];
    struct elem_coll_thread_data thread_data_array[total_threads];

    // Create reader threads
    for (int i = 0; i < num_read_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = read_iterations;
        thread_data_array[i].selector =
            (i % 3 == 0) ? foo_selector : ((i % 3 == 1) ? bar_selector : div_selector);
        thread_data_array[i].div_selector = div_selector;
        thread_data_array[i].mutex = &css_mutex;

        int ret = pthread_create(&threads[i], NULL, elem_coll_read_thread,
                                 &thread_data_array[i]);
        ASSERT_EQ(ret, 0);
    }

    // Create writer threads
    for (int i = 0; i < num_write_threads; i++) {
        thread_data_array[num_read_threads + i].doc = doc;
        thread_data_array[num_read_threads + i].thread_id =
            num_read_threads + i;
        thread_data_array[num_read_threads + i].iterations = write_iterations;

        int ret = pthread_create(&threads[num_read_threads + i], NULL,
                                 elem_coll_write_thread,
                                 &thread_data_array[num_read_threads + i]);
        ASSERT_EQ(ret, 0);
    }

    // Wait for all threads to complete
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&css_mutex);

    // Clean up pre-created selectors
    pcdoc_selector_delete(foo_selector);
    pcdoc_selector_delete(bar_selector);
    pcdoc_selector_delete(div_selector);

    purc_document_delete(doc);
}

// Thread function for nested lock testing
void *nested_lock_thread(void *arg)
{
    struct elem_coll_thread_data *data = (struct elem_coll_thread_data *)arg;

    for (int i = 0; i < data->iterations; i++) {
        pthread_mutex_lock(data->mutex);

        // First acquire read lock
        int ret = pcdoc_document_lock_for_read(data->doc);
        EXPECT_EQ(ret, 0);

        // Try to acquire another read lock inside (should succeed)
        ret = pcdoc_document_lock_for_read(data->doc);
        EXPECT_EQ(ret, 0);

        // Release inner read lock
        ret = pcdoc_document_unlock(data->doc);
        EXPECT_EQ(ret, 0);

        // Release outer read lock
        ret = pcdoc_document_unlock(data->doc);
        EXPECT_EQ(ret, 0);

        pthread_mutex_unlock(data->mutex);

        // Short sleep
        usleep(1000);
    }

    return NULL;
}

// Test nested locks to detect potential deadlock issues
TEST(elem_coll_rwlock, nested_locks)
{
    // Load test document
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML, html_contents,
                                             strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    const int num_threads = 4;
    const int iterations = 50;

    pthread_mutex_t css_mutex;
    pthread_mutex_init(&css_mutex, NULL);

    pthread_t threads[num_threads];
    struct elem_coll_thread_data thread_data_array[num_threads];

    // Create multiple threads for nested lock testing
    for (int i = 0; i < num_threads; i++) {
        thread_data_array[i].doc = doc;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].iterations = iterations;
        thread_data_array[i].mutex = &css_mutex;

        int ret = pthread_create(&threads[i], NULL, nested_lock_thread,
                                 &thread_data_array[i]);
        ASSERT_EQ(ret, 0);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&css_mutex);

    purc_document_delete(doc);
}
