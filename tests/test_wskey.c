#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>


void make_ws_accept(const char *client_key, char *accept_key) {
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    char buf[256];
    unsigned char sha1_result[SHA_DIGEST_LENGTH];

    // 1. 拼接 key + GUID
    snprintf(buf, sizeof(buf), "%s%s", client_key, GUID);

    // 2. SHA1
    SHA1((unsigned char *)buf, strlen(buf), sha1_result);

    // 3. Base64
    EVP_EncodeBlock(
        (unsigned char *)accept_key,
        sha1_result,
        SHA_DIGEST_LENGTH
    );
}

int main(int argc, char **argv) {
    char accept_key[64];
    make_ws_accept(argv[1], accept_key);

    printf("Sec-WebSocket-Accept: %s\n", accept_key);
    return 0;

}
