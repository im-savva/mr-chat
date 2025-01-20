#include "encryption.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

RSA* generateRSAKeyPair() {
    RSA *rsa = RSA_new();
    BIGNUM *e = BN_new();
    BN_set_word(e, RSA_F4);
    RSA_generate_key_ex(rsa, 2048, e, nullptr);
    BN_free(e);
    return rsa;
}

AES_KEY* generateAESKey() {
    AES_KEY *aesKey = (AES_KEY *)malloc(sizeof(AES_KEY));
    unsigned char key[AES_BLOCK_SIZE];
    RAND_bytes(key, AES_BLOCK_SIZE);
    AES_set_encrypt_key(key, 256, aesKey);
    return aesKey;
}

const unsigned char fixedSalt[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

AES_KEY* generateAESKeyFromPassword(const QString &password) {
    const EVP_MD *digest = EVP_sha256(); // Using SHA-256 as the hash function
    const int iterations = 10000; // Number of iterations for PBKDF2
    const int keyLength = 256; // AES key length in bits

    AES_KEY *aesKey = (AES_KEY *)malloc(sizeof(AES_KEY));
    unsigned char key[keyLength / 8];
    unsigned char iv[AES_BLOCK_SIZE];

    PKCS5_PBKDF2_HMAC((const char *)password.toUtf8().constData(), -1, fixedSalt, sizeof(fixedSalt), iterations, digest, keyLength / 8, key);
    PKCS5_PBKDF2_HMAC((const char *)password.toUtf8().constData(), -1, fixedSalt, sizeof(fixedSalt), iterations, digest, AES_BLOCK_SIZE, iv);

    AES_set_encrypt_key(key, keyLength, aesKey);
    AES_cbc_encrypt(iv, iv, AES_BLOCK_SIZE, aesKey, iv, AES_ENCRYPT);

    return aesKey;
}

unsigned char* rsaEncryptAESKey(RSA *rsa, const unsigned char *aesKey, int aesKeyLength, int *encryptedKeyLength) {
    unsigned char *encryptedKey = (unsigned char *)malloc(RSA_size(rsa));
    *encryptedKeyLength = RSA_public_encrypt(aesKeyLength, aesKey, encryptedKey, rsa, RSA_PKCS1_PADDING);
    return encryptedKey;
}

unsigned char* rsaDecryptAESKey(RSA *rsa, const unsigned char *encryptedKey, int encryptedKeyLength, int *decryptedKeyLength) {
    unsigned char *decryptedKey = (unsigned char *)malloc(RSA_size(rsa));
    *decryptedKeyLength = RSA_private_decrypt(encryptedKeyLength, encryptedKey, decryptedKey, rsa, RSA_PKCS1_PADDING);
    return decryptedKey;
}

QByteArray encryptTextAES(const unsigned char *plaintext, int plaintextLength, const unsigned char *aesKey, unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        // Error handling
        return QByteArray();
    }

    int ciphertextLen = plaintextLength + AES_BLOCK_SIZE;
    unsigned char *ciphertext = (unsigned char *)malloc(ciphertextLen); // Allocate memory for ciphertext

    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int len = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aesKey, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintextLength);
    ciphertextLen = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    QByteArray result(reinterpret_cast<const char *>(ciphertext), ciphertextLen);

    // Free the dynamically allocated memory
    free(ciphertext);

    return result;
}

QByteArray decryptTextAES(const unsigned char *ciphertext, int ciphertextLength, const unsigned char *aesKey, unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        // Error handling
        return QByteArray();
    }

    int plaintextLen = 0; // Initialize plaintext length to 0
    unsigned char *plaintext = (unsigned char *)malloc(ciphertextLength + EVP_MAX_BLOCK_LENGTH); // Allocate memory for plaintext

    if (!plaintext) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int len = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aesKey, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextLength);
    plaintextLen = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    QByteArray result(reinterpret_cast<const char *>(plaintext), plaintextLen);

    // Free the dynamically allocated memory
    free(plaintext);

    return result;
}

QString rsaPublicKeyToString(RSA *rsaKey) {
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSA_PUBKEY(bio, rsaKey);

    char *ptr;
    long length = BIO_get_mem_data(bio, &ptr);
    QString publicKeyString = QString::fromLatin1(ptr, length);

    BIO_free(bio);

    return publicKeyString;
}

RSA* rsaPublicKeyFromString(const QString &publicKeyString) {
    QByteArray byteArray = publicKeyString.toLatin1();
    const char *ptr = byteArray.constData();
    BIO *bio = BIO_new_mem_buf(ptr, byteArray.size());
    RSA *rsaKey = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return rsaKey;
}

QString aesKeyToString(const AES_KEY *aesKey) {
    const unsigned char *keyData = (const unsigned char *)aesKey;
    return QString::fromLatin1(reinterpret_cast<const char *>(keyData), sizeof(AES_KEY));
}

AES_KEY* aesKeyFromString(const QString &aesKeyString) {
    AES_KEY *aesKey = (AES_KEY *)malloc(sizeof(AES_KEY));
    if (aesKeyString.length() == sizeof(AES_KEY)) {
        memcpy(aesKey, aesKeyString.toLatin1().constData(), sizeof(AES_KEY));
        return aesKey;
    } else {
        free(aesKey);
        return nullptr;
    }
}

QByteArray generateRandomIV() {
    const int ivSize = AES_BLOCK_SIZE;
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, AES_BLOCK_SIZE);
    return QByteArray(reinterpret_cast<const char*>(iv), ivSize);
}

QByteArray encryptQStringAES(const QString &plaintext, const AES_KEY *aesKey, unsigned char *iv) {
    QByteArray plaintextBytes = plaintext.toUtf8();
    return encryptTextAES(reinterpret_cast<const unsigned char *>(plaintextBytes.constData()), plaintextBytes.size(), (const unsigned char *)aesKey, iv);
}

QString decryptQStringAES(const QByteArray &ciphertext, const AES_KEY *aesKey, unsigned char *iv) {
    return QString::fromUtf8(decryptTextAES(reinterpret_cast<const unsigned char *>(ciphertext.constData()), ciphertext.size(), (const unsigned char *)aesKey, iv));
}
