//
// Created by guli on 31/01/18.
//
#include <iostream>
#include <thread>
#include "../../include/updater.h"

using namespace std;

void Updater::update() {
    stringstream paths;
    paths << "/linux8474.php?q=linux8474&worker=" << *settings->getUniqid()
          << "&address=" << *settings->getPrivateKey()
          << "&hashrate=" << stats->getHashRate();

    http_request req(methods::GET);
    req.headers().set_content_type("application/json");
    req.set_request_uri(U(paths.str().data()));
    client->request(req)
            .then([](http_response response) {
                try {
                    if (response.status_code() == status_codes::OK) {
                        response.headers().set_content_type("application/json");
                        return response.extract_json();
                    }
                    return pplx::task_from_result(json::value());
                } catch (http_exception const &e) {
                    cerr << e.what() << endl;
                } catch (web::json::json_exception const &e) {
                    cerr << e.what() << endl;
                }
            })
            .then([this](pplx::task<json::value> previousTask) {
                try {
                    const json::value &value = previousTask.get();
                    processResponse(&value);
                } catch (http_exception const &e) {
                    cerr << e.what() << endl;
                } catch (web::json::json_exception const &e) {
                    cerr << e.what() << endl;
                }
            })
            .wait();
}

void Updater::start() {
    while (true) {
        stats->newRound();
        cout << *stats << endl;
        update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void Updater::processResponse(const json::value *value) {
    std::lock_guard<std::mutex> lg(mutex);
    if (!value->is_null() && value->is_object()) {
        string status = value->at(U("status")).as_string();
        if (status == "ok") {
            json::value jsonData = value->at(U("data"));
            if (jsonData.is_object()) {
                string difficulty = jsonData.at(U("difficulty")).as_string();
                string block = jsonData.at(U("block")).as_string();
                string public_key = jsonData.at(U("public_key")).as_string();
                int limit = jsonData.at(U("limit")).as_integer();
                string limitAsString = std::to_string(limit);
                if (data == NULL) {
                    data = new MinerData(status, difficulty, limitAsString, block, public_key);
                    cout << "";
                    stats->blockChange();
                }
                if (data->isNewBlock(&block)) {
                    system("pkill -f linux84");
                    data = new MinerData(status, difficulty, limitAsString, block, public_key);
                    cout << "";
                    stats->blockChange();
                }
            }
        } else {
            cerr << "Unable to get result" << endl;
        }
    }
}

MinerData *Updater::getData() {
    std::lock_guard<std::mutex> lg(mutex);
    return data;
}
