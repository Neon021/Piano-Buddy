#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "piano_buddy.h"

static size_t write_data_to_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/**
 * Downloads a file from a URL and saves it to a local path.
 * Returns 0 on success, 1 on failure.
 */
int download_file(const char *url, const char *output_filename) {
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Could not initialize curl.\n");
        return 1;
    }

    fp = fopen(output_filename, "wb"); // Open file for writing in binary
    if (!fp) {
        fprintf(stderr, "Error: Could not open file for writing: %s\n", output_filename);
        curl_easy_cleanup(curl);
        return 1;
    }

    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Set the write function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_file);
    // Set the file pointer to write to
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    // Follow redirects (important for bitmidi.com)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // Perform the request
    res = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        // Try to delete the empty/corrupt file
        remove(output_filename);
        return 1;
    }

    return 0;
}