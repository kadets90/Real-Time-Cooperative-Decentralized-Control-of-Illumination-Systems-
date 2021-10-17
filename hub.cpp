#include<SPI.h>
#include<mcp2515.h>
#include "hub.h" 
using namespace std;

struct can_frame canMsg;
MCP2515 mcp2515(10);  //SS pin 10
uint32_t mask= 0xFFFFFFFF;
uint32_t filt0 = BROADCAST_VALUE; //accepts msg ID 1 on RXB0
//uint32_t filt2 = ID + 1000; //accepts msg ID 1 on RXB0
uint32_t filt1 = ID; //accepts msg ID 2 on RXB1
char read_buffer[NUM_MSG][MSG_SIZE + 1];

void init_hub(){
	SPI.begin();
	mcp2515.reset();
	mcp2515.setBitrate(CAN_1000KBPS, MCP_16MHZ);
  mcp2515.setFilterMask(MCP2515::MASK0, 0, mask);
  mcp2515.setFilterMask(MCP2515::MASK1, 0, mask);
  
  //filters related to RXB0
  mcp2515.setFilter(MCP2515::RXF0, 0, filt0);
  mcp2515.setFilter(MCP2515::RXF1, 0, filt0);
  
  //filters related to RXB1
  mcp2515.setFilter(MCP2515::RXF2, 0, filt0);
  mcp2515.setFilter(MCP2515::RXF3, 0, filt0);
  mcp2515.setFilter(MCP2515::RXF4, 0, filt0);
  mcp2515.setFilter(MCP2515::RXF5, 0, filt0);
  
  mcp2515.setFilter(MCP2515::RXF2, 0, filt1);
  mcp2515.setFilter(MCP2515::RXF3, 0, filt1);
  mcp2515.setFilter(MCP2515::RXF4, 0, filt1);
  mcp2515.setFilter(MCP2515::RXF5, 0, filt1);
	mcp2515.setNormalMode();   // or setLoopbackMode() for debug
}


// read a message from the other arduino
String read_hub(){
	if(mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // && (ID == canMsg.can_id || BROADCAST_VALUE == canMsg.can_id)
    char ch[MSG_SIZE + 1];
    for(int i = 0; i < canMsg.can_dlc; i++){  // print the data
      ch[i] = char(canMsg.data[i]);
    }
    return ch;
	}else{
    return END_OF_COMUNICATION;
	}
}

// send a message for the other arduino
void write_hub(String msg, int id){
	canMsg.can_id  = id; //ID of the message
	canMsg.can_dlc = MSG_SIZE;

	for(int i = 0; i < MSG_SIZE; i++)
    canMsg.data[i] = msg[i];

	mcp2515.sendMessage(&canMsg);
}

// function that sends a message and waits for an answer from the other side
// could just be used to wait for a message with the Input END_OF_COMUNICATION
String ask(String x, int id){
  
  if(id != 0)
    write_hub(x, id);
    
  String ch;
  unsigned long int final_time = millis() + WAIT_TIME;
  
  while(String((ch = read_hub())[0]) == END_OF_COMUNICATION)
    if(millis() > final_time)
      break;

  return ch;
}

int look_for_msg(){
  
  String ch;
  int number_of_messages = 0;
  while(String((ch = read_hub())[0]) != END_OF_COMUNICATION){
    for(int i = 0; i < MSG_SIZE; i++)
      read_buffer[number_of_messages][i] = ch[i];
    number_of_messages++;
  }

  return number_of_messages;
}

String get_order_from_buffer(int indice){
  return String(read_buffer[indice]);
}

// Class data
data::data(float freq){
  
  o = false; // unoccupied
  I = 0;
  d = 0;
  r = 0; 
  O = 40.0;
  U = 20.0;
  L = O*o + U*!o;
  x = 0;
  c = 1;
  p = 0; // external illuminace, energy cost, power consumption
  t = 0;
  e = 0;
  v = 0;
  f = 0; // energy consumption, visibility error, flicker error (acumulated)

  Lprev = L;
  cprev = c;
  I1 = 0, I2 = 0;
  iteration = 0;

  frequency = freq;
  head_id = 0;
}

// receives a string from the serial input, decode it and act according to
// get or set a value of an arduino, reset the system, create a buffer or
// start streaming a value 
String data::order(){
  
  if (Serial.available() > 0){
    if(head_id != ID){
      head_id = ID;  // I am the HUB
      write_hub("H" + String(head_id), BROADCAST_VALUE); // Tell the world that I am the HUB
    }
    
    // read_serial
    //////////////////////////////////////////////////////////////////
    int i = 0;
    char ch[MSG_SIZE + 1];
    while(Serial.available() > 0 && i <= MSG_SIZE){  // print the data
      ch[i++] = char(Serial.read());
    }
    ch[i] = '\0';
    String serial_str = String(ch);
    //////////////////////////////////////////////////////////////////
    
    switch(serial_str[0]){
      case 'g':
        switch(serial_str[2]){
          case 'o':
            get_info(serial_str, String(o));
            break;
          
          case 'O':
            get_info(serial_str, String(O));
            break;
          
          case 'U':
            get_info(serial_str, String(U));
            break;
          
          case 'L':
            get_info(serial_str, String(L));
            break;
          
          case 'x':
            get_info(serial_str, String(x));
            break;
          
          case 'r':
            get_info(serial_str, String(r));
            break;
          
          case 'c':
            get_info(serial_str, String(c));
            break;
          
          case 't':
            get_info(serial_str, String(t));
            break;
          
          case 'p':
            get_info_2(serial_str, String(p));
            break;
            
          case 'e':
            get_info_2(serial_str, String(e));
            break;
            
          case 'v':
            get_info_2(serial_str, String(v));
            break;
            
          case 'f':
            get_info_2(serial_str, String(f));
            break;
          
          default:
            break;
        }
        break;
        
      case 'o':
        set_value(serial_str);
        Serial.print("ack\n");
        break;
        
      case 'O':
        set_value(serial_str);
        Serial.print("ack\n");
        break;
        
      case 'U':
        set_value(serial_str);
        Serial.print("ack\n");
        break;
        
      case 'c':
        set_value(serial_str);
        Serial.print("ack\n");
        break;
        
      case 'r':
        write_hub("R", BROADCAST_VALUE);
        Serial.print("ack\n");
        return "R";
        break;
        
      default:
        break;
    }
  }
  
  return END_OF_COMUNICATION;
}

// it decides what to do when a message is recived through the can bus
void data::interpreter(String order){

  switch(order[0]){
    case 'g':  
      switch(order[2]){
        case 'I':
          write_hub("I" + SEND_ID(ID) + String(I), BROADCAST_VALUE);
          break;
        
        case 'd':
          write_hub("d" + SEND_ID(ID) + String(d), BROADCAST_VALUE);
          break;
        
        case 'o':
          write_hub("o" + SEND_ID(ID) + String(int(o)), BROADCAST_VALUE);
          break;
        
        case 'O':
          write_hub("O" + SEND_ID(ID) + String(O), BROADCAST_VALUE);
          break;
        
        case 'U':
          write_hub("U" + SEND_ID(ID) + String(U), BROADCAST_VALUE);
          break;
        
        case 'L':
          write_hub("L" + SEND_ID(ID) + String(L), BROADCAST_VALUE);
          break;
        
        case 'x':
          write_hub("x" + SEND_ID(ID) + String(x), BROADCAST_VALUE);
          break;
        
        case 'r':
          write_hub("r" + SEND_ID(ID) + String(r), BROADCAST_VALUE);
          break;
        
        case 'c':
          write_hub("c" + SEND_ID(ID) + String(c), BROADCAST_VALUE);
          break;
        
        case 'p':
          write_hub("p" + SEND_ID(ID) + String(p), BROADCAST_VALUE);
          break;
        
        case 't':
          write_hub("t" + SEND_ID(ID) + String(t), BROADCAST_VALUE);
          break;
        
        case 'e':
          write_hub("e" + SEND_ID(ID) + String(e), BROADCAST_VALUE);
          break;
      
        case 'v':
          write_hub("v" + SEND_ID(ID) + String(v), BROADCAST_VALUE);
          break;
        
        case 'f':
          write_hub("f" + SEND_ID(ID) + String(f), BROADCAST_VALUE);
          break;
        
        default:
          break;
      }
      break;

    // set values
    case 's':
      switch(order[1]){
        case 'o':
          o = order.substring(2,MSG_SIZE).toFloat();
          L = O*o + U*!o;
          break;
          
        case 'O':
          O = order.substring(2,MSG_SIZE).toFloat();
          L = O*o + U*!o;
          break;
          
        case 'U':
          U = order.substring(2,MSG_SIZE).toFloat();
          L = O*o + U*!o;
          break;
          
        case 'c':
          c = order.substring(2,MSG_SIZE).toFloat();
          break;

        default:
          break;
      }
      break;
      
    case 'R':
      break;
      
    case 'q':
      break;

    // receive from get from a priveous ask
    case 'I':
      Serial.print(order + "\n");
      break;
      
    case 'd':
      Serial.print(order + "\n");
      break;

    case 'o':
      Serial.print(order + "\n");
      break;
    
    case 'O':
      Serial.print(order + "\n");
      break;
    
    case 'U':
      Serial.print(order + "\n");
      break;
    
    case 'L':
      Serial.print(order + "\n");
      break;
    
    case 'x':
      Serial.print(order + "\n");
      break;
    
    case 'r':
      Serial.print(order + "\n");
      break;
    
    case 'c':
      Serial.print(order + "\n");
      break;
    
    case 'p':
      Serial.print(order + "\n");
      break;
    
    case 't':
      Serial.print(order + "\n");
      break;
    
    case 'e':
      Serial.print(order + "\n");
      break;
  
    case 'v':
      Serial.print(order + "\n");
      break;
    
    case 'f':
      Serial.print(order + "\n");
      break;

    case 'H':
      head_id = order.substring(1,MSG_SIZE).toFloat();
      break;
      
    default:
      break;
  }
  
}

// function get, when 'g' value is recived this function checks the id
// and decide if it is to print a value or ask the other arduino for
// it 
void data::get_info(String serial_str, String own_value){
  
  int id = get_value(serial_str[4]);

  if(get_value(serial_str[4]) == ID){
    Serial.print(String(serial_str[2]) + SEND_ID(ID) + own_value + "\n");
  }else{
    write_hub(serial_str.substring(0,3), id);
  }  
}

// function get, when 'g' value is recived this function checks the id
// and decide if it is to print a value, ask the other arduino for
// it, or do both to get total value
void data::get_info_2(String serial_str, String own_value){

  int id = get_value(serial_str[4]);
  if(get_value(serial_str[4]) == ID){
    Serial.print(String(serial_str[2]) + SEND_ID(ID) + own_value + "\n");
  }else if(serial_str[4] == 'T'){
    Serial.print(String(serial_str[2]) + SEND_ID(ID) + own_value + "\n");
    write_hub(serial_str.substring(0,3), BROADCAST_VALUE);
  }else{
    write_hub(serial_str.substring(0,3), id);
  }

  
}

// set a value
void data::set_value(String serial_str){

  String value;
  int id = get_value(serial_str[2]);

  value = serial_str.substring(4,MSG_SIZE);

  if(id == ID){
    switch(serial_str[0]){
      case 'o':
        o = value.toFloat();
        break;
        
      case 'O':
        O = value.toFloat();
        break;
        
      case 'U':
        U = value.toFloat();
        break;
        
      case 'c':
        c = value.toFloat();
        break;
        
      default:
        break;
    }
    L = O*o + U*!o;
  }else{
    write_hub("s" + String(serial_str[0]) + value, id);
  }
}


// Compute the power in the moment
void data::update_data(float nominal_power, float max_duty_cycle, unsigned long int initial_time){
  
  p = nominal_power*d/max_duty_cycle;
  e += p/frequency;
  t = (millis() - initial_time)/1000.0; // seconds
  iteration++;
  
  if(L > I){
    v = (v*(iteration - 1) + (L - I))/iteration;
  }else{
    v = v*(iteration - 1)/iteration; 
  }
  
  if( (I - I1) * (I1 - I2) < 0){
    f = (f*(iteration - 1) + (abs(I - I1) + abs(I1 - I2))/2*frequency)/iteration;
  }else{
    f = f*(iteration - 1)/iteration;
  }
  I2 = I1;
  I1 = I;
}

void data::reset_data(){

  o = false; // unoccupied
  I = 0;
  d = 0;
  r = 0; 
  O = 40.0;
  U = 20.0;
  L = O*o + U*!o;
  x = 0;
  c = 1;
  p = 0; // external illuminace, energy cost, power consumption
  t = 0;
  e = 0;
  v = 0;
  f = 0; // energy consumption, visibility error, flicker error (acumulated)

  Lprev = L;
  cprev = c;
  I1 = 0, I2 = 0;
  iteration = 0;
}

String data::check_consensus_values(){

  if(Lprev != L || cprev != c){
    Lprev = L;
    cprev = c;
    // need to run consensus
    write_hub(RUN_CONSENSUS, BROADCAST_VALUE);
    return RUN_CONSENSUS;
  }
  else
    return END_OF_COMUNICATION;
}


void data::stream_values(){

  // Variable: 1 byte
  // ID: 1 byte (255 IDs available, excluding 0)
  // value: 6 bytes  
  if(head_id == ID){
    Serial.print("I" + SEND_ID(ID) + String(I) + "\n");
    Serial.print("d" + SEND_ID(ID) + String(d) + "\n");
  }
  else if(head_id != 0){
    write_hub("I" + SEND_ID(ID) + String(I), head_id);
    write_hub("d" + SEND_ID(ID) + String(d), head_id);
  } 
}
