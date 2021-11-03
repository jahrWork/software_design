
#include "misc.h"
#include "print_debug.h"
#include "RTC_SAM3X.h"
#include "JSON_tools.h"
#include "timers.h"




float Latitude  = 40.416775; 
float Longitude = -3.703790; // Madrid
#define PI 3.14159265 

boolean verboset = false; //true; //




//***********************************************************************
//
//***********************************************************************
void timers_setup( Timer timers[],  short int N_timers, JSON_Row config[] )
{
   
   short int N_c; 
   short int i, j; 

  
      for( i = 0; i<N_timers; i++) 
      {
          timers[i].name    = config[i].c[0];
          timers[i].begin   = config[i].c[1];
          timers[i].end     = config[i].c[2];
          timers[i].days    = config[i].c[3];
          timers[i].mode    = config[i].c[4];

          timers[i].on = false;

          timers[i].setup( );  
           
      } 


}




//***************************************************************************
//
//***************************************************************************
void Timer :: setup( )
{
    long r = random(0, 100); 
    
    if       (begin == "Sunrise") t_begin = -1.0; 
    else if  (begin == "Sunset" ) t_begin = 25.0; 
    else if  (begin == "Simul")   t_begin = 7.0 + 13. * (float)r / 100.; 
    else         Time_to_decimal( begin, t_begin); 
    
    if       (end == "Sunrise") t_end = -1.0; 
    else if  (end == "Sunset" ) t_end = 25.0;
    else if  (end == "Simul")   t_end = 7.0 + 13. * (float)r / 100. + 2./60.;
    else       Time_to_decimal( end, t_end); 
}

//******************************************************************************
//
//******************************************************************************
int isLeapYear(int year)
{

    int is;

    if (year % 4 == 0)        // Divisible by 4
    {
        if (year % 100 == 0)  // Divisible by 100
        {

            if (year % 400 == 0) // Divisible by 400
            {
                is = 1;
            }
            else                // Not divisible by 400
            {
                is = 0;
            }
        }
        else // Not divisible by 100
        {
            is = 1;
        }
    }
    else  // Not divisible by 4
    {
        is = 0;
    }

    return is;

}


//******************************************************************************
//
//******************************************************************************
int  days_in_month(int month, int year)
{

    int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int days_month; 

    if (month == 2)
    {
        days_month = days[month-1] + isLeapYear(year);
    }
    else if (month <= 12)
    {
        days_month = days[month-1]; 
    }
    else
    {

    }
    return days_month; 

}


//***************************************************************************************************************
//
//***************************************************************************************************************
float hour_angle(struct Time_Date timedate)
{
    float delta, cosh; 
    float days; 
    float deg2rad = PI / 180; 
    float rad2deg = 180 / PI; 
    int i; 

    days = 0.; 
    for (i = 1; i < timedate.month; i++)
    {
        days +=  days_in_month(i, timedate.year); 
    }
    days += timedate.dayOfMonth; 

//  Serial.print(str(" month =")); Serial.println(timedate.month);
//  Serial.print(str(" days =")); Serial.println(days); 

    delta = 23.45 * sin( 360 * (days - 81) / 365 * deg2rad ); 

    cosh = -tan( Latitude * deg2rad ) * tan( delta  * deg2rad ); 

    return acos(cosh) * rad2deg;    

}

//***************************************************************************************************************
//
//***************************************************************************************************************
float Central_European_Time(struct Time_Date timedate)
{
    float CET; 
    
    if (timedate.month == 3)
    {
        if      (timedate.dayOfWeek < 7  && timedate.dayOfMonth + 7 > 31)    CET = 2;
        else if (timedate.dayOfWeek < 7  && timedate.dayOfMonth + 7 < 31)    CET = 1;
        else if (timedate.dayOfWeek == 7 && timedate.dayOfMonth + 7 > 31)    CET = 2;
        else if (timedate.dayOfWeek == 7 && timedate.dayOfMonth + 7 < 31)    CET = 1;
        else   println( str("ERROR CET ") );
    }
    else if (timedate.month == 10)
       {
        if      (timedate.dayOfWeek < 7 && timedate.dayOfMonth + 7 > 31)    CET = 1;
        else if (timedate.dayOfWeek < 7 && timedate.dayOfMonth + 7 < 31)    CET = 2;
        else if (timedate.dayOfWeek == 7 && timedate.dayOfMonth + 7 > 31)   CET = 1;
        else if (timedate.dayOfWeek == 7 && timedate.dayOfMonth + 7 < 31)   CET = 2;
        else   println( str("ERROR CET ") );                                                 
       }
    else if (timedate.month < 3 || timedate.month >10)  CET = 1;
    else                                                CET = 2; 
    
    if (verboset) 
    {
        print(str("CET = ")); println(CET);
    }

    return CET; // summertime +2, wintertime +1 
    
}


//***************************************************************************************************************
//
//***************************************************************************************************************
float Sunrise(struct Time_Date timedate)
{
    return 12. - hour_angle(timedate) / 15. - Longitude / 15. + Central_European_Time(timedate) - 0.5;
}

//***************************************************************************************************************
//
//***************************************************************************************************************
float Sunset(struct Time_Date timedate)
{
  //  return 12. + hour_angle(timedate) / 15. - Longitude / 15. + Central_European_Time(timedate) + 1; // sunset+1 means no light 
   return 12. + hour_angle(timedate) / 15. - Longitude / 15. + Central_European_Time(timedate) + 0.30; // sunset+1/2 hour after real testing (JAHR 2/07/2020)
}



//***********************************************************************
//
//***********************************************************************
short int Timer :: check(struct Time_Date timedate)
{
        float dec_hour;
        short int iday, iday_of_week; 
        String periodicity[10] = { "Daily", "Mon-Fri", "Sat-Sun",  "Mon", "Tue", "Wed",  "Thu", "Fri", "Sat", "Sun" }; 
        float tb, te, t; 
       
        iday = find_index( days, periodicity, 10) - 2; 
        iday_of_week = timedate.dayOfWeek;
      

        dec_hour = timedate.hour + timedate.minute/60. + timedate.second / 3600.;
       
        if (iday < -2 || iday>7) { print( str(" ERROR Timer::check  iday =") );  println(iday); }
       
        if      (mode == "Disable")                        enforceable = false;
        else if ( days == "Daily")                         enforceable = true; 
        else if ( days == "Mon-Fri" && iday_of_week <= 5 ) enforceable = true; 
        else if ( days == "Sat-Sun" && iday_of_week >= 6 ) enforceable = true; 
        else if ( iday == iday_of_week )                   enforceable = true; 
        else                                               enforceable = false; 
        
      

       

        if (enforceable)
        {
          // set sunset and sunrise for everyday 
          // sunrise time=-1; sunset time =25  
          if      (t_begin < 0.0)   t_begin = Sunrise(timedate);
          else if (t_begin > 24.0)  t_begin = Sunset(timedate);
          
          if      (t_end < 0.0)   t_end = Sunrise(timedate) + 2./60.;
          else if (t_end > 24.0)  t_end = Sunset(timedate)  + 2./60.; 

          if (t_end < t_begin)  // e.g. lights ON from 21:00  to 02:00
                                // e.g. lights ON from Sunset to 01:00 
          { 
              tb = t_begin; te = t_end + 24;
              if (dec_hour < t_end) t = dec_hour + 24; 
              else                  t = dec_hour; 
          }
          else                 // e.g. lights ON from 22:00  to 23:00
          { 
              tb = t_begin; te = t_end; t = dec_hour; 
          }

          
          
           if ( mode == "On" ) 
           {
             if ( t >= tb  && t <= te ) { on = true;  state = 1; } 
             else                       { on = false; state = 0; } // JAHR 17/04/11, 25/10/16
           }
           
           else if  ( mode == "Off" )
           {
             if ( t >= tb  && t <= te ) { on = false; state = 0; } 
             else                         enforceable = false; // 8/6/2017 on = true; // JAHR 17/04/17
           }

          else if ( mode == "Up")
          {
            if (t >= tb  && t <= te) { on = true;  state = 1; } 
            else                                             enforceable = false;
          }

          else if (mode == "Down")
          {
            if (t >= tb  && t <= te) { on = true; state = 2; } 
            else                       enforceable = false;
          }

          else if (mode == "Middle_Down")
          {
               te = tb + 5. / 3600.; 
              if (t >= tb && t <= te) { on = true; state = 2; } // only 5 seconds going down 
                                        enforceable = false;
          }

          else if (mode == "Middle_Up")
          {
               te = tb + 5. / 3600.; 
              if (t >= tb && t <= te) { on = true; state = 2; } // only 5 seconds going up  
              else                      enforceable = false;
          }

          
        }  

        else // not enforceable 
        {
            if (mode == "On")
            {
                enforceable = true; 
                on = false; // e.g. if the timer is On trough Mon-Fri in some hours, then the rest is off
            }

            else if (mode == "Up" || mode == "Down" || mode == "Middle_Down" || mode == "Middle_Up")
            {
              enforceable = true; state = 0;
              on = false; // up and down orders for blinds after 1 minute or 5 seconds should return to state=0
            }

          
        }
             
   
        
        if (verboset)
        {
         print( str("          Timer=") ); print( name ); 
         print( str(" enforceable=") ); print( enforceable );  
         print(str(" on=") ); println((bool)on);
       
         print( str("          periodicity=") ); print( days ); print( str(" iday=") ); println( iday );
         print( str("          hour=") ); print(dec_hour); print( str(" begin:end ") );  
         print( t_begin );  print( str(":") ); println( t_end);
         println( str("_________________________________________") );
        }
        
        //  
      //  Serial.print(str("iday_of_week ="));   Serial.println(iday_of_week);
        //print(str(" Timer = ")); print(name);
        //print(str(" enforceable by days = ")); println(enforceable);

        //else if (enforceable && mode == "Sunrise") 
        //{
            //hour = Sunrise(timedate);
            //tbegin = hour + -2. / 60.;
            //tend = hour + 2. / 60.;
            //if (verboset) { Serial.print(str("           Sunrise ="));  Serial.println(tbegin); }
            //if (dec_hour >= tbegin  && dec_hour <= tend) on = true; 
            //else enforceable = false;     
        //}

        //else if (enforceable && mode == "Sunset") 
        //{
            //hour = Sunset(timedate);
            //tbegin = hour + 0.5 - 2. / 60.; // half an hour after sunset, blinds are lowered 
            //tend = hour + 0.5 + 2. / 60.;
            //if (verboset) { Serial.print(str("           Sunset ="));  Serial.println(tbegin); }
                
            //if (dec_hour >= tbegin  && dec_hour <= tend) on = true; 
            //else enforceable = false;
        //}

       
        //else if (enforceable && mode == "Simul") 
        //{
            ////hour = Sunset(timedate);   // Sunset
            ////tbegin = hour + 0.5 - 2. / 60.; // half an hour after sunset, blinds are lowered 
            ////tend   = hour + 0.5 + 2. / 60.;
            ////Serial.print(str("Sunset ="));  Serial.println(tbegin);
            ////if (dec_hour >= tbegin  && dec_hour <= tend) on = true;
            ////else enforceable = false;
        //}

    //    if (!enforceable) on = false; 
       
   


 //   
   
  
      
}
