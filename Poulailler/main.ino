
/*********************************/
/* L'alimentation se fait en 3.3V et 5V. */
/* 
/*********************************/
#include <MsTimer2.h> // inclusion de la librairie Timer2
#include <DS1307RTC.h> //inclusiondumodule RTC

//DEFINITION DES PORTS

#define	PORT_FIN_DE_COURSE_HAUT	PD2 //Port qui accepte les interruptions
#define	PORT_FIN_DE_COURSE_BAS	PD3 //Port qui accepte les interruptions
#define	TEMOIN_MOTEUR_MOUVEMENT	PD4 //LED de retour d'état du mouvement de la porte
#define TEMOIN_ETAT_PORTE	PD5 //LED de retour d'état de la porte
#define	PORT_PROJECTEUR	PD7 //Port qui est branché au niveau de la LED projecteur.
#define PORT_LECTURE_LUMINOSITE A0 //Port de lecture de l'intensité lumineuse
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

// the setup routine runs once when you press reset:
void setup() {
  MsTimer2::set(FREQ_MESURE, InterruptTimer2); // Timer2 Interne Arduino : période 5000ms
  MsTimer2::start(); // active Timer 2
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_HAUT), FinDeCourseHaut, RISING); //Interuption capteur fin de course haut
  attachInterrupt(digitalPinToInterrupt(PORT_FIN_DE_COURSE_BAS), FinDeCourseBas, RISING); //Interuption capteur fin de course bas

  Ouvrir(); //etat de la porte au départ.
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
