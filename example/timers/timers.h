

//#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
//#else
//    #include "WProgram.h"
//#endif



class Timer
{
    public:
        String name;
        String begin;
        String end;
        String days; 
        String mode;
        float t_begin;
        float t_end; 
        bool on; 
        short int state; 
        bool enforceable; 
        void setup();
        short int check(struct Time_Date );
    
      

};

void timers_setup( Timer [],  short int, JSON_Row []  );
float Sunrise( struct Time_Date ); 
float Sunset( struct Time_Date ); 

