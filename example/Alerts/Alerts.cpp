
#include "misc.h"
#include "print_debug.h"
#include "RTC_SAM3X.h"
#include "JSON_tools.h"
#include "Alerts.h"
#include "watchdog.h"


#define LOW 1
#define MEDIUM 2
#define HIGH 3
#define VERY_HIGH 4

Reference_group *RG = NULL;
short int N_groups;

Condition *conditions = NULL;
short int N_conditions;

short int N_alerts;
Alert* alerts = NULL;

bool verbose_alert = false;  // true;//




//**********************************************************************
// Warnings such as beeps or emails are carried out 
// emails are prepared with: 
//                          message : body 
//                          state   : content 
//**********************************************************************
short int verify_alerts(char facility[], Time_Date timedate, short int N_sensors, Sensor sensors[], short int N_actuators, Actuator actuators[], char main_switch[], char message[], char state[])
{
    short int i, j, k;
    short int  sound_level;
    char cname[20];
    bool report_alert, sound;


    strcpy(state, "");
    sound_level = 0; 

    //**  process every alarm by means of its value (out of range ? ) 
    for (i = 0; i < N_alerts; i++) alerts[i].check(timedate, main_switch, sensors, N_sensors, actuators, N_actuators);

    //**Conditional Alert = Alert_i && Alert_j &&..  
    for (i = 0; i < N_conditions; i++)
    {
        conditions[i].on = true;
        for (j = 0; j < conditions[i].N; j++)
        {
            k = conditions[i].index[j];
            conditions[i].on = conditions[i].on && alerts[k].on;
        }
        if (verbose_alert)
        {
          print(" conditions.name = "); print(conditions[i].name);
          print("     ON = "); print(conditions[i].on);
          print(" sent_at_hour = "); println(conditions[i].sent_at_hour);
        }
       
    }

    //**email for those true conditional alerts every hour
    //  the email is formed with different rows of every alert. 
    //  Different conditionals are sent consecutively

    for (i = 0; i < N_conditions; i++)
    {
        if (conditions[i].on)
        {
            if (conditions[i].sent_at_hour != timedate.hour)
            {
                conditions[i].name.toCharArray(cname, 20);
                strcat(state, "The conditional alert: "); strcat(state, cname); strcat(state, " is met \n");

                for (j = 0; j < conditions[i].N; j++)
                {
                    k = conditions[i].index[j];
                    report_alert = (alerts[k].actions == "email") ||
                        (alerts[k].actions == "All") ||
                        (alerts[k].actions == "ALL");
                    if (report_alert) alerts[k].write_email(state, sensors, actuators);
                } 
                conditions[i].sent_at_hour = timedate.hour;
            }
            for (j = 0; j < conditions[i].N; j++)
            {
                k = conditions[i].index[j];
                sound_level = (alerts[k].actions == "sound") ||
                    (alerts[k].actions == "All") ||
                    (alerts[k].actions == "ALL");
            }
        }
     
        if (strcmp(state, "") != 0)
        {
            strcpy(message, "WARNING from: "); strcat(message, facility);
        } 
        return sound_level;
    }

    return 0;


}




void Alert::check(Time_Date timedate, char main_switch[], Sensor sensors[], short int N_sensors, Actuator actuators[], short int N_actuators)
{
    String periodicity[10] = { "Daily", "Mon-Fri", "Sat-Sun",  "Mon", "Tue", "Wed",  "Thu", "Fri", "Sat", "Sun" };
    short int iday, iday_of_week, i;
    char char_mode[7];
    float dec_hour, v;

    if (verbose_alert) println(str("BEGIN alert check "));

    //Determines the state of the days mode
    iday = find_index(days, periodicity, 10) - 2;
    iday_of_week = timedate.dayOfWeek;

    //Establish the current time as a decimal
    dec_hour = timedate.hour + timedate.minute / 60.;

    mode.toCharArray(char_mode, 7);

    // Check alert if main_switch mode coincides with alert mode 
    if (!strcmp(char_mode, main_switch) || mode == "ANY")
    {
        //Check if the current day if the day selected in the alert or in the range of days selected
        if ((iday < -2 || iday>7) && verbose_alert) { print(str(" ERROR Alert::check  iday ="));  println(iday); }

        if (days == "Daily")				      		     enforceable = true;
        else if (days == "Mon-Fri" && iday_of_week <= 5)		 enforceable = true;
        else if (days == "Sat-Sun" && iday_of_week >= 6)		 enforceable = true;
        else if (iday == iday_of_week)						 enforceable = true;
        else													 enforceable = false;

        if (enforceable)
        {
            if (time_begin == "")                              enforceable = true;

            // Current time is between its enforceable range
            else if (dec_hour >= t_begin && dec_hour <= t_end) enforceable = true;
            else												 enforceable = false;

        }
    }
    else enforceable = false;


    if (enforceable)
    {

        if (is_sensor) check_sensors(sensors, N_sensors);
        else           check_actuators(actuators, N_actuators);

        if (verbose_alert)
        {
            println(str("__________________________"));
            print(str("is_sensor is:")); println(is_sensor);
            if (is_sensor) v = (float)sensors[index].value;
            else           v = (float)actuators[index].state;

            print(str("Alert: ")); print(name);
            print(str("  Activation is: "));  print(on);
            print(str("  Value=")); println(v);
            print(str("  index=")); println(index);

        }

    }
    else on = false;
}


void Alert::check_sensors_value(float f)
{
    const float _on = 1.0, _off = 0.0, _open = 1.0, _close = 0.0, _null = 10;

    if (value == "ON")	            on = f == _on;
    else if (value == "OFF")	    on = f == _off;
    else if (value == "OPEN")	    on = f == _open;
    else if (value == "CLOSE")	    on = f == _close;
    else                            on = f == _null;


}


void Alert::check_actuators_value(unsigned char c)
{
    const short int _up = 1, _down = 2, _on = 1, _off = 0, _null = 10;


    // if      (value == "NOTNORMAL") on = notnormal(seconds_on);
    if (value == "UP")		       on = c == _up;
    else if (value == "DOWN")	   on = c == _down;
    else if (value == "ON")		   on = c == _on;
    else if (value == "OFF")	   on = c == _off;
    else                           on = c == _null;

}



void Alert::check_sensors(Sensor sensors[], short int N_sensors)
{

    float fvalue, v, vm;
    short int i;

    //** check range of sensors between max and min 
    if ((min_value != "" && max_value != ""))
    {
        //** check if there is at least one sensor out of range 
        if (name == "ANY")
        {
            on = false; index = 0; vm = 0;
            for (i = 0; i < N_sensors; i++)
                if (sensors[i].magnitude == reference_group)
                {
                    v = sensors[i].value;

                    if (v < v_min || v > v_max)
                    {
                        if (abs(v) > vm) { index = i; vm = abs(v); }
                        on = on || true;
                    }
                    else  on = on || false;
                    //sprintf(buff, " v = %8.2f vm= %8.2f index= %d \n", v, vm, index); 
                    //println(buff); 
                }
            if (verbose_alert) { print(str("on flag is: ")); println(on); }
        }

        //** check if the specific sensors is out of range 
        else if (sensors[index].value < v_min || sensors[index].value > v_max) on = true;
        else                                                                   on = false;
    }

    //** check if sensors are digital: on, off, open, close  
    else if (digital)
    {
        //** check if there is at least one sensor with the alert 
        if (name == "ANY")
        {
            for (i = 0; i < N_sensors; i++)
                if (sensors[i].magnitude == reference_group)
                {
                    check_sensors_value(sensors[i].value);
                    if (on) { index = i; break; }
                }
        }

        //** check the specific sensor
        else  check_sensors_value(sensors[index].value);

    }

}




void Alert::check_actuators(Actuator actuators[], short int N_actuators)
{

    short int i;


    //** check if there is at least one actuator with some alert 
    if (name == "ANY")
    {
        for (i = 0; i < N_actuators; i++)
            if (actuators[i].device == reference_group)
            {
                check_actuators_value(actuators[i].state);
                if (on) { index = i; break; }
            }
    }

    //** check the specific actuator
    else  check_actuators_value(actuators[index].state);


}


void Alert::write_email(char state[], Sensor sensors[], Actuator actuators[])
{
    char char_name[20], c1[7], c2[7], char_value[7];
    float v;
    short int i, j, k, m;

    //print(name); println(digital); 

    if (digital && name != "ANY")
    {
        strcat(state, "Actuator: ");
        name.toCharArray(char_name, 20);
        strcat(state, char_name);
        strcat(state, " has value: ");
        value.toCharArray(c1, 7);
        strcat(state, c1);

    }
    else if (!digital && name != "ANY")
    {
        strcat(state, "Sensor: ");
        name.toCharArray(char_name, 20);
        strcat(state, char_name);
        strcat(state, " has value: ");
        sprintf(char_name, "  %6.2f", sensors[index].value);
        strcat(state, char_name);

    }
    else if (name == "ANY" && !digital)
    {
        m = index_reference_group;
        for (k = 0; k < RG[m].N; k++)
        {
            j = RG[m].index[k];
            v = sensors[j].value;
            if (v < v_min || v > v_max)
            {
                sensors[j].label.toCharArray(char_name, 20);
                strcat(state, char_name);
                strcat(state, " = ");
                sprintf(char_name, "  %6.2f", sensors[j].value);
                strcat(state, char_name);
                strcat(state, " \n");
            }
        }
        if (strcmp(state, "") != 0)
        {
            min_value.toCharArray(c1, 7);
            max_value.toCharArray(c2, 7);
            strcat(state, "Sensors are out of range : [ "); strcat(state, c1);
            strcat(state, " , "); strcat(state, c2); strcat(state, " ] \n");
        }

    }
    else if (name == "ANY" && digital)
    {
        m = index_reference_group;
        for (k = 0; k < RG[m].N; k++)
        {
            j = RG[m].index[k];
            v = sensors[j].value;
            sensors[j].label.toCharArray(char_name, 20);
            strcat(state, char_name);
            if (v > 0.9) // OPEN or ON 
            {
                if (reference_group == "Door") strcat(state, " is OPEN \n");
                else strcat(state, " is ON \n");
                //sprintf(char_name, "  %6.2f", sensors[j].value);
                //strcat(state, char_name);
                //strcat(state, " \n");
            }
            else
            {
                if (reference_group == "Door") strcat(state, " is CLOSED \n");
                else strcat(state, " is OFF \n");
            }
        }
        if (strcmp(state, "") != 0)
        {
            time_begin.toCharArray(c1, 7);
            time_end.toCharArray(c2, 7);
            strcat(state, "Alert is ON, time period  : [ "); strcat(state, c1);
            strcat(state, " , "); strcat(state, c2); strcat(state, " ] \n");

        }

    }
}




























//***********************************************************************
//  Determine the number of groups                               
//***********************************************************************
void set_groups(Reference_group G[],  Sensor sensors[], short int N_sensors)
{
    short int i, j, k;

    for (i = 0; i < N_alerts; i++)
        if (alerts[i].reference_group != "")
        {
            for (j = i; j < N_alerts; j++)
            {
                if (G[j].name == "")
                {
                    G[j].name = alerts[i].reference_group;
                    G[j].is_sensor = alerts[i].is_sensor;
                    G[j].digital = alerts[i].digital;
                    G[j].index[0] = i;
                    G[j].N = 1;
                    N_groups++;
                    break;
                }
                else if (G[j].name == alerts[i].reference_group)
                {
                    k = G[j].N;
                    G[j].index[k] = i;
                    G[j].N++;
                    break;
                }
            }
        }


    // for (i = 0; i < N_groups; i++) { print(str("Group name ="));  println(G[i].name); }

 //**Groups with name which is a magnitude.
 //  These groups have every sensor with this magnitude 
    for (j = 0; j < N_groups; j++)
    {
        k = 0;
        for (i = 0; i < N_sensors; i++)
            if (sensors[i].magnitude == G[j].name)
            {
                G[j].index[k] = i;
                k++;
                //   print(i); print("  "); println(sensors[i].magnitude); 
            }

        if (k > 0)
        {
            G[j].N = k; G[j].any = true;
        }
        else  G[j].any = false;
    }


}


//***********************************************************************
//   Determine the number of conditions                             
//***********************************************************************
void set_conditions(Condition C[])
{
    short int i, j, k;

    for (i = 0; i < N_alerts; i++)
        if (alerts[i].condition != "")
        {
            for (j = 0; j < N_alerts; j++)
            {
                if (C[j].name == "")
                {
                    C[j].name = alerts[i].condition;
                    C[j].N = 1;
                    C[j].index[0] = i;
                    N_conditions++;
                    break;
                }
                else if (C[j].name == alerts[i].condition)
                {
                    k = C[j].N;
                    C[j].index[k] = i;
                    C[j].N++;
                    break;
                }
            }
        }

}




// **********************************************************************************************************
// Asign values to each Alert field
// **********************************************************************************************************
void alerts_setup(int& size, JSON_Row config[], Sensor sensors[], short int N_sensors, Actuator actuators[], short int N_actuators)
{
    short int i, j, k;
    Reference_group* mRG;
    Condition* mC;
    String reference_group;
    int memory;
    short int N_any;

    read_config_file("/config/alerts.ini", config, N_alerts);
    if (N_alerts > 0)
    {

        delete[] alerts;
        alerts = new Alert[N_alerts];

        memory = sizeof(Alert) * N_alerts;
        size += memory;

        print(str("N_alerts = "));  print(N_alerts);
        print(str("  memory = ")); print(memory); println(str(" bytes"));
    }


    N_groups = 0;
    N_conditions = 0;
    N_any = 0;

    for (i = 0; i < N_sensors; i++)
        if (sensors[i].magnitude == "Temperature") N_any++;


    mRG = new Reference_group[N_alerts];
    mC = new Condition[N_alerts];

    for (i = 0; i < N_alerts; i++)
    {

        mRG[i].name = "";
        //  mRG[i].index = new short int[N_alerts];
        mRG[i].index = new short int[N_any];

        mC[i].name = "";
        mC[i].index = new short int[N_alerts];

    }



    for (i = 0; i < N_alerts; i++)
    {
        alerts[i].name = config[i].c[0];
        alerts[i].min_value = config[i].c[1];
        alerts[i].max_value = config[i].c[2];
        alerts[i].value = config[i].c[3];
        alerts[i].reference_group = config[i].c[4];
        alerts[i].condition = config[i].c[5];
        alerts[i].time_begin = config[i].c[6];
        alerts[i].time_end = config[i].c[7];
        alerts[i].days = config[i].c[8];
        alerts[i].mode = config[i].c[9];
        alerts[i].actions = config[i].c[10];
        alerts[i].report = config[i].c[11];



        for (j = 0; j < N_sensors; j++)
            if (alerts[i].name == sensors[j].label)
            {
                alerts[i].is_sensor = true;
                alerts[i].index = j;
                alerts[i].magnitude = sensors[j].magnitude;
                // if min value is blank, then the sensors is digital
                if (alerts[i].value == "")               alerts[i].digital = false;
                else if (alerts[i].value == "NOTNORMAL") alerts[i].digital = false;
                else                                     alerts[i].digital = true;
            }

        for (j = 0; j < N_actuators; j++)
            if (alerts[i].name == actuators[j].label)
            {
                alerts[i].is_sensor = false;
                alerts[i].digital = true;      // an actuator is always digital 
                alerts[i].index = j;
                alerts[i].device = actuators[j].device;
            }

        if (alerts[i].name == "ANY")
        {
            reference_group = alerts[i].reference_group;
            //	  alerts[i].magnitude = magnitude;

            if (alerts[i].value == "")
            {
                alerts[i].is_sensor = true; alerts[i].digital = false;
            }
            else
            {
                for (j = 0; j < N_sensors; j++)
                {
                    if (reference_group == sensors[j].magnitude)
                    {
                        if (alerts[i].value == "NOTNORMAL")
                        {
                            alerts[i].is_sensor = true; alerts[i].digital = false;
                        }
                        else
                        {
                            alerts[i].is_sensor = true; alerts[i].digital = true;
                        }
                        break;
                    }
                }
                for (j = 0; j < N_actuators; j++)
                {
                    if (reference_group == actuators[j].device)
                    {
                        alerts[i].is_sensor = false; alerts[i].digital = true;
                    }
                }
            }
        }



    }

    set_groups(mRG, sensors, N_sensors);
    set_conditions(mC);

    RG = new Reference_group[N_groups];
    conditions = new Condition[N_conditions];

    for (j = 0; j < N_groups; j++)
    {
        RG[j].name = mRG[j].name;
        RG[j].is_sensor = mRG[j].is_sensor;
        RG[j].digital = mRG[j].digital;
        RG[j].N = mRG[j].N;
        RG[j].any = mRG[j].any;
        RG[j].index = new short int[RG[j].N];

        //     println(RG[j].name); 

        for (i = 0; i < RG[j].N; i++)
        {
            //          print(str("index =") );  println(mRG[j].index[i]); 
            RG[j].index[i] = mRG[j].index[i];
        }
    }

    memory = 0;
    for (j = 0; j < N_groups; j++)
    {
        /*       print(str("Reference group:") ); print(RG[j].name);
               print(str(" N sensors or actuators = ")); println(RG[j].N);*/
        for (i = 0; i < RG[j].N; i++)
        {
            //       print(str("  var index =") );  println(RG[j].index[i]); 
        }
        memory += sizeof(RG[j]);

    }
    print(str("Alerts: size of Reference Groups = ")); print(memory); println(str(" bytes"));
    size += memory;

    memory = 0;
    for (i = 0; i < N_conditions; i++)
    {
        conditions[i].name = mC[i].name;
        conditions[i].N = mC[i].N;
        conditions[i].index = new short int[conditions[i].N];
        for (j = 0; j < conditions[i].N; j++) conditions[i].index[j] = mC[i].index[j];
        memory += sizeof(conditions[j]);
    }
    print(str("Alerts: size of Conditions = ")); print(memory); println(str(" bytes"));
    size += memory;

    // for (j = 0; j < N_groups; j++) { print(str("RG j:")); println(RG[j].name); }
   //  println("enter alerts ini"); 
    for (i = 0; i < N_alerts; i++) alerts[i].ini();


    //    if (N_groups > 0) { print(str("Number of reference groups = ")); println(N_groups); } 
    for (j = 0; j < N_groups; j++)
    {
      //  print(str("Reference group=")); print(RG[j].name); print(str(" digital sensor=")); println(RG[j].digital);
        for (i = 0; i < RG[j].N; i++)
        {
            k = RG[j].index[i];
            if (RG[j].any)
            {
                //      print(str("  sensor =   ")); print(sensors[k].label); print(str("   ")); println(k);
            }
            else
            {
                //          print(str("  alert =   ")); print(alerts[k].name); print(str("   ")); println(alerts[k].index);
            }

        }
    }


    if (N_conditions > 0) { print(str("Number of conditions = ")); println(N_conditions); }
    for (i = 0; i < N_conditions; i++)
    {
        //    println(conditions[i].name);
        for (j = 0; j < conditions[i].N; j++)
        {
            k = conditions[i].index[j];
            //         print(str("   ")); 
            //         println(alerts[k].name); print(str("   ")); println(alerts[k].index);
        }
    }



}




void Alert :: ini()
{
    short int j; 
    
    if (time_begin != "")
    {
       Time_to_decimal( time_begin, t_begin );
	   Time_to_decimal( time_end, t_end );
    }
	

    if (min_value != "")
    {
        String_to_float(max_value, v_max);
        String_to_float(min_value, v_min);
    }
    

 //   for (j = 0; j < N_groups; j++) { print(str("RG j:")); println(RG[j].name); }

 //   print(str("N_groups = ")); println(N_groups);
    for (j=0; j<N_groups; j++)
    {
    //    print(str("RG[j].name = ")); println(RG[j].name);
   //     print(str("reference_group = ")); println(reference_group);
        if (RG[j].name == reference_group)
        {
            index_reference_group = j;
   //         print(str("index_reference_group = ")); println(index_reference_group);
        }
    //    break;
    }

  //  reset_values(); 
}



//short int Alert :: sound_priority(String name)
//{
//    short int i;
//
//  //  print(str(" actions =")); print(actions);
//  //  print(str(" mode =")); println(mode); 
//
//    if (on && (actions == name || actions == "all"))
//
//        return get_level();
//
//    else return 0;
//
//}




//short int Alert :: get_level()
//{
//    
//    if      (reference_group == "Temperature")    return MEDIUM;
//    else if (reference_group == "Humidity")		  return MEDIUM;
//    else if (reference_group == "Power AC")		  return LOW;
//    else if (reference_group == "Volts AC")		  return LOW;
//    else if (reference_group == "I_rms AC")		  return LOW;
//    else if (reference_group == "kWh")			  return LOW;
//    else if (reference_group == "Pyranometer")    return MEDIUM;
//    else if (reference_group == "Irradiance")	  return MEDIUM;
//    else if (reference_group == "DC Power")		  return LOW;
//    else if (reference_group == "DC Current")	  return LOW;
//    else if (reference_group == "DC Voltage")	  return LOW;
//    else if (reference_group == "Battery")		  return HIGH;
//    else if (reference_group == "Push Button")	  return HIGH;
//    else if (reference_group == "Flow meter")	  return MEDIUM;
//    else if (reference_group == "Water counter")  return MEDIUM;
//    else if (reference_group == "Alarm")		  return HIGH;
//    else if (reference_group == "Door")			  return HIGH;
//    else if (reference_group == "AtenTTo aerial") return VERY_HIGH;
//    else if (reference_group == "AtenTTo plug")	  return VERY_HIGH;
//    else if (reference_group == "AtenTTo switch") return VERY_HIGH;
//    else if (reference_group == "Blind")		  return VERY_HIGH;
//    else if (reference_group == "Heat/Cool mode") return VERY_HIGH;
//    else if (reference_group == "Heater")		  return VERY_HIGH;
//    else if (reference_group == "Pump")			  return VERY_HIGH;
//    else if (reference_group == "Light")		  return VERY_HIGH;
//    else if (reference_group == "Taisi")		  return VERY_HIGH;
//
//}
//




// **********************************************************************************************************
// Benchmark to test alerts 
// **********************************************************************************************************
void test_alerts_and_reports()
{
    short int N_alerts = 2;
    short int N_sensors = 8;
    short int N_actuators = 0;
    char facility[40] = "Test_alerts";

    Alert* alerts = NULL;
    JSON_Row* config = NULL;
    Sensor* sensors = NULL;
    Actuator* actuators = NULL;


    alerts = new Alert[N_alerts];
    config = new JSON_Row[N_alerts];
    sensors = new Sensor[N_sensors];
    actuators = new Actuator[N_actuators];

    short int level;
    Time_Date time;
    float DT;
    char message[100], state[2000];
    short int i;
    int size;

    // sensors.ini 
    sensors[0].value = 300;
    sensors[0].label = "W_ext";
    sensors[0].type = "Analog";
    sensors[0].magnitude = "Power";
    sensors[0].address = "A5";

    sensors[3].value = 700;
    sensors[3].label = "W_int";
    sensors[3].type = "Analog";
    sensors[3].magnitude = "Power";
    sensors[3].address = "A6";

    sensors[1].value = 30;
    sensors[1].label = "kwhExt";
    sensors[1].type = "Analog";
    sensors[1].magnitude = "kWh";
    sensors[1].address = "A5";

    sensors[2].value = 1;
    sensors[2].label = "GarageDoor";
    sensors[2].type = "Digital";
    sensors[2].magnitude = "Door";
    sensors[2].address = "D50";

    sensors[4].value = -4.;
    sensors[4].label = "T_ext";
    sensors[4].type = "OneWire";
    sensors[4].magnitude = "Temperature";
    sensors[4].address = "D41:28-15-F9-A-3-0-0-4";

    sensors[5].value = 45.;
    sensors[5].label = "T_int";
    sensors[5].type = "OneWire";
    sensors[5].magnitude = "Temperature";
    sensors[5].address = "D41:28-15-F9-A-3-0-0-4";

    sensors[6].value = 35.;
    sensors[6].label = "T_wall";
    sensors[6].type = "OneWire";
    sensors[6].magnitude = "Temperature";
    sensors[6].address = "D41:28-15-F9-A-3-0-0-4";

    sensors[7].value = 0.;
    sensors[7].label = "Differential";
    sensors[7].type = "Digital";
    sensors[7].magnitude = "VAC";
    sensors[7].address = "OC2";


    //// alerts.ini 
    //i = 0;
    //config[i].c[0] = "Differential";
    //config[i].c[1] = ""; // min value 
    //config[i].c[2] = "";  // max value ;
    //config[i].c[3] = "OFF";
    //config[i].c[4] = ""; // reference group 
    //config[i].c[5] = "C2"; // condition 
    //config[i].c[6] = ""; // begin time
    //config[i].c[7] = ""; // end time
    //config[i].c[8] = "Daily"; // periodicity 
    //config[i].c[9] = "AUTO";  // mode
    //config[i].c[10] = "All"; // actions;
    //config[i].c[11] = "None"; // periodicity report


    ////config[i].c[0] = "W_ext";
    ////config[i].c[1] = "0"; // min value 
    ////config[i].c[2] = "200";  // max value ;
    ////config[i].c[3] = "";
    ////config[i].c[4] = "PowerDemand"; // reference group 
    ////config[i].c[5] = "C2"; // condition 
    ////config[i].c[6] = ""; // begin time
    ////config[i].c[7] = ""; // end time
    ////config[i].c[8] = "Daily"; // periodicity 
    ////config[i].c[9] = "AUTO";  // mode
    ////config[i].c[10] = "All"; // actions;
    ////config[i].c[11] = "Daily"; // periodicity report

    ////i++; 
    ////config[i].c[0] = "W_int";
    ////config[i].c[1] = "0"; // min value 
    ////config[i].c[2] = "500";  // max value ;
    ////config[i].c[3] = "";
    ////config[i].c[4] = "PowerDemand"; // reference group 
    ////config[i].c[5] = "C2"; // condition 
    ////config[i].c[6] = ""; // begin time
    ////config[i].c[7] = ""; // end time
    ////config[i].c[8] = "Daily"; // periodicity 
    ////config[i].c[9] = "AUTO";  // mode
    ////config[i].c[10] = "All"; // actions;
    ////config[i].c[11] = "Daily"; // periodicity report 

    ////i++; 
    ////config[i].c[0] = "GarageDoor";
    ////config[i].c[1] = "";
    ////config[i].c[2] = "";
    ////config[i].c[3] = "OPEN";
    ////config[i].c[4] = "D1";
    ////config[i].c[5] = "C3";
    ////config[i].c[6] = "00:00";
    ////config[i].c[7] = "08:00";
    ////config[i].c[8] = "Daily";
    ////config[i].c[9] = "AUTO";
    ////config[i].c[10] = "All";
    ////config[i].c[11] = "Daily";


    ////i++; 
    ////config[i].c[0] = "GarageDoor";
    ////config[i].c[1] = "";
    ////config[i].c[2] = "";
    ////config[i].c[3] = "OPEN";
    ////config[i].c[4] = "D1";
    ////config[i].c[5] = "C1";
    ////config[i].c[6] = "";
    ////config[i].c[7] = "";
    ////config[i].c[8] = "Daily";
    ////config[i].c[9] = "AUTO";
    ////config[i].c[10] = "All";
    ////config[i].c[11] = "Daily";

    //i++;
    //config[i].c[0] = "T_ext";
    //config[i].c[1] = "10";
    //config[i].c[2] = "20";
    //config[i].c[3] = "";
    //config[i].c[4] = "D1";
    //config[i].c[5] = "C1";
    //config[i].c[6] = "";
    //config[i].c[7] = "";
    //config[i].c[8] = "Daily";
    //config[i].c[9] = "AUTO";
    //config[i].c[10] = "All";
    //config[i].c[11] = "Daily";

    //// i++; 
    // // only T_int and T_ext are out of range. T_wall remains at 35
    // //config[i].c[0] = "ANY";
    // //config[i].c[1] = "0";
    // //config[i].c[2] = "40";
    // //config[i].c[3] = "";
    // //config[i].c[4] = "Temperature";     // reference group 
    // //config[i].c[5] = "ANY_Temperature"; // condition 
    // //config[i].c[6] = "";
    // //config[i].c[7] = "";
    // //config[i].c[8] = "Daily";
    // //config[i].c[9] = "AUTO";
    // //config[i].c[10] = "All";
    // //config[i].c[11] = "Daily";

    //// i++; 
    ////config[i].c[0] = "ANY";
    ////config[i].c[1] = "";
    ////config[i].c[2] = "";
    ////config[i].c[3] = "OPEN";
    ////config[i].c[4] = "Door";
    ////config[i].c[5] = "ANY_Door";
    ////config[i].c[6] = "00:00";
    ////config[i].c[7] = "08:00";
    ////config[i].c[8] = "Daily";
    ////config[i].c[9] = "AUTO";
    ////config[i].c[10] = "All";
    ////config[i].c[11] = "Daily";


    time.hour = 5;
    time.minute = 0;
    time.second = 0;

    time.dayOfWeek = 2;
    time.dayOfMonth = 14;
    time.month = 7;
    time.year = 2020;

    println("**********BEGIN of alert test");
    alerts_setup(size, config, sensors, N_sensors, actuators, N_actuators);
    println( str("alerts are  set") );

    DT = 10; 
    while (1)
    {
        time.minute = time.minute + DT;
        if (time.minute >= 60)
        {
            time.minute = 0;
            time.hour += 1;
        }

        sensors[0].value += 10.; // W_ext 
        sensors[1].value = 0.01; // kwh 
        sensors[2].value = 1; // GarageDoor
        sensors[3].value += 20.; // W_int 
        sensors[4].value += 0.1; // T_ext 
        sensors[5].value += -0.1; // T_int 
        if (time.hour> 7) sensors[2].value = 0; // Close Door 

        level = verify_alerts(facility, time, N_sensors, sensors, N_actuators, actuators, "AUTO", message, state);


        if (strcmp(state, "") != 0)
        {
            println(str("email alert: "));
            println(message);
            println(state);
            println(str("end email alert"));
        }

        print(str("hour =")); print((int)time.hour);
        print(str(" minute =")); print((int)time.minute);
        //print(str(" second =")); println((int)time.second);
        print(str("  T_int =")); print(sensors[5].value);
        print(str("  T_ext =")); println(sensors[4].value);
        delay(10000);
        watchdog_restart();


    }
}



