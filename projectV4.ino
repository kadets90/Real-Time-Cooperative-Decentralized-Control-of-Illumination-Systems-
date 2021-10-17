#include <stdlib.h>
#include"pid.h"
#include"hub.h"
#include"consensus.h"

using namespace std;

/*********************/
/***** CONSTANTS *****/
/*********************/
#define MAX_DC 255
#define NOMINAL_POWER 1
#define FREQUENCY 100.0 // Hz
#define T 1/FREQUENCY // seconds

// Initialize led and ldr pins
int led_pin = 9;
int ldr_pin = A0;

// LDRs datasheet information
float m;
float b;
// constant time
float slope_time;
float offset_time;


// Circuit elements
float R1 = pow(10,4); // Resistence 1

//float duty_cycle;
float vi = 0;
unsigned long int initial_time;
//float illuminance;
float expected_illuminance;
float v;

//PID constants
float kp = 2;
float ki = 20;
float kd = 0.0;
float a = 0.0;


int number_of_arduinos = 2;

// luminance coeficentes and static error
struct calibration_data{ 
   float* e;
   float** L; // just the ones that we needs to compute his own duty_cycle
};

struct calibration_data calib_data;

// declare a local object
pid c{kp, ki, kd, T, a};
data my_data{FREQUENCY};
admm qp{ID, MAX_DC};


/*********************/
/***** FUNCTIONS *****/
/*********************/
// turn on the led with a value of duty cicle between 0 and 255 (8 bit word)
void actuate_LED(float duty_cycle){
  analogWrite(led_pin, round(duty_cycle));
}

float limit_duty_cycle(float duty_cycle){
    if(duty_cycle > MAX_DC){
    duty_cycle = MAX_DC;
  }
  else if(duty_cycle < 0.0){
    duty_cycle = 0.0;
  }
  return duty_cycle;
}

// returns the value of the voltage in volts (between 0 and 5V)
float get_voltage(){
  int VCC = 1023; // 5 volts in 10 bits word
  return float(analogRead(ldr_pin))/VCC*5;
}

// reads the voltage and returns the illuminance value
float read_illuminance(float v0){
  float R2 = (5.0/v0-1)*R1;
  return pow(R2/pow(10,b),1/m); // return illuminance
}

// expected value of voltage for a certain PWM value
float expected_voltage(float illuminance_reference, float duty_cycle_reference, float vi, unsigned long int initial_time){

  float tau = slope_time*duty_cycle_reference + offset_time;  // ms 
  float R2 = pow(illuminance_reference, m)*pow(10, b);
  float vf = 5*R1/(R1+R2);


  float v = vf + (vi - vf)*exp( -double(millis() - initial_time)/tau);
  return v;
}

void apply_consensus(){

  String ch;
  int number_of_tries;

  delay(10);
  for(int x = 0; x < qp.max_iterations; ++x){
    qp.argmin();
    
    for(int i = 0; i < number_of_arduinos; ++i){
      for(int j = 0; j < number_of_arduinos; ++j){
        if(j == INDICE){
          write_hub(String(qp.d[i]), BROADCAST_VALUE);
        }else{
          
          ch = ask(END_OF_COMUNICATION, 0);
          qp.d_[i] += ch.toFloat();
        }
      }
    }
    
    for(int j = 0; j < number_of_arduinos; ++j){
        qp.d_[j] /= number_of_arduinos;
    }
    
    for(int j = 0; j < number_of_arduinos; ++j){
        qp.y[j] += qp.rho*(qp.d[j] - qp.d_[j]);
    }
  }

  for(int j = 0; j < number_of_arduinos; ++j){
    qp.d[j] = qp.d_[j];
    Serial.print("D" + String(j + 1) + ": " + String(qp.d[j]) + "\n");
  }
}

void compute_coefficents(){

  String ch;
  int number_of_tries;

  
  Serial.print("OFF: ");
  actuate_LED(0);
  delay(CALIB_TIME);
  my_data.I = read_illuminance(get_voltage());

  // static part
  for(int i = 0; i < number_of_arduinos; ++i){
    if(i == INDICE){
      write_hub(String(my_data.I), BROADCAST_VALUE);
      calib_data.e[INDICE] = my_data.I;
    }else{
      
      number_of_tries = 0;
      while(number_of_tries < number_of_arduinos*2){
        ch = ask(END_OF_COMUNICATION, 0);
        if(ch[0] > 47 && ch[0] < 58)
          break;
        else
          number_of_tries++;
      }
        
      if(number_of_tries == number_of_arduinos*2){
        Serial.print("Err\n");
      }

      calib_data.e[i] = ch.toFloat();
    }
    Serial.print("L" + String(i + 1) + " = " + String(calib_data.e[i]) + " ");
  }
  Serial.print("\n");
   
  // turn on leds
  for(int led = 0; led < number_of_arduinos; ++led){
    Serial.print("LED " + String(led + 1) + " ON: ");
    if(led == INDICE)
      actuate_LED(MAX_DC);
      
    delay(CALIB_TIME);
    my_data.I = read_illuminance(get_voltage());

    for(int i = 0; i < number_of_arduinos; ++i){
      if(i == INDICE){
        write_hub(String(my_data.I), BROADCAST_VALUE);
        calib_data.L[INDICE][led] = (my_data.I - calib_data.e[INDICE])/MAX_DC;
        Serial.print("L" + String(i + 1) + " = " + String(my_data.I) + " ");
      }else{
        ch = ask(END_OF_COMUNICATION, 0);
        calib_data.L[i][led] = (ch.toFloat() - calib_data.e[i])/MAX_DC;
        Serial.print("L" + String(i + 1) + " = " + String(ch.toFloat()) + " ");
      }
    }

    Serial.print("\n");
    actuate_LED(0);
  }
  
  for(int j = 0; j < number_of_arduinos; ++j)
    Serial.println(calib_data.e[j]);
  
  for(int j = 0; j < number_of_arduinos; ++j){
    for(int i = 0; i < number_of_arduinos; ++i)
      Serial.print(String(calib_data.L[j][i]) + " ");
    Serial.print("\n");
  }
}





/*********************/
/***** SETUP/RUN *****/
/*********************/
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(250000);
  pinMode(led_pin, OUTPUT);
  pinMode(ldr_pin, INPUT);
  
  // properties of each ID
  if(ID == 1){
    // LDRs datasheet information
    m = -0.6855;
    b = log10(65*pow(10,3));
    // constant time
    slope_time = -0.0545;
    offset_time = 30.1342;
  }
  else{
    // LDRs datasheet information
    m = -0.7;
    b = 4.85;
    // constant time
    slope_time = -0.18;
    offset_time = 76.301;
  }
  
  init_hub();
    
  calib_data.e = (float*) calloc (number_of_arduinos, sizeof(float));
  if(calib_data.e == NULL)
    Serial.print("Error e\n");
    
  calib_data.L = (float**) calloc (number_of_arduinos, sizeof(float*));
  if(calib_data.L == NULL)
    Serial.print("Error L\n");
    
  for(int i = 0; i < number_of_arduinos; ++i){
    calib_data.L[i] = (float*) calloc (number_of_arduinos, sizeof(float));
    if(calib_data.L[i] == NULL)
      Serial.print("Error Li\n");
  }
  
  qp.init(number_of_arduinos);
  write_hub("R", BROADCAST_VALUE);
}

unsigned long int loop_time = 0;
unsigned long int restart_time;
String ch = END_OF_COMUNICATION;
String my_request = "R";

// the loop function runs over and over again forever
void loop(){
  
  if(my_request == RUN_CONSENSUS){
    
    qp.update(my_data.c, calib_data.L[INDICE], calib_data.e[INDICE], my_data.L); // run consensus
    apply_consensus();

    qp.d[INDICE] = limit_duty_cycle(qp.d[INDICE]);
    my_data.d = qp.d[INDICE];
    my_data.r = 0; 
    for(int i = 0; i < number_of_arduinos; ++i)
      my_data.r = my_data.r + calib_data.L[INDICE][i]*limit_duty_cycle(qp.d[i]);
    my_data.r = my_data.r + calib_data.e[INDICE];
    
    vi = get_voltage();
    initial_time = millis();
    
  }else if(my_request == "R"){

    compute_coefficents();
    my_data.reset_data();
    
    // consensus
    qp.update(my_data.c, calib_data.L[INDICE], calib_data.e[INDICE], my_data.L); // run consensus
    apply_consensus();
    
    qp.d[INDICE] = limit_duty_cycle(qp.d[INDICE]);
    my_data.x = calib_data.e[INDICE];
    for(int i = 0; i < number_of_arduinos; ++i)
      my_data.r = my_data.r + calib_data.L[INDICE][i]*limit_duty_cycle(qp.d[i]);
    my_data.r = my_data.r + calib_data.e[INDICE];
      
    my_request = END_OF_COMUNICATION;
    vi = get_voltage();
    initial_time = millis();
    restart_time = millis();
  }


  // Give time to the message to reach the other point
  // PART 1 ///////////////////////////////////////////////////////////////////////////////////
  if((millis() - loop_time) > T*1000){
    my_data.update_data(NOMINAL_POWER, MAX_DC, restart_time);
    
    my_data.d = qp.d[INDICE];
    
    v = expected_voltage(my_data.r, my_data.d, vi, initial_time);
    expected_illuminance = read_illuminance(v);
    my_data.I = read_illuminance(get_voltage());
  
    my_data.d = limit_duty_cycle(my_data.d + c.calc(expected_illuminance, my_data.I, my_data.d));
    actuate_LED(my_data.d);
    //Serial.print("Time: " + String((millis() - loop_time)/1000.0) + " seconds\n");
    loop_time = millis();
    my_data.stream_values();
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
      
  my_request = my_data.order();  // Serial Input

  if(my_request == END_OF_COMUNICATION)
    my_request = my_data.check_consensus_values();

  int number_of_messages = look_for_msg();
  
  while(number_of_messages){       
    ch = get_order_from_buffer(--number_of_messages);
    my_data.interpreter(ch);
    
    if(String(ch[0]) == RUN_CONSENSUS){
      my_request = RUN_CONSENSUS;
    }else if(String(ch[0]) == "R"){
      my_request = "R";
    }
  }
  
}
