#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <jansson.h>

#include "Headers/config.h"

const char* host = "identify-ap-southeast-1.acrcloud.com";

char* base64_encode(const unsigned char* input, int length){
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char*)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);

    buff[bptr->length] = 0;

    BIO_free_all(b64);
    return(buff);
}

struct MemoryStruct{
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <audio_file.wav>\n", argv[0]);
        return 1;
    }
    char *file_path = argv[1];

    //PREPARE SIGNATURE
    char http_method[] = "POST";
    char http_uri[] = "/v1/identify";
    char data_type[] = "audio";
    char signature_version[] = "1";
    char timestamp_str[20];
    sprintf(timestamp_str, "%ld", time(NULL));

    char string_to_sign[512];
    sprintf(string_to_sign, "%s\n%s\n%s\n%s\n%s\n%s",
            http_method, http_uri, access_key, data_type, signature_version, timestamp_str);

    // Hash it
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha1(), access_secret, strlen(access_secret),
         (unsigned char*)string_to_sign, strlen(string_to_sign), hmac_result, &hmac_len);

    // Base64 encode the result
    char* signature = base64_encode(hmac_result, hmac_len);
    printf("🔑 Generated Signature: %s\n", signature);

    //HTTP POST REQUEST
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        return 1;
    }
    
    struct stat file_info;
    if (stat(file_path, &file_info) != 0) {
        fprintf(stderr, "Error getting file stats for %s\n", file_path);
        return 1;
    }
    char sample_bytes_str[20];
    sprintf(sample_bytes_str, "%ld", file_info.st_size);

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sample", CURLFORM_FILE, file_path, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "access_key", CURLFORM_COPYCONTENTS, access_key, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sample_bytes", CURLFORM_COPYCONTENTS, sample_bytes_str, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "timestamp", CURLFORM_COPYCONTENTS, timestamp_str, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "signature", CURLFORM_COPYCONTENTS, signature, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "data_type", CURLFORM_COPYCONTENTS, data_type, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "signature_version", CURLFORM_COPYCONTENTS, signature_version, CURLFORM_END);

    // Construct the full URL
    char url[256];
    sprintf(url, "https://%s%s", host, http_uri);

    // Setup the curl request
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    //EXECUTE AND PARSE RESPONSE
    printf("📡 Sending request to ACRCloud...\n");
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("✅ Response Received. Parsing...\n\n");
        
        json_error_t error;
        json_t *root = json_loads(chunk.memory, 0, &error);

        if (!root) {
            fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        } else {
            json_t *status = json_object_get(root, "status");
            const char *msg = json_string_value(json_object_get(status, "msg"));

            if (strcmp(msg, "Success") == 0) {
                json_t *metadata = json_object_get(root, "metadata");
                json_t *music_array = json_object_get(metadata, "music");

                if (json_array_size(music_array) > 0) {
                    json_t *first_result = json_array_get(music_array, 0);
                    const char *title = json_string_value(json_object_get(first_result, "title"));
                    
                    json_t *artists_array = json_object_get(first_result, "artists");
                    json_t *first_artist = json_array_get(artists_array, 0);
                    const char *artist = json_string_value(json_object_get(first_artist, "name"));

                    printf("--- 🎵 Match Found! ---\n");
                    printf("Title:  %s\n", title);
                    printf("Artist: %s\n", artist);
                    printf("------------------------\n");
                } else {
                    printf("--- 🤷 No Match Found ---\n");
                }
            } else {
                printf("--- ❌ Error from API ---\n");
                printf("Message: %s\n", msg);
            }
            json_decref(root);
        }
    }

    free(signature);
    free(chunk.memory);
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    curl_global_cleanup();

    return 0;
}