//
// Created by guli on 01/02/18.
//
#define DD "4hDFRqgFDTjy5okh2A7JwQ3MZM7fGyaqzSZPEKUdgwSM8sKLPEgs8Awpdgo3R54uo1kGMnxujQQpF94qV6SxEjRL"

#include <iomanip>
#include <openssl/sha.h>
#include <thread>

#include "../../include/linux84.h"

using namespace std;

static int b64_byte_to_char(unsigned x) {
    return (LT(x, 26) & (x + 'A')) |
           (GE(x, 26) & LT(x, 52) & (x + ('a' - 26))) |
           (GE(x, 52) & LT(x, 62) & (x + ('0' - 52))) | (EQ(x, 62) & '+') |
           (EQ(x, 63) & '/');
}

void Miner::mine() {
    while (true) {
        if (data == nullptr || data->isNewBlock(updater->getData()->getBlock())) {
            data = updater->getData();
            limit.set_str(*data->getLimit(), 10);
            diff.set_str(*data->getDifficulty(), 10);
        }
        nonces.clear();
        bases.clear();
        argons.clear();

        buildBatch();

        computeHash();

        for (int j = 0; j < *settings->getBatchSize(); ++j) {
            checkArgon(&bases[j], &argons[j], &nonces[j]);
        }
        stats->addHashes(*settings->getBatchSize());
    }
}

void Miner::to_base64(char *dst, size_t dst_len, const void *src, size_t src_len) {
    size_t olen;
    const unsigned char *buf;
    unsigned acc, acc_len;

    olen = (src_len / 3) << 2;
    switch (src_len % 3) {
        case 2:
            olen++;
            /* fall through */
        case 1:
            olen += 2;
            break;
    }
    if (dst_len <= olen) {
        cout << "SHORTTTTTTTTTTTTTTTTT" << endl;
        return;
    }
    acc = 0;
    acc_len = 0;
    buf = (const unsigned char *) src;
    while (src_len-- > 0) {
        acc = (acc << 8) + (*buf++);
        acc_len += 8;
        while (acc_len >= 6) {
            acc_len -= 6;
            *dst++ = (char) b64_byte_to_char((acc >> acc_len) & 0x3F);
        }
    }
    if (acc_len > 0) {
        *dst++ = (char) b64_byte_to_char((acc << (6 - acc_len)) & 0x3F);
    }
    *dst++ = 0;
}


void Miner::generateBytes(char *dst, size_t dst_len, uint8_t *buffer, size_t buffer_size) {
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = distribution(generator);
    }
    to_base64(dst, dst_len, buffer, buffer_size);
}

void Miner::buildBatch() {
    for (int j = 0; j < *settings->getBatchSize(); ++j) {
        generateBytes(nonceBase64, 64, byteBuffer, 32);
        std::string nonce(nonceBase64);

        boost::replace_all(nonce, "/", "");
        boost::replace_all(nonce, "+", "");

        std::stringstream ss;
        ss << *data->getPublic_key() << "-" << nonce << "-" << *data->getBlock() << "-" << *data->getDifficulty();
        //cout << ss.str() << endl;
        std::string base = ss.str();

        nonces.push_back(nonce);
        bases.push_back(base);
    }
}


void Miner::checkArgon(string *base, string *argon, string *nonce) {

    std::stringstream oss;
    oss << *base << *argon;

    auto sha = SHA512((const unsigned char *) oss.str().c_str(), strlen(oss.str().c_str()), nullptr);
    sha = SHA512(sha, 64, nullptr);
    sha = SHA512(sha, 64, nullptr);
    sha = SHA512(sha, 64, nullptr);
    sha = SHA512(sha, 64, nullptr);
    sha = SHA512(sha, 64, nullptr);

    stringstream x;
    x << std::hex;
    x << std::dec << (int) sha[10];
    x << std::dec << (int) sha[15];
    x << std::dec << (int) sha[20];
    x << std::dec << (int) sha[23];
    x << std::dec << (int) sha[31];
    x << std::dec << (int) sha[40];
    x << std::dec << (int) sha[45];
    x << std::dec << (int) sha[55];
    string duration = x.str();

    duration.erase(0, min(duration.find_first_not_of('0'), duration.size() - 1));

    result.set_str(duration, 10);
    mpz_tdiv_q(rest.get_mpz_t(), result.get_mpz_t(), diff.get_mpz_t());
    if (mpz_cmp(rest.get_mpz_t(), ZERO.get_mpz_t()) > 0 && mpz_cmp(rest.get_mpz_t(), limit.get_mpz_t()) <= 0) {
        bool d = mpz_cmp(rest.get_mpz_t(), BLOCK_LIMIT.get_mpz_t()) < 0 ? stats->newBlock() : stats->newShare();
        gmp_printf("Submitting - %Zd - %s - %s\n", rest.get_mpz_t(), nonce->data(), argon->data());
        submit(argon, nonce, d);
    }
    long si = rest.get_si();
    stats->newDl(si);
    x.clear();
}

void Miner::submit(string *argon, string *nonce, bool d) {
    string argonTail = argon->substr(29);
    stringstream body;
    boost::replace_all(*nonce, "+", "%2B");
    boost::replace_all(argonTail, "+", "%2B");
    boost::replace_all(*nonce, "$", "%24");
    boost::replace_all(argonTail, "$", "%24");
    boost::replace_all(*nonce, "/", "%2F");
    boost::replace_all(argonTail, "/", "%2F");
    body << "linux1=" << *settings->getPrivateKey()
         << "&linux2=" << argonTail
         << "&linux3=" << *nonce
         << "&linux4=" << *settings->getPrivateKey()
         << "&linux5=" << *data->getPublic_key();
    http_request req(methods::POST);
    req.set_request_uri(U("/linux8474.php?q=linux84"));
    req.set_body(body.str(), "application/x-www-form-urlencoded");
    client->request(req)
            .then([this](http_response response) {
                try {
                    if (response.status_code() == status_codes::OK) {
                        response.headers().set_content_type("application/json");
                        return response.extract_json();
                    }
                } catch (http_exception const &e) {
                    cout << e.what() << endl;
                } catch (web::json::json_exception const &e) {
                    cerr << e.what() << endl;
                }
                return pplx::task_from_result(json::value());
            })
            .then([this](pplx::task<json::value> previousTask) {
                try {
                    json::value jvalue = previousTask.get();
                    if (!jvalue.is_null() && jvalue.is_object()) {
                        string status = jvalue.at(U("status")).as_string();
                        if (status == "ok") {
                            cout << "nonce accepted by pool !!!!!" << endl;
                        } else {
                            cout << "nonce refused by pool :(:(:(" << endl;
                            cout << jvalue << endl;
                            stats->newRejection();
                        }
                    }
                } catch (http_exception const &e) {
                    cout << e.what() << endl;
                } catch (web::json::json_exception const &e) {
                    cerr << e.what() << endl;
                }
            })
            .wait();
}

char *Miner::encode(void *res, size_t reslen) {
    std::stringstream ss;
    ss << "$argon2i";
    ss << "$v=";
    ss << version;
    ss << "$m=";
    ss << params->getMemoryCost();
    ss << ",t=";
    ss << params->getTimeCost();
    ss << ",p=";
    ss << params->getLanes();
    ss << "$";
    auto salt = new char[32];
    to_base64(salt, 32, params->getSalt(), params->getSaltLength());
    ss << salt;
    auto hash = new char[64];
    to_base64(hash, 64, res, reslen);
    ss << "$";
    ss << hash;


    std::string str = ss.str();
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    free(salt);
    free(hash);
    return cstr;
}

