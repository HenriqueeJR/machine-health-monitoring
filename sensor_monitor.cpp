#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // json handling
#include "mqtt/client.h" // paho mqtt
#include <iomanip>

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"

int main(int argc, char* argv[]) {
    std::string clientId = "sensor-monitor";
    mqtt::client client(BROKER_ADDRESS, clientId);

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    try {
        client.connect(connOpts);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::clog << "connected to the broker" << std::endl;

    // Get the unique machine identifier, in this case, the hostname.
    char hostname[1024];
    gethostname(hostname, 1024);
    std::string machineId(hostname);

    nlohmann::json j_sensor1;
    j_sensor1["sensor_id"] = "124";
    j_sensor1["data_type"] = "naosei";
    j_sensor1["data_interval"] = "random1";

    nlohmann::json j_sensor2;
    j_sensor2["sensor_id"] = "1242";
    j_sensor2["data_type"] = "naosei2";
    j_sensor2["data_interval"] = "random2";

    nlohmann::json j_inicial;
    j_inicial["machine_id"] = machineId;
    j_inicial["sensors"] = {j_sensor1, j_sensor2};

    std::string topic_inicial = "/sensor_monitors";
    mqtt::message msg_inicial(topic_inicial, j_inicial.dump(), QOS, false);
    //std::clog << "message published - topic: " << topic_inicial << " - message: " << j_inicial.dump() << std::endl;
    client.publish(msg_inicial);

    while (true) {
       // Get the current time in ISO 8601 format.
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        std::stringstream ss;
        ss << std::put_time(now_tm, "%FT%TZ");
        std::string timestamp = ss.str();

        // Generate a random value.
        int value = rand();

        // Construct the JSON message.
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["value"] = value;

        // Publish the JSON message to the appropriate topic.
        std::string topic = "/sensor_monitors/" + machineId + "/rand";
        mqtt::message msg(topic, j.dump(), QOS, false);
        std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl;
        client.publish(msg);

        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(1));




    }

    return EXIT_SUCCESS;
}
