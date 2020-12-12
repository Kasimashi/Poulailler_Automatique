
/* L'alimentation se fait en 3.3V et 5V. */

// Les connections suivantes doivent être effectuées DS1302.
// DS1302 patte RST  -> Arduino Digital 2
// DS1302 patte DATA -> Arduino Digital 3
// DS1302 patte CLK  -> Arduino Digital 4

#include <Arduino.h>
#include "Timer2.h" // inclusion de la librairie Timer2
#include "RTCDS1302.h"
#include "LiquidCrystal.h"
#include "Poulailler.h"

//DEFINITION DES PORTS

#define	PORT_FIN_DE_COURSE_HAUT	8 //Port qui accepte les interruptions
#define	PORT_FIN_DE_COURSE_BAS	9 //Port qui accepte les interruptions
#define	TEMOIN_MOTEUR_MOUVEMENT	10 //LED de retour d'état du mouvement de la porte
#define TEMOIN_ETAT_PORTE	PD5 //LED de retour d'état de la porte
#define	PORT_PROJECTEUR	PD7 //Port qui est branché au niveau de la LED projecteur.
#define PORT_LECTURE_LUMINOSITE A0 //Port de lecture de l'intensité lumineuse

// DS1302(uint8_t ce_pin, uint8_t data_pin, uint8_t sclk_pin)
#define PORT_RTC_RST 2 //PD2
#define PORT_RTC_DATA 3 //PD3
#define PORT_RTC_SCLK 4 //PD4

#define PORT_LCD_D7 PB5 //Port LCD D7 //D13
#define PORT_LCD_D6 PB4 //Port LCD D6 //D12
#define PORT_LCD_D5 PB3 //Port LCD D5 //D11
#define PORT_LCD_D4 PB2 //Port LCD D4 //D10
#define PORT_LCD_RS PB1 //Port LCD RS //D9
#define PORT_LCD_ENABLE PB0 //Port LCD ENABLE //D8
//MODE CAPTEUR DE LUMIERE
#define SEUIL_CAPTEUR_PHOTORESISTANCE 300 //Seuil de déclenchement
#define TAILLE_TABLEAU_MOY 10 // Nombre de valeur dans le tableau
#define FREQ_MESURE 1000*20 // fréquence de mesure de l'intensité lumineuse : 20sec


//MODE RTC

//MODE COMMUM
#define PRISE_DE_DECISION_ALLUMAGE_LUMIERE 5 //Nombre de tick avant l'allumage du projecteur.
#define NB_C_HORLOGE_AVANT_OP 10 //Nombre de tick avant l'opération (Nombre de tick total : ALLUMAGE + NB_C_HORLOGE_AVANT_OP
#define DELAIS_TICK_MOUVEMENT 1000*20 //20 secondes //délais d'attente avant opération

//Initialisation des variables
//Capteur photosensible
int tableau_intensite[TAILLE_TABLEAU_MOY] = {0}; //initialisitation du tableau à porte ouverte
int index_intensite =0, nb_mesures =0, tick_changement_detat_jour=0, tick_changement_detat_nuit=0;
int moyenne;

//Variables commune.
int Ouvert; //Etat de la porte

// Init DS1302

DS1302 rtc(2, 3, 4);

// Init structure Time-data
Time t;

// Init LiquidCrystal Port
LiquidCrystal lcd(PORT_LCD_RS, PORT_LCD_ENABLE, PORT_LCD_D4, PORT_LCD_D5, PORT_LCD_D6, PORT_LCD_D7);

// the setup routine runs once when you press reset:
void setup() {
  MsTimer2::set(FREQ_MESURE, InterruptTimer2); // Timer2 Interne Arduino : période 5000ms
  MsTimer2::start(); // active Timer 2

  // Positionnement horloge a run-mode et desactive protection en ecriture
  rtc.halt(false);
  rtc.writeProtect(false);

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");

  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_HAUT), FinDeCourseHaut, RISING); //Interuption capteur fin de course haut
  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_BAS), FinDeCourseBas, RISING); //Interuption capteur fin de course bas

  setup_rtc();

  Ouvrir(); //etat de la porte au départ.
}

void setup_rtc(){
// Initialisation de l'horloge
  // A mettre a jour avec les bones valeurs pour initialiser l horloge RTC DS1302
  rtc.setDOW(FRIDAY);        // Jour a FRIDAY
  rtc.setTime(19, 10, 0);    // Heure a 19:10:00 (format sur 24 heure)
  rtc.setDate(3, 6, 2016);   // Date  au 3 juin 2016
}
// the loop routine runs over and over again forever:
void loop() {
		/** OUVERTURE DE LA PORTE **/
		if (moyenne<SEUIL_CAPTEUR_PHOTORESISTANCE && !Ouvert){ //Le jour pointe le bout de son nez
			//On prévient les poules de l'iminmence de l'ouverture de porte
			if (!Ouvert && tick_changement_detat_jour >= PRISE_DE_DECISION_ALLUMAGE_LUMIERE){ //On allume la lumière 5 minutes plus tard après la detection
				digitalWrite(PORT_PROJECTEUR,1); //On prévient les poules !
			}
			tick_changement_detat_jour++;
			delay(DELAIS_TICK_MOUVEMENT);
			Serial.println(tick_changement_detat_jour);
			if (tick_changement_detat_jour >= NB_C_HORLOGE_AVANT_OP  + PRISE_DE_DECISION_ALLUMAGE_LUMIERE)
				{
					Ouvrir();
					tick_changement_detat_jour =0;
					digitalWrite(PORT_PROJECTEUR,0);
				}
		}
		if (moyenne>SEUIL_CAPTEUR_PHOTORESISTANCE && !Ouvert){ //Apparition d'un projecteur.
			digitalWrite(PORT_PROJECTEUR,0);
			tick_changement_detat_jour = 0;
		}
		/** FERMETURE DE LA PORTE **/
		if (moyenne>SEUIL_CAPTEUR_PHOTORESISTANCE && Ouvert){//Il commence à faire nuit
			if (Ouvert && tick_changement_detat_nuit >= PRISE_DE_DECISION_ALLUMAGE_LUMIERE){//On allume la lumière 5 minutes plus tard après la detection
				digitalWrite(PORT_PROJECTEUR,1);
			}
			tick_changement_detat_nuit++;
			delay(DELAIS_TICK_MOUVEMENT);
			Serial.println(tick_changement_detat_nuit);
			if (tick_changement_detat_nuit >= NB_C_HORLOGE_AVANT_OP +PRISE_DE_DECISION_ALLUMAGE_LUMIERE)
			{
				Fermer();
				tick_changement_detat_nuit =0;
				digitalWrite(PORT_PROJECTEUR,0);
			}
		}
		if (moyenne<SEUIL_CAPTEUR_PHOTORESISTANCE && Ouvert){ // Apparition d'un nuage
			digitalWrite(PORT_PROJECTEUR,0);
			tick_changement_detat_nuit = 0; //On recommence.
		}

		/**** AFFICHAGE DE l'HEURE **/
	  // recup donnees DS1302
	  t = rtc.getTime();

	  // Ecriture date sur console serie
	  Serial.print("Jour : ");
	  Serial.print(t.date, DEC);
	  Serial.print(" - Mois : ");
	  Serial.print(rtc.getMonthStr());
	  Serial.print(" - Annee : ");
	  Serial.print(t.year, DEC);
	  Serial.println(" -");

	  // Ecriture heure sur console serie
	  Serial.print("C est le ");
	  Serial.print(t.dow, DEC);
	  Serial.print(" ieme jour de la semaine (avec lundi le premier), et il est ");
	  Serial.print(t.hour, DEC);
	  Serial.print(" heures, ");
	  Serial.print(t.min, DEC);
	  Serial.print(" minutes ");
	  Serial.print(t.sec, DEC);
	  Serial.println(" secondes.");

	  // Affichage d un separateur
	  Serial.println("------------------------------------------");

	  // Attente d une seconde avant lecture suivante :)
	  delay (1000);
}

void Ouvrir(){
	Serial.println("Opening");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,1);
	while (digitalRead(PORT_FIN_DE_COURSE_HAUT)!=0){ //La porte n'a pas atteint le haut
		Serial.println("Ouverture ..........");
	}
	Serial.println("OUVEEEEEEEEEERT !!!");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,0);
	Ouvert = 1;
	digitalWrite(TEMOIN_ETAT_PORTE,Ouvert);
}

void Fermer(){
	Serial.println("Closing");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,1);
	while (digitalRead(PORT_FIN_DE_COURSE_BAS)!=0){ //La porte n'a pas atteint le bas
		Serial.println("Fermeture ..........");
	}
	Serial.println("FERMEEEEEEEEE !!!!!!");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,0);
	Ouvert = 0;
	digitalWrite(TEMOIN_ETAT_PORTE,Ouvert);
}

void InterruptTimer2() { // debut de la fonction d'interruption Timer2

	//Lecture de la valeur de luminosité.
	Serial.println("Mesure en cours ... !");
	unsigned int read = analogRead(PORT_LECTURE_LUMINOSITE);
	tableau_intensite[index_intensite%10] = read;
	index_intensite++;
	nb_mesures++;
	if (nb_mesures>TAILLE_TABLEAU_MOY){ //Si on a assez d'étantillons
			moyenne = moyenneTableau(tableau_intensite); //On calcul la moyenne
			Serial.println(moyenne);

	}
}

int moyenneTableau(int tableau[])
{
    int i = 0, somme = 0, moyenne = 0;
    for(i = 0; i < TAILLE_TABLEAU_MOY; i++)
    {
        somme += tableau[i];
    }
    moyenne = somme / TAILLE_TABLEAU_MOY;
    return moyenne;
}

void FinDeCourseHaut(){
	Serial.println("Capteur de fin de course haut atteint ! (Porte ouverte)"); //DEBUG

}

void FinDeCourseBas(){
	Serial.println("Capteur de fin de course bas atteint ! (Porte fermée)"); //DEBUG

}
