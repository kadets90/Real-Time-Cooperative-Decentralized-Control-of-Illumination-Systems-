#ifndef HUB_H
#define HUB_H

#include <Arduino.h>

#define ID 2 // ID of the arduino (in our case is 1 or 2)
#define INDICE (ID - 1)

#define MSG_SIZE 8
#define NUM_MSG 40
#define WAIT_TIME 50 // ms
#define CALIB_TIME 200 // ms
#define BROADCAST_VALUE 16
#define get_value(x) (int(x) - 48) // 48 is 0 in ASCII
#define END_OF_COMUNICATION "N"
#define RUN_CONSENSUS "q"
#define SEND_ID(x) String(char(x + 48))

class data{

public:
  bool o; // occupancy flag
  float I, d, r; // illuminance, duty_cycle, reference_illuminance
  float O, U, L; // Occupied Lower Bound, Unoccupied Lower Bound, Actual Lower Bound
  float x, c, p; // external illuminace, energy cost, power consumption
  unsigned long int t; // elapsed time
  float e, v, f; // energy consumption, visibility error, flicker error (acumulated)
  
  data(float); // ctor
  void interpreter(String);
  String order();
  void update_data(float, float, unsigned long int);
  void reset_data();
  String check_consensus_values();
  void stream_values();
  int head_id;

private:
  float Lprev, cprev; // check if it is needed to run consensus 
  float I1, I2; // auxiliars to compute flicker error
  unsigned long int iteration; // number of loops
  
  float frequency;
  
  void get_info(String, String);
  void get_info_2(String, String);
  void set_value(String);
};

void test();
void init_hub();
String read_hub();
void write_hub(String, int);
String ask(String, int);
String get_order_from_buffer(int);
int look_for_msg();

#endif //HUB_H
