// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _Poulailler_H_
#define _Poulailler_H_
#include "Arduino.h"
//add your includes for the project Poulailler here


//end of add your includes here


//add your function definitions for the project Poulailler here
void setup();
void setup_rtc(void);
void setup_cmd(void);
void Ouvrir();
void Fermer();
void InterruptTimer2();
int moyenneTableau(int tableau[]);
void IsrFinDeCourseHaut(void);
void IsrFinDeCourseBas(void);
int GetDayState(int moyenne);
void getHoursRTC(void);
void Shell(void);
int getMoyenne(void);



//Do not add code below this line
#endif /* _Poulailler_H_ */
