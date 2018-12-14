// include the library code:
#include < Arduino.h > #include < ESP8266WiFi.h > #include < ESP8266mDNS.h > #include < ArduinoOTA.h > #include < LiquidCrystal_I2C.h > #include < Wire.h > #include < ArduinoJson.h > #include < ESP8266HTTPClient.h > #include < WiFiUdp.h > //pour NTP
  #include < NTPClient.h >

  const char * ssid = "";
const char * password = "";

String listeLignes[3][7] = { //ligne (long), ligne (court), URL arrêt, direction1, direction2, direction3, direction4 (si il y a 4 noms différents pour la même direction)
  {
    "Tram D",
    "D",
    "http://data.metromobilite.fr/api/routers/default/index/clusters/SEM:GENEDVAILLA/stoptimes",
    "GIERES-PL. DES SPORTS",
    "Hector Berlioz - Universités",
    "Les Taillées - Universités",
    "Plaine des Sports"
  },
  {
    "Bus C5",
    "C5",
    "http://data.metromobilite.fr/api/routers/default/index/clusters/SEM:GENBONPASTE/stoptimes",
    "UNIVERSITES-BIOLOGIE",
    "Universités - Biologie",
    "null",
    "null"
  },
  {
    "Bus 13",
    "13",
    "http://data.metromobilite.fr/api/routers/default/index/clusters/SEM:GENHOUILLEB/stoptimes",
    "Maisons Neuves",
    "MAISONS NEUVES",
    "null",
    "null"
  }
};
int tailleListeLignes = 3; //à adapter à la taille de l'array

int heuresPassage[3][2]; //array dans laquelle on stocke les heures de passage.
//3: nb de lignes
//2: nb d'horaires stockées

WiFiUDP UDP; // Create an instance of the WiFiUDP class to send and receive
NTPClient timeClient(UDP);

String convertSecondsToTime(int num_seconds);

LiquidCrystal_I2C lcd(0x3F, 20, 4);

void setup() {
  Serial.begin(9600);
  Wire.begin(2, 0);
  lcd.begin(20, 4);

  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initialisation...");

  //setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("Echec connect wifi");
    lcd.setCursor(0, 1);
    lcd.print("Redemarrage...");
    delay(1000);
    ESP.restart();
  }

  lcd.setCursor(0, 0);
  lcd.print("Connecte a:      ");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  ArduinoOTA.begin();
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("horaires")) {
    lcd.print("mdns error");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  timeClient.begin();

}

void loop() {
  ArduinoOTA.handle();
  timeClient.update(); //update NTP time
  Serial.println(timeClient.getFormattedTime());
  int heureEnSecondes = (timeClient.getHours() + 2) * 3600 + timeClient.getMinutes() * 60 + timeClient.getSeconds(); //attention, peut-être changer pour l'heure d'hiver?

  lcd.setCursor(0, 0);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print("                    ");

  HTTPClient http; //Declare an object of class HTTPClient

  for (int i = 0; i < tailleListeLignes; i++) { //pour chaque ligne on fait une requête. oui on peut pas tout faire en une requête, car 1 requête = 1 arrêt et il y a différents arrêts. donc autant de requêtes que de lignes, même si certaines lignes partagent le même arrêt.
    //listeLignes[0]=nom long
    //listeLignes[1]=nom court
    //listeLignes[2]=url arrêt
    //listeLignes[3]=ID ligne
    //listeLignes[4]=direction

    http.begin(listeLignes[i][2]); //une requête http pour chaque arrêt. listeLignes[i][2] contient l'URL de l'arrêt.
    int httpCode = http.GET();

    if (httpCode = 200) { //Check the returning code

      String payload = http.getString(); //Get the request response payload
      //Serial.println(heureEnSecondes);                     //Print the response payload

      DynamicJsonBuffer jsonBuffer;
      JsonArray & reponse = jsonBuffer.parseArray(payload);

      //lcd.print(reponse.size());
      for (int j = 0; j < reponse.size(); j++) {

        // String tempDest = reponse[j]["pattern"]["desc"];
        // lcd.setCursor(1,0);
        // lcd.print("desc:");
        // lcd.setCursor(0,1);
        // lcd.print(tempDest);
        // lcd.setCursor(0,2);
        // lcd.print("listeligne:");
        // lcd.setCursor(0,3);
        // lcd.print(listeLignes[i][3]);
        // delay(100);

        if (reponse[j]["pattern"]["desc"] == listeLignes[i][3] || reponse[j]["pattern"]["desc"] == listeLignes[i][4] || reponse[j]["pattern"]["desc"] == listeLignes[i][5] || reponse[j]["pattern"]["desc"] == listeLignes[i][6]) { //si la destination correspond à ce qu'on cherche

          //ensuite on stocke les horaires dans un tableau, qu'on réutilisera ensuite pour les afficher sur l'écran
          if (reponse[j]["times"].size() >= 2) {
            heuresPassage[j][0] = reponse[j]["times"][0]["realtimeArrival"];
            heuresPassage[j][1] = reponse[j]["times"][1]["realtimeArrival"];
          } else if (reponse[j]["times"].size() == 1) { // si il reste des horaires
            heuresPassage[j][0] = reponse[j]["times"][0]["realtimeArrival"];
          }

        } // fin du test destination correspondant à ce qu'on cherche

        http.end(); //Close connection
      } //fin du second "for" j

    } else {
      lcd.setCursor(0, 0);
      lcd.print("erreur http");
      lcd.setCursor(0, 1);
      lcd.print(httpCode);

    } // fin du "if" sur http 200
  } //fin du premier "for" i

  //PARTIE affichage

  lcd.setCursor(0, 0);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 4);
  lcd.print("                    ");

  //maintenant qu'on a récupéré les horaires, on les affiche en se resservant des tableaux
  for (int k = 0; k < tailleListeLignes; k++) { //on reparcourt la liste des lignes.

    bool afficherUnSeulHoraire = false;

    lcd.setCursor(0, k);
    lcd.print(listeLignes[k][0]); //afficher le nom long de la ligne (ex: tram D)

    //conversion en minutes.
    heuresPassage[k][0] = (heuresPassage[k][0] - heureEnSecondes) / 60;
    heuresPassage[k][1] = (heuresPassage[k][1] - heureEnSecondes) / 60;

    if (heuresPassage[k][1] < heuresPassage[k][0]) {
      int tempHeurePassage = heuresPassage[k][0];
      heuresPassage[k][0] = heuresPassage[k][1];
      heuresPassage[k][1] = tempHeurePassage;
    } //ordonner les horaires: si le second est plus tôt que le premier, le mettre en premier.

    if (heuresPassage[k][0] < 0) {

      lcd.setCursor(14, k);
      lcd.print("m");
    } else if (heuresPassage[k][0] > 30 or afficherUnSeulHoraire) {
      lcd.setCursor(13, k);
      lcd.print(convertSecondsToTime(heuresPassage[k][0]));
    } else {
      int positionCurseur1 = 13, positionCurseur2 = 17; //décaler l'affichage si + de 9mn (2 caractères)
      if (heuresPassage[k][0] > 9) positionCurseur1 = 12;
      if (heuresPassage[k][1] > 9) positionCurseur2 = 16;
      lcd.setCursor(positionCurseur1, k);
      if (heuresPassage[k][0] < 0) {
        lcd.setCursor(12, k);
        lcd.print(">1");
      } else {
        lcd.print(heuresPassage[k][0]);
      }
      lcd.setCursor(14, k);
      lcd.print("m");
      lcd.setCursor(positionCurseur2, k);
      lcd.print(heuresPassage[k][1]);
      lcd.setCursor(18, k);
      lcd.print("m");
    }
  }

  delay(30000); //Send a request every 30 seconds

}

String convertSecondsToTime(int num_seconds) {
  int days = num_seconds / (60 * 60 * 24);
  num_seconds -= days * (60 * 60 * 24);
  int hours = num_seconds / (60 * 60);
  num_seconds -= hours * (60 * 60);
  int minutes = num_seconds / 60;
  return hours + ":" + minutes;
}