#ifndef alerts_h
#define alerts_h


//#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
//#else
//#include "WProgram.h"
//#endif

#include "sensors_actuators.h"



class Reference_group
{
public:
    String name;            // Name to group a set of alerts 
                            // When the name of the alert is ANY, 
                            // this name should be a specific magnitude such as: Temperature, kWh,..
    short int *index;       // array of indexes of the associated alerts 
    bool is_sensor;         // 
    bool digital;           // 
    bool any;               // if the reference group is a magnitude, then it comprises every sensor with this magnitude 
    short int N =0;
//    float mean = 0;
//    float deviation = 0 ;
};

class Condition
{
public:
    String name;             // name to associate a bunch of alerts 
    short int *index;        // array of indexes of the associated alerts 
    short int N;             // number of the associated alerts 
    bool on;                 // condition is TRUE if every associated alert is true 
    byte sent_at_hour = 25;  // hour at which the condition(bunch of alerts) is sent by email 
};





class Alert
{



public:

    //** Given values by the user or the WEB page 
    String name;                   // name of the alert: sensor, actuator or ANY 
    String magnitude;              // magnitude of the alert corresponding to its associated sensor
    String device;                 // device of the alert corresponding to its associated  actuator
    String min_value, max_value;   // if the sensor is out of this range, then alert is ON
    String value;                  // if the digital sensor or the actuator is equal to VALUE, then alert is ON 
    String reference_group;        // given name to check alerts with regards to a specific group 
    String condition;              // given name to link different alerts 
    String time_begin, time_end;    // time range to check the alert : 0 to 24 hours 
    String days;                   // days of the week to check the alert 
    String mode;                   // check if this mode coincides with Atentto mode 
    String actions;                // actions to be carried out: email, sound, all, none 
    String report;                 // periodicity of the reports: daily, monthly,...

                                   //** Calculated values 
 //   String magnitude; // magnitude of the alert
 //   String device;    // device of the alert
    bool is_sensor;   // true this alert is a sensor
                      // false this alert is an actuator

    bool digital;     // true is the sensor is digital 

    short int index;  // index of table of sensors of actuators
                      // if this alert is ANY, then this index 
                      // is where the maximum is reached 

    short int index_reference_group;  // index of reference group array 
                   

    float t_begin, t_end;  // time range( float values)
    float v_max, v_min;    // range of the alert (float values) 


    bool on;              // value of the alert: TRUE or FALSE 
    bool enforceable;     // The alert is only evaluted if the present time is in time range 

  //  float seconds_on;     // if the alert is a digital sensor or an actuator, 
                          // seconds_on measures the time this actuator is ON 


  //  float max, min, mean; // max, min and mean value the alert reaches during the reporting period 
  //  Time_Date tmax, tmin; // time and date at max or min
  //  float deviation;      // typical deviation of the alert with regards to the reference group
 //   float RG_mean = 0;    // typical deviation of the alert with regards to the reference group 
 //   float sum;            // sum of the associated sensors
 //   long int N_samples;   // number of times the alert is evaluated to calculate the mean value 

 //   byte sent_at_hour = 25;  // hour at which the report is sent by email 

    void ini();

  //  void set_maxmin(Time_Date, float, Sensor[], short int, Actuator[], short int);

    void check(Time_Date, char[], Sensor[], short int, Actuator[], short int);

    void write_email( char[], Sensor[], Actuator[]);

  //  short int sound_priority(String);

 //   void set_report( Time_Date,  char [], bool ); 
    
    

private: 

  //  void set_maxmin_sensors(Time_Date, Sensor[], short int);
  //  void set_maxmin_sensor(Time_Date, float);
  //  void set_maxmin_actuators(float DT, Actuator[], Sensor[]);
  //  void set_actuator_timer(float, Actuator []);
  //  void set_digital_sensor_timer(float, Sensor []);
 //   short int get_level();

  //  bool report_periodicity(Time_Date); 
  //  void write_sensor_report( char []);
   // void write_actuators_and_digit_sensors_report(char []);
  //  void reset_values();
    

	void check_actuators(Actuator [], short int);
	void check_sensors(Sensor [], short int );
//	bool notnormal( float );
    void check_sensors_value(float);
    void check_actuators_value(unsigned char);

	
   
};

	
void test_alerts_and_reports();

void alerts_setup(int &,  JSON_Row [], Sensor [], short int, Actuator [], short int);
	
short int verify_alerts(char[], Time_Date, short int, Sensor [], short int, Actuator [], char[], char[], char[]);


#endif
