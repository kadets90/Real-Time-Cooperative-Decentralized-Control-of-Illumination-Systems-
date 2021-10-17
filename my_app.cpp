#include <iostream>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>

/*#include <mutex>   // std::mutex
std::recursive_mutex mtx;    // mutexfor criticalsection
mtx.lock();
mtx.unlock();*/

using namespace boost::system;
using namespace boost::asio;

//GLOBALS
io_context io;
serial_port sp{ io };
deadline_timer tim{ io };
streambuf read_buf; //read buffer

int number_of_arduinos;
char input[8];
char get_something = 'N';
int get_id;
char* str;

bool** stream_data_map;
unsigned long** stream_start_timers;
auto start = chrono::steady_clock::now();

float* totais;
char total_variable = 'N';
int total_counter;

char get_buffer = 'N';
float** I_buffer;
int* I_max_indice;
int* I_actual_indice;
unsigned long* I_buffer_timer;

float** I_buffer_ref;
float ref = 20;

float** d_buffer;
int* d_max_indice;
int* d_actual_indice;
unsigned long* d_buffer_timer;

std::string get_chars = "oOULxrcptevf";

std::ofstream myfile;


void write_handler(const error_code& ec, size_t nbytes) {


	std::cin.getline(input, sizeof(input));

    int id;
    int value;

    switch (input[0]) {
        case 'P':
            std::cout << "Saving data..." << std::endl;
            input[0] = '\0';
            myfile.open("data.txt");
            if (!myfile.is_open()) {
                std::cout << "Unable to open file" << std::endl;
                break;
            }
            for (int j = 0; j < number_of_arduinos; ++j) {
                myfile << "d" << std::to_string(j + 1) << " = [" << std::endl;
                if (d_max_indice[j] == 0)
                    for (int i = 0; i < d_actual_indice[j]; ++i) {
                        myfile << d_buffer[j][i] << std::endl;
                    }
                else {
                    for (int i = d_actual_indice[j] + 1; i != d_actual_indice[j]; ++i) {
                        if (i == d_max_indice[j])
                            i = 0;
                        myfile << d_buffer[j][i] << std::endl;
                    }
                }
                myfile << "];\n" << std::endl;
            }
            for (int j = 0; j < number_of_arduinos; ++j) {
                myfile << "I" << std::to_string(j + 1) << " = [" << std::endl;
                if (I_max_indice[j] == 0)
                    for (int i = 0; i < I_actual_indice[j]; ++i) {
                        myfile << I_buffer[j][i] << std::endl;
                    }
                else {
                    for (int i = I_actual_indice[j] + 1; i != I_actual_indice[j]; ++i) {
                        if (i == I_max_indice[j])
                            i = 0;
                        myfile << I_buffer[j][i] << std::endl;
                    }
                }
                myfile << "];\n" << std::endl;
            }
            myfile.close();
            std::cout << "Data saved" << std::endl;
            break;
        case 'g':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
                input[0] = '\0';
                break;
            }
            get_something = 'A';
            switch (input[2]) {
                case 'I':
                    get_something = 'I';
                    get_id = int(input[4]) - 48;
                    input[0] = '\0';
                    break;

                case 'd':
                    get_something = 'd';
                    get_id = int(input[4]) - 48;
                    input[0] = '\0';
                    break;

                case 'o':
                    break;

                case 'O':
                    break;

                case 'U':
                    break;

                case 'L':
                    break;

                case 'x':
                    break;

                case 'r':
                    break;

                case 'c':
                    break;

                case 't':
                    break;

                case 'p':
                    if (input[4] == 'T') {
                        total_variable = 'p';
                        total_counter = 0;
                    }
                    break;

                case 'e':
                    if (input[4] == 'T') {
                        total_variable = 'e';
                        total_counter = 0;
                    }
                    break;

                case 'v':
                    if (input[4] == 'T') {
                        total_variable = 'v';
                        total_counter = 0;
                    }
                    break;

                case 'f':
                    if (input[4] == 'T') {
                        total_variable = 'f';
                        total_counter = 0;
                    }
                    break;

                default:
                    std::cout << "Bad request!\n";
                    input[0] = '\0';
                    break;
            }
            break;

        case 'o':
            if (input[1] != ' ' || input[3] != ' ' || (input[4] != '0' && input[4] != '1')) {
                std::cout << "Bad request!\n";
                input[0] = '\0';
            }
            break;

        case 'O':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
                input[0] = '\0';
            }
            break;

        case 'U':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
                input[0] = '\0';
            }
            break;

        case 'c':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
                input[0] = '\0';
            }
            break;

        case 'r':
            break;

        case 'b':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
            }
            get_id = int(input[4]) - 48;
            get_buffer = input[2];
            input[0] = '\0';
            break;

        case 's':
            if (input[1] != ' ' || input[3] != ' ') {
                std::cout << "Bad request!\n";
            }
            id = int(input[4]) - 48 - 1;
            if (input[2] == 'd') {
                stream_data_map[id][1] = !stream_data_map[id][1];
                auto end = chrono::steady_clock::now();
                stream_start_timers[id][1] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            }
            else if(input[2] == 'I'){
                stream_data_map[id][0] = !stream_data_map[id][0];
                auto end = chrono::steady_clock::now();
                stream_start_timers[id][0] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            }
            input[0] = '\0';
            break;

        default:
            std::cout << "Bad request!\n";
            input[0] = '\0';
            break;
    }
    async_write(sp, buffer(input), write_handler);
}

void read_handler(const error_code& ec, size_t nbytes) {

    std::istream is(&read_buf);
    std::string str;
    std::getline(is, str);

    int id = str[1] - 48;

    if (str[0] == 'I') {
        str.erase(0, 2); // value
        // code 48 corresponds to 0 in ASCII
        if (stream_data_map[id - 1][0]) {
            auto end = chrono::steady_clock::now();
            unsigned long actual_time = chrono::duration_cast<chrono::milliseconds>(end - start).count() - stream_start_timers[id - 1][0];
            std::cout << "s I " << std::to_string(id) << " " << str << " " << actual_time << std::endl;
        }
        if (get_something == 'I' && get_id == id) {
            std::cout << "I " << std::to_string(id) << " " << str << std::endl;
            get_something = 'N';
        }
        if (get_buffer == 'I' && get_id == id) {

            std::cout << "b I " << std::to_string(id) << std::endl;
            if(I_max_indice[id - 1] == 0)
                for (int i = 0; i < I_actual_indice[id - 1]; ++i) {
                    std::cout << I_buffer[id - 1][i] << std::endl;
                }
            else{
                for (int i = I_actual_indice[id - 1] + 1; i != I_actual_indice[id - 1]; ++i) {
                    if (i == I_max_indice[id - 1])
                        i = 0;
                    std::cout << I_buffer[id - 1][i] << std::endl;
                }
            }
            get_buffer = 'N';
        }
        std::string::size_type sz;
        I_buffer[id - 1][I_actual_indice[id - 1]++] = std::stof(str, &sz);
        auto end = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(end - start).count() - I_buffer_timer[id - 1] >= 60) {
            I_buffer_timer[id - 1] = chrono::duration_cast<chrono::seconds>(end - start).count();
            I_max_indice[id - 1] = I_actual_indice[id - 1];
            I_actual_indice[id - 1] = 0;
        }

    }
    else if (str[0] == 'd') {
        // code 48 corresponds to 0 in ASCII
        str.erase(0, 2); // value
        if (stream_data_map[id - 1][1]) {
            auto end = chrono::steady_clock::now();
            unsigned long actual_time = chrono::duration_cast<chrono::milliseconds>(end - start).count() - stream_start_timers[id - 1][1];
            std::cout << "s d " << std::to_string(id) << " " << str << " " << actual_time << std::endl;
        }
        if (get_something == 'd' && get_id == id) {
            std::cout << "d " << std::to_string(id) << " " << str << std::endl;
            get_something = 'N';
        }

        if (get_buffer == 'd' && get_id == id) {

            std::cout << "b d " << std::to_string(id) << std::endl;
            if (d_max_indice[id - 1] == 0)
                for (int i = 0; i < d_actual_indice[id - 1]; ++i) {
                    std::cout << d_buffer[id - 1][i] << std::endl;
                }
            else {
                for (int i = d_actual_indice[id - 1] + 1; i != d_actual_indice[id - 1]; ++i) {
                    if (i == d_max_indice[id - 1])
                        i = 0;
                    std::cout << d_buffer[id - 1][i] << std::endl;
                }
            }
            get_buffer = 'N';
        }

        std::string::size_type sz;
        d_buffer[id - 1][d_actual_indice[id - 1]++] = std::stof(str, &sz);
        auto end = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(end - start).count() - d_buffer_timer[id - 1] >= 60) {
            d_buffer_timer[id - 1] = chrono::duration_cast<chrono::seconds>(end - start).count();
            d_max_indice[id - 1] = d_actual_indice[id - 1];
            d_actual_indice[id - 1] = 0;
        }
    }
    else if(get_chars.find(str[0]) != std::string::npos && (get_something == 'A' || total_variable != 'N')){
        char field = str[0];
        str.erase(0, 2); // value
        if (field == total_variable) {
            std::string::size_type sz;
            totais[total_counter++] = std::stof(str, &sz);
            if (total_counter == number_of_arduinos) {
                float sum = 0;
                for (int i = 0; i < number_of_arduinos; ++i)
                    sum += totais[i];
                total_variable = 'N';
                std::cout << field << " T " << sum << std::endl;
            }
        }
        else {
            std::cout << field << " " << std::to_string(id) << " " << str << std::endl;
            get_something = 'N';
        }
    }
    else
        std::cout << str << std::endl;


	//program new read cycle
	async_read_until(sp, read_buf, '\n', read_handler);
}

void write_function() {
	io.run(); //get things rolling
}

void read_function() {
	io.run(); //get things rolling
}


void memory_allocations() {

    stream_data_map = (bool**)calloc(number_of_arduinos, sizeof(bool*));
    if (stream_data_map == NULL)
        exit(1);

    for (int i = 0; i < number_of_arduinos; ++i) {
        stream_data_map[i] = (bool*)calloc(number_of_arduinos, sizeof(bool));
        if (stream_data_map[i] == NULL)
            exit(1);
    }

    stream_start_timers = (unsigned long**)calloc(number_of_arduinos, sizeof(unsigned long*));
    if (stream_start_timers == NULL)
        exit(1);
    for (int i = 0; i < number_of_arduinos; ++i) {
        stream_start_timers[i] = (unsigned long*)calloc(number_of_arduinos, sizeof(unsigned long));
        if (stream_start_timers[i] == NULL)
            exit(1);
    }

    for (int i = 0; i < number_of_arduinos; ++i)
        for (int j = 0; j < number_of_arduinos; ++j)
            stream_data_map[i][j] = false;




    totais = (float*)calloc(number_of_arduinos, sizeof(float));
    if (totais == NULL)
        exit(1);



    I_buffer = (float**)calloc(number_of_arduinos, sizeof(float*));
    if (I_buffer == NULL)
        exit(1);
    for (int i = 0; i < number_of_arduinos; ++i) {
        I_buffer[i] = (float*)calloc(60 * 1000, sizeof(float));
        if (I_buffer[i] == NULL)
            exit(1);
    }

    I_max_indice = (int*)calloc(number_of_arduinos, sizeof(int));
    if (I_max_indice == NULL)
        exit(1);
    I_actual_indice = (int*)calloc(number_of_arduinos, sizeof(int));
    if (I_actual_indice == NULL)
        exit(1);
    I_buffer_timer = (unsigned long*)calloc(number_of_arduinos, sizeof(unsigned long));
    if (I_buffer_timer == NULL)
        exit(1);

    I_buffer_ref = (float**)calloc(number_of_arduinos, sizeof(float*));
    if (I_buffer_ref == NULL)
        exit(1);
    for (int i = 0; i < number_of_arduinos; ++i) {
        I_buffer_ref[i] = (float*)calloc(60 * 1000, sizeof(float));
        if (I_buffer_ref[i] == NULL)
            exit(1);
    }


    d_buffer = (float**)calloc(number_of_arduinos, sizeof(float*));
    if (d_buffer == NULL)
        exit(1);
    for (int i = 0; i < number_of_arduinos; ++i) {
        d_buffer[i] = (float*)calloc(60 * 1000, sizeof(float));
        if (d_buffer[i] == NULL)
            exit(1);
    }

    d_max_indice = (int*)calloc(number_of_arduinos, sizeof(int));
    if (d_max_indice == NULL)
        exit(1);
    d_actual_indice = (int*)calloc(number_of_arduinos, sizeof(int));
    if (d_actual_indice == NULL)
        exit(1);
    d_buffer_timer = (unsigned long*)calloc(number_of_arduinos, sizeof(unsigned long));
    if (d_buffer_timer == NULL)
        exit(1);
}

//MAIN : ERROR CHECKING ABSENT FOR SIMPLICITY
int main() {
    boost::system::error_code ec;
    sp.open("COM3", ec);    //connect to port
    if (ec) {
        std::cout << "Can not open serial port" << std::endl;
        return -1;
    }
    sp.set_option(serial_port_base::baud_rate(250000), ec);

    //std::cout << "Insert the number of desks:" << std::endl;
    //std::cin.getline(input, sizeof(input));

    /*std::stringstream aux;
    aux << "nT" << input;
    std::cout << aux.str() << std::endl;
    write(sp, buffer(aux.str(), sizeof(aux.str())));*/

    number_of_arduinos = 2;

    //std::stringstream ss(input);
    //ss >> number_of_arduinos;

    memory_allocations();

    async_write(sp, buffer(input), write_handler);
    //program chain of read operations
    async_read_until(sp, read_buf, '\n', read_handler);

    std::thread thread1{ write_function };
    std::thread thread2{ read_function };

    thread1.join();
    thread2.join();
}