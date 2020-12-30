
/* L'alimentation se fait en 3.3V et 5V. */

// Les connections suivantes doivent être effectuées DS1302.
// DS1302 patte RST  -> Arduino Digital 2
// DS1302 patte DATA -> Arduino Digital 3
// DS1302 patte CLK  -> Arduino Digital 4

#include <Arduino.h>
#include <Time.h>
#include <stdio.h>
#include "lib/Timer2.h" // inclusion de la librairie Timer2
#include "lib/RTCDS1302.h"
#include "lib/LiquidCrystal.h"
#include "Poulailler.h"
#include "Shell/SimpleCLI.h"

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

int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
#define SECONDS_FROM_1970_TO_2000 946684800
//MODE RTC

//MODE COMMUM
#define PRISE_DE_DECISION_ALLUMAGE_LUMIERE 5 //Nombre de tick avant l'allumage du projecteur.
#define NB_C_HORLOGE_AVANT_OP 10 //Nombre de tick avant l'opération (Nombre de tick total : ALLUMAGE + NB_C_HORLOGE_AVANT_OP
#define DELAIS_TICK_MOUVEMENT 1000*20 //20 secondes //délais d'attente avant opération

//Initialisation des variables
//Capteur photosensible
int tableau_intensite[TAILLE_TABLEAU_MOY] = {0}; //initialisitation du tableau à porte ouverte
int index_intensite =0, nb_mesures =0, tick_changement_detat_jour=0, tick_changement_detat_nuit=0;
int moyenne=400;

//Variables commune.
int Ouvert; //Etat de la porte

// Init DS1302

DS1302 rtc(PORT_RTC_RST, PORT_RTC_DATA, PORT_RTC_SCLK);

// Init structure Time-data
Time t;

// Init LiquidCrystal Port
LiquidCrystal lcd(PORT_LCD_RS, PORT_LCD_ENABLE, PORT_LCD_D4, PORT_LCD_D5, PORT_LCD_D6, PORT_LCD_D7);

// Create CLI Object
SimpleCLI cli;

// Commands
Command cmdPing;
Command cmdMycommand;
Command cmdEcho;
Command cmdDate;
Command cmdRm;
Command cmdLs;
Command cmdBoundless;
Command cmdSingle;
Command cmdHelp;

// the setup routine runs once when you press reset:
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);


  MsTimer2::set(FREQ_MESURE, InterruptTimer2); // Timer2 Interne Arduino : période 5000ms
  MsTimer2::start(); // active Timer 2

  // set up the LCD's number of columns and rows:
  //lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.print("hello, world!");

  setup_rtc();

  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_HAUT), FinDeCourseHaut, RISING); //Interuption capteur fin de course haut
  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_BAS), FinDeCourseBas, RISING); //Interuption capteur fin de course bas

  setup_cmd();




  //Ouvrir(); //etat de la porte au départ.
}

void setup_cmd(void){
	Serial.println("Welcome to this simple Arduino command line interface (CLI).");
	Serial.println("Type \"help\" to see a list of commands.");

	cmdPing = cli.addCmd("ping");
	cmdPing.addArg("n", "10");
	cmdPing.setDescription("Responds with a ping n-times");

	cmdMycommand = cli.addCmd("mycommand");
	cmdMycommand.addArg("o");
	cmdMycommand.setDescription("Says hi to o");

	cmdEcho = cli.addCmd("echo");
	cmdEcho.addPosArg("text", "something");
	cmdEcho.setDescription("Echos what you said");

	cmdDate = cli.addCmd("date");
	cmdDate.setDescription("Get RTC of the system");

	cmdRm = cli.addCmd("rm");
	cmdRm.addPosArg("file");
	cmdRm.setDescription("Removes specified file (but not actually)");

	cmdLs = cli.addCmd("ls");
	cmdLs.addFlagArg("a");
	cmdLs.setDescription("Lists files in directory (-a for all)");

	cmdBoundless = cli.addBoundlessCmd("boundless");
	cmdBoundless.setDescription("A boundless command that echos your input");

	cmdSingle = cli.addSingleArgCmd("single");
	cmdSingle.setDescription("A single command that echos your input");

	cmdHelp = cli.addCommand("help");
	cmdHelp.setDescription("Get help!");
}
void setup_rtc(void){
  char sec;
  char min;
  char hours;

//setup rtc with compiler_time
  sscanf(__TIME__, "%02c:%02c:%02c",&hours,&min,&sec);
// Initialisation de l'horloge
	// Positionnement horloge a run-mode et desactive protection en ecriture
  rtc.halt(false);
  rtc.writeProtect(false);
  // A mettre a jour avec les bones valeurs pour initialiser l horloge RTC DS1302
  rtc.setTime(hours, min, sec);    // Heure a 19:10:00 (format sur 24 heure)
  rtc.setDate(30, 12, 2020);   // Date  au 3 juin 2016
}
// the loop routine runs over and over again forever:
void loop() {
	Shell();
	/** OUVERTURE DE LA PORTE **/
	/**
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
	*/
	/** FERMETURE DE LA PORTE **/
	/**
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
	**/
}

void Shell(void){
	String input ="";
		char ch="";
		Serial.print("# ");
		while ((ch!=13)){ //Tant que l'utilisateur n'a pas valid� ou n'a pas fait CTRL+C on lit.
			if (Serial.available() >0){ //Si on d�tecte une entr�e s�rie.
				ch = Serial.read(); //Lecture du caract�re d'entr�e.
				switch(ch){
					case(3): //CTRL+C
						return;
						break;
					case(13): //ENTER
						break;
					case(8593): //Fl�che du haut
						Serial.println("Fl�che du haut! ");
						break;
					default:
						Serial.print(ch);
						input += (char)ch;
				}

			}
		}
		Serial.println();
		String last_input = input;
		if (input.length() > 0) {
			cli.parse(input);
		}

		if (cli.available()) {
			Command c = cli.getCmd();

			int argNum = c.countArgs();

			Serial.print(">");
			Serial.print(c.getName());
			Serial.print(' ');

			for (int i = 0; i<argNum; ++i) {
				Argument arg = c.getArgument(i);
				// if(arg.isSet()) {
				Serial.print(arg.toString());
				Serial.print(' ');
				// }
			}

			Serial.println();

			if (c == cmdPing) {
				Serial.print(c.getArgument("n").getValue() + "x ");
				Serial.println("Pong!");
			} else if (c == cmdMycommand) {
				Serial.println("Hi " + c.getArgument("o").getValue());
			} else if (c == cmdDate) {
				getHoursRTC();
			} else if (c == cmdEcho) {
				Argument str = c.getArgument(0);
				Serial.println(str.getValue());
			} else if (c == cmdRm) {
				Serial.println("Remove directory " + c.getArgument(0).getValue());
			} else if (c == cmdLs) {
				Argument a   = c.getArgument("a");
				bool     set = a.isSet();
				if (a.isSet()) {
					Serial.println("Listing all directories");
				} else {
					Serial.println("Listing directories");
				}
			} else if (c == cmdBoundless) {
				Serial.print("Boundless: ");

				for (int i = 0; i<argNum; ++i) {
					Argument arg = c.getArgument(i);
					if (i>0) Serial.print(",");
					Serial.print("\"");
					Serial.print(arg.getValue());
					Serial.print("\"");
				}
			} else if (c == cmdSingle) {
				Serial.println("Single \"" + c.getArg(0).getValue() + "\"");
			} else if (c == cmdHelp) {
				Serial.println(cli.toString());
			}
		}

		if (cli.errored()) {
			CommandError cmdError = cli.getError();
			Serial.println();
			Serial.print("ERROR: ");
			Serial.println(cmdError.toString());

			if (cmdError.hasCommand()) {
				Serial.print("Did you mean \"");
				Serial.print(cmdError.getCommand().toString());
				Serial.println("\"?");
			}
		}
}
void Ouvrir(){
	Serial.println("Opening");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,1);
}

void Fermer(){
	Serial.println("Closing");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,1);
}

void InterruptTimer2() { // debut de la fonction d'interruption Timer2

	//Lecture de la valeur de luminosité.
	Serial.println("Mesure en cours ... !");
	unsigned int read = analogRead(PORT_LECTURE_LUMINOSITE);
	tableau_intensite[index_intensite%10] = read;
	index_intensite++;
	nb_mesures++;
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

void FinDeCourseHaut(void){
	Serial.println("Capteur de fin de course haut atteint ! (Porte ouverte)"); //DEBUG
	Serial.println("OUVEEEEEEEEEERT !!!");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,0);
	Ouvert = 1;
	digitalWrite(TEMOIN_ETAT_PORTE,Ouvert);

}

void FinDeCourseBas(void){
	Serial.println("Capteur de fin de course bas atteint ! (Porte fermée)"); //DEBUG
	Serial.println("FERMEEEEEEEEE !!!!!!");
	digitalWrite(TEMOIN_MOTEUR_MOUVEMENT,0);
	Ouvert = 0;
	digitalWrite(TEMOIN_ETAT_PORTE,Ouvert);

}
int getMoyenne(void){
	if (nb_mesures>TAILLE_TABLEAU_MOY){ //Si on a assez d'étantillons
				moyenne = moyenneTableau(tableau_intensite); //On calcul la moyenne
				Serial.println(moyenne);

	}
	return moyenne;
}
void getHoursRTC(void){
	  // recup donnees DS1302
	  t = rtc.getTime();

	  // Ecriture date sur console serie
	  Serial.print("Jour : ");
	  Serial.print(t.date, DEC);
	  Serial.print(" - Mois : ");
	  Serial.print(rtc.getMonthStr());
	  Serial.print(" - Annee : ");
	  Serial.println(t.year, DEC);

	  // Ecriture heure sur console serie
	  Serial.print("Heure : ");
	  Serial.print(t.hour, DEC);
	  Serial.print(":");
	  Serial.print(t.min, DEC);
	  Serial.print(":");
	  Serial.println(t.sec, DEC);

	  Serial.print("Etat : ");
	  if (Jour(moyenne)){
		  Serial.println("Jour");
	  }else{
		  Serial.println("Nuit");
	  }


	  // Affichage d un separateur
	  Serial.println("------------------------------------------");
}

/*
 *
 * Cette fonctionne d�termine s'il fait jour ou s'il fait nuit
 * Retourne 0 s'il fait nuit sinon retourne 1 /
 *
 */

int Jour(int moyenne){
	if (moyenne > SEUIL_CAPTEUR_PHOTORESISTANCE){
		return 0;
	}
	return 1;
}
