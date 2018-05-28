//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <TPixy2.h>

#define PID_MAX_INTEGRAL         2000
#define ZUMO_BASE_DEADBAND       20

class PIDLoop
{
public:
  PIDLoop(int32_t pgain, int32_t igain, int32_t dgain, bool servo)
  {
    m_pgain = pgain;
    m_igain = igain;
    m_dgain = dgain;
    m_servo = servo;

    reset();
  }

  void reset()
  {
    if (m_servo)
      m_command = PIXY_RCS_CENTER_POS;
    else
      m_command = 0;
      
    m_integral = 0;
    m_prevError = 0x80000000L;
  }  
  
  void update(int32_t error)
  {
    int32_t pid;
  
    if (m_prevError!=0x80000000L)
    { 
      // integrate integral
      m_integral += error;
      // bound the integral
      if (m_integral>PID_MAX_INTEGRAL)
        m_integral = PID_MAX_INTEGRAL;
      else if (m_integral<-PID_MAX_INTEGRAL)
        m_integral = -PID_MAX_INTEGRAL;

      // calculate PID term
      pid = (error*m_pgain + ((m_integral*m_igain)>>4) + (error - m_prevError)*m_dgain)>>10;
    
      if (m_servo)
      {
        m_command += pid; // since servo is a position device, we integrate the pid term
        if (m_command>PIXY_RCS_MAX_POS) 
          m_command = PIXY_RCS_MAX_POS; 
        else if (m_command<PIXY_RCS_MIN_POS) 
          m_command = PIXY_RCS_MIN_POS;
      }
      else
      {
        // Deal with Zumo base deadband
        if (pid>0)
          pid += ZUMO_BASE_DEADBAND;
        else if (pid<0)
          pid -= ZUMO_BASE_DEADBAND;
         m_command = pid; // Zumo base is velocity device, use the pid term directly  
      }
    }

    // retain the previous error val so we can calc the derivative
    m_prevError = error; 
  }

  int32_t m_command; 

private: 
  int32_t m_pgain;
  int32_t m_igain;
  int32_t m_dgain;
  
  int32_t m_prevError;
  int32_t m_integral;
  bool m_servo;
};

