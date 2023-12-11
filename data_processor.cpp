#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "json.hpp" 
#include "mqtt/client.h" 

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define GRAPHITE_HOST "127.0.0.1"
#define GRAPHITE_PORT 2003

std::mutex m;
std::mutex m2;

void post_metric(const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {

    
    std::string path = machine_id + '.' + sensor_id;
    std::string metric = path + " " + std::to_string(value) + " " + timestamp_str;

    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Erro ao criar o socket" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(GRAPHITE_PORT);
    inet_pton(AF_INET, GRAPHITE_HOST, &(serv_addr.sin_addr));

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Erro ao conectar ao servidor Graphite" << std::endl;
        close(sockfd);
        return;
    }
    std::string string_teste = "test_no_codigo.teste 20 20";

    if (send(sockfd, string_teste.c_str(), string_teste.length(), 0) < 0) {
        std::cerr << "Erro ao enviar dados para o servidor Graphite" << std::endl;
    }

    close(sockfd);

}


std::vector<std::string> split(const std::string &str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string clientId = "clientId";
mqtt::async_client client(BROKER_ADDRESS, clientId);

//Guarda o valor do timestamp da ultima mensagem recebida de cada sensor
//encadeado como: {id_do_sensor: ultimo_timestamp}
std::map<std::string, std::string> actual_timestamps;

void monitor_sensor_inactivity(std::string sensorId, int data_interval) {
    bool data_received = true;
    int count = 0; // Contador de tempo sem dados


    while (true) {
        auto last_timestamp = actual_timestamps[sensorId]; // Obtém o timestamp inicial

        std::this_thread::sleep_for(std::chrono::milliseconds(data_interval)); // Aguarda 1 intervalo

        m.lock(); // Bloqueia o mutex para acessar actual_timestamps
        auto current_timestamp = actual_timestamps[sensorId];
        m.unlock(); // Libera o mutex

        if (current_timestamp == last_timestamp) {
            data_received = false;
            count++;

            if (count == 10) {
                std::cout << "Não houve recebimento de dados do "<<sensorId << " por 10 intervalos!" << std::endl;
                
                count = 0;
            }
        } else {
            data_received = true;
            count = 0;
        }


        if (data_received) {
            std::cout << "Dados do sensor " << sensorId << " estão sendo recebidos." << std::endl;
        }
    }
}



int main(int argc, char* argv[]) {

    

    // Create an MQTT callback.
    class callback : public virtual mqtt::callback {
    public:

        
        void message_arrived(mqtt::const_message_ptr msg) override {
            
            auto j = nlohmann::json::parse(msg->get_payload());
            


            //std::cout << "topico: " << msg->get_topic() << "    payload: " << msg->get_payload() << std::endl;

            

            if (msg->get_topic() == "/sensor_monitors") {
                
                
                std::string new_machine_id = j["machine_id"];
                
                std::string new_sensor1_id = j["sensors"][0]["sensor_id"];
                std::string new_sensor1_data_type = j["sensors"][0]["data_type"];
                int new_sensor1_interval = j["sensors"][0]["data_interval"];

                std::string new_sensor2_id = j["sensors"][1]["sensor_id"];
                std::string new_sensor2_data_type = j["sensors"][1]["data_type"];
                int new_sensor2_interval = j["sensors"][1]["data_interval"];

                std::string topic1 = "/sensor_monitors/" + new_machine_id + "/" + new_sensor1_id;
                std::string topic2 = "/sensor_monitors/" + new_machine_id + "/" + new_sensor2_id;

                client.subscribe(topic1, QOS);
                client.subscribe(topic2, QOS);

                actual_timestamps.insert_or_assign(new_sensor1_id, "0T00:00:00");
                actual_timestamps.insert_or_assign(new_sensor2_id, "0");

                std::thread m_i_1(monitor_sensor_inactivity, new_sensor1_id , new_sensor1_interval);
                m_i_1.detach();
                std::thread m_i_2(monitor_sensor_inactivity, new_sensor2_id, new_sensor2_interval);
                m_i_2.detach();
            

            }

            else {
            
            std::string topic = msg->get_topic();
            auto topic_parts = split(topic, '/');
            std::string machine_id = topic_parts[2];
            std::string sensor_id = topic_parts[3];

            std::string timestamp = j["timestamp"];
            double value = j["value"];
    
            std::string path = machine_id + '.' + sensor_id;
            std::string metric = path + " " + std::to_string(value) + " " + timestamp;

            

            int sockfd;
            struct sockaddr_in serv_addr;

            

            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            

            if (sockfd < 0) {
                std::cerr << "Erro ao criar o socket" << std::endl;
                return;
            }
            

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(GRAPHITE_PORT);
            

            int v = inet_pton(AF_INET, GRAPHITE_HOST, &(serv_addr.sin_addr));
            if (v <= 0) std::cout << "deu erro aq";
            
            if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                std::cout << "Erro ao conectar ao servidor Graphite" << std::endl;
                close(sockfd);
                return;
            }
            
            

            std::string string_teste = "test_no_codigo.teste 20 20";

            if (send(sockfd, string_teste.c_str(), string_teste.length(), 0) < 0) {
                std::cerr << "Erro ao enviar dados para o servidor Graphite" << std::endl;
            }

           close(sockfd);
            
            
           

            /*std::thread post_t(post_metric, machine_id, sensor_id, timestamp, value);
            post_t.detach();*/
            
            actual_timestamps.insert_or_assign(sensor_id, timestamp);
            
            }
        }
    };

    

    callback cb;
    client.set_callback(cb);

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    try {
        client.connect(connOpts)->wait();
        client.subscribe("/sensor_monitors", QOS);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    while (true) {
        std::cout << actual_timestamps["sensor1"] << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}