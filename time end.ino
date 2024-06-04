
#include "header.h"

enum MenuState {MAIN_MENU, CONFIGURATION_MENU, INCUBATION};
MenuState currentState = MAIN_MENU;

void setup() {
  Serial.begin(9600);
  servo1.attach(12);
  pinMode(RELE_1, INPUT);
  digitalWrite(RELE_1, LOW);
  pinMode(RELE_2, INPUT);
  digitalWrite(RELE_2, LOW);
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(SAVE_BUTTON, INPUT_PULLUP);
  pinMode(BUTTON_MENU, INPUT_PULLUP);
  pinMode(BUTTON_SERVO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, FALLING);
  lcd.createChar(0, arrow);
  startmessage();
  lcd.clear();
  Serial.println("Listo :D");
  segundos = convertir(minutos);  // Convertir minutos a segundos
  // Inicializa la comunicación con el RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar RTC");
    while (1);
  }
  // Solo para DS1307: inicializa el reloj si no está funcionando
  if (!rtc.isrunning()) {
    Serial.println("RTC no está corriendo, configurando la hora.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // Marca el tiempo de inicio
  startTime = rtc.now();
  DateTime now;
  readRTC(now);
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);

  currentState = CONFIGURATION_MENU;
  menuCount = 0;
}

void startmessage(){
  lcd.setCursor(0, 0);
  String message = "Incubadora Huevo"; 
  for (byte i = 0; i < message.length(); i++) {
    lcd.print(message[i]);
    delay(75);
  }
  lcd.setCursor(6, 1);
  lcd.print("V.03");
  delay(1500);
  lcd.clear();
}

void initserv() {
  if (digitalRead(BUTTON_SERVO) == LOW) { // Botón presionado
    if (!isServoRunning) { // Iniciar/reiniciar el servo
      iniciarCicloServo();
    }
  }

  if (isServoRunning) {
    unsigned long Timecurrent = millis();
    //Comprueba si es momento de mover el servo
    if (Timecurrent - lastMovementTime >= velCount) {
      moveServo();
      lastMovementTime = Timecurrent;
    }
    // Comprueba si es hora de comprobar el RTC
    if (Timecurrent - lastCheckTime >= 1000) { // Chequea cada segundo
      checkTime();
      lastCheckTime = Timecurrent;
    }
  }
}

void iniciarCicloServo() {
  Serial.println("Iniciando ciclos del servo");
  isServoRunning = true;
  currentCycle = 0;
  timeStart = rtc.now();  // Reiniciar el tiempo de inicio
  lastMovementTime = millis();
  lastCheckTime = millis();
}

void checkTime() {
  DateTime timeNow = rtc.now();
  TimeSpan timeElapsed = timeNow - timeStart;
  if (timeElapsed.totalseconds() >= segundos) {
    Serial.print("Han pasado ");
    Serial.print(timeCount);
    Serial.println(" minutos");
    servo1.write(90); // Detener el servo en la posición 90 grados
    Serial.println("Detenido");

    if (currentCycle < ciclos - 1) {
      currentCycle++;
      Serial.print("Iniciando ciclo ");
      Serial.println(currentCycle + 1);

      Serial.println("Reinicio");
      timeStart = rtc.now();  // Reiniciar el tiempo de inicio después de cada ciclo
    } else {
      Serial.println("Todos los ciclos completados");
      isServoRunning = false; // Detener el programa después de todos los ciclos
    }
  }
}

void moveServo() {
  if (isMovingRight) {
    if (pos < angCount) {
      pos++;
    } else {
      isMovingRight = false;
    }
  } else {
    if (pos > 0) {
      pos--;
    } else {
      isMovingRight = true;
    }
  }
  servo1.write(pos);
}

int convertir(int minutos) {
  return minutos * 60;  // Convierte minutos a segundos
}

void readRTC(DateTime &now) {
  now = rtc.now();
}

void sensor(float& h, float& t) {
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float hic = dht.computeHeatIndex(t, h, false);
}

void relay() {
  if (t > tempCount) { //ventilador
    digitalWrite(RELE_2, HIGH);
  } else {
    digitalWrite(RELE_2, LOW);
  }

  if (t < tempCount) { //foco
    digitalWrite(RELE_1, HIGH);
  } else {
    digitalWrite(RELE_1, LOW);
  }
}

void servmo() {
  lcd.setCursor(0, 0);
  lcd.print("Ang:");
  lcd.print(angCount);
  lcd.setCursor(8, 0);
  lcd.print("Vel:");
  lcd.print(velCount);
  lcd.setCursor(0, 1);
  lcd.print("Time:");
  lcd.print(timeCount);
  lcd.setCursor(8, 1);
  lcd.print("Time2:");
  lcd.print("32"); //intervalo de tiempo realiza cada movimiento servo
}

void incub() {
  lcd.setCursor(2, 0);
  lcd.print("Obj: ");
  //countdays();
  //lcd.setCursor(6, 0);
  lcd.print(daysCount);
  lcd.print(" dias");
}

void disptemp() {
  lcd.setCursor(1, 0);
  lcd.print("Ta:");
  lcd.print(t, 1);
  lcd.setCursor(9, 0);
  lcd.print("To:");
  lcd.print(tempCount);

  DateTime now;
  readRTC(now);
  lcd.setCursor(1, 1);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.setCursor(7, 1);
  lcd.print('-');
  lcd.setCursor(9, 1);
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
}

void readEncoder() {
  int dtValue = digitalRead(ENCODER_DT);
  if (dtValue == HIGH) {
    delay(100);
    if (selectMenu) menuCount++;
    if (selectDays) daysCount++;
    else if (selectTemp) tempCount++;
    else if (selectVel) velCount++;
    else if (selectAng) angCount = min(angCount + 1, 180); // Limitar angCount a 180
    else if (selecttime) {
      timeCount++;
      segundos = convertir(timeCount);
    } // Actualizar segundos

  } else {
    if (selectMenu) menuCount--;
    if (selectDays) daysCount = max(daysCount - 1, 0); // Limitar daysCount a un mínimo de 0
    else if (selectTemp) tempCount--;
    else if (selectVel) velCount = max(velCount - 1, 0); // Limitar velCount a un mínimo de 0 
    else if (selectAng) angCount = max(angCount - 1, 0); // Limitar angCount a un mínimo de 0
    else if (selecttime) {
      timeCount = max(timeCount - 1, 1); // Limitar timeCount a un mínimo de 1
      segundos = convertir(timeCount);
    } // Actualizar segundos
  }
  newContent = true;
}

enum State {TEMP, INCUB, SERV};
State Statecurrent = TEMP;

void checkBut(int pin) {
  if (digitalRead(pin) == LOW) {
    currentState = MAIN_MENU;
    menuCount = 0;
    Serial.println("Menu");
  }
}

void loop() {
  sensor(h, t);
  static int lastMenuCount = -1;  // Almacenar el último estado de menuCount para detectar cambios
  static MenuState lastState = MAIN_MENU;  // Almacenar el último estado para detectar cambios
  // Revisar los límites de menuCount basados en el estado actual
  if (currentState == MAIN_MENU) {
    menuCount = max(0, min(menuCount, 1));
  } // Solo 2 opciones disponibles

  if (currentState != lastState || menuCount != lastMenuCount ) { // Solo actualiza la pantalla si hay cambios
    lcd.clear();  // Limpiar sólo si es necesario (cuando cambia el estado)
    lastState = currentState;
    lastMenuCount = menuCount;
  }

  switch (currentState) {
    case MAIN_MENU:
      lcd.setCursor(0, menuCount);
      lcd.write(0);
      lcd.setCursor(1, 0);
      lcd.print("1.Iniciar Incu");
      lcd.setCursor(1, 1);
      lcd.print("2.Config");
      if (digitalRead(SAVE_BUTTON) == LOW) {
        switch (menuCount) {
          case 0: currentState = INCUBATION; menuCount = 0; break;
          case 1: currentState = CONFIGURATION_MENU; menuCount = 0; break;
        }
      }
      break;

    case CONFIGURATION_MENU:
      if (newContent) {
        lcd.clear();
        if (selectDays) {
          lcd.setCursor(0, 0);
          lcd.print("Dias encubacion: ");
          lcd.setCursor(7, 1);
          lcd.print(daysCount);
        } else if (selectTemp) {
          lcd.setCursor(0, 0);
          lcd.print("Select Temp:");
          lcd.setCursor(7, 1);
          lcd.print(tempCount);
        } else if (selectVel) {
          lcd.setCursor(0, 0);
          lcd.print("Select Vel:");
          lcd.setCursor(7, 1);
          lcd.print(velCount);
        } else if (selectAng) {
          lcd.setCursor(0, 0);
          lcd.print("Select Ang:");
          lcd.setCursor(7, 1);
          lcd.print(angCount);
        } else if (selecttime) {
          lcd.setCursor(0, 0);
          lcd.print("Select Time:");
          lcd.setCursor(7, 1);
          lcd.print(timeCount); // Asegúrate de que `timeCount` se imprima aquí
        } else if (selectconfig) {
          lcd.setCursor(3, 0);
          lcd.print("Fin de la");
          lcd.setCursor(1, 1);
          lcd.print("Configuracion");
          delay(1000);
          currentState = MAIN_MENU; menuCount = 0; break;
        }
        newContent = false;
      }

      if (digitalRead(SAVE_BUTTON) == LOW) {
        delay(500);
        lcd.clear();
        if (selectDays) {
          selectDays = false;
          selectTemp = true;
        } else if (selectTemp) {
          selectTemp = false;
          selectVel = true;
        } else if (selectVel) {
          selectVel = false;
          selectAng = true;
        } else if (selectAng) {
          selectAng = false;
          selecttime = true;
        } else if (selecttime) {
          selecttime = false;
          selectconfig = true;
        } else if (selectconfig) {
          selectconfig = false;
        }
        newContent = true;
      }
      break;

    case INCUBATION:
      checkBut(BUTTON_MENU);
      display();
      relay();
      initserv();
      break;
  }
}

void display() {
  if (incubationComplete) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Incubacion");
    lcd.setCursor(0, 1);
    lcd.print("completada!");
    // Aquí puedes añadir código para detener otros procesos si es necesario
    return;
  }

  switch (Statecurrent) { // Ejecutar la función correspondiente al estado actual
    case TEMP:
      disptemp();
      if (digitalRead(SAVE_BUTTON) == LOW) {
        delay(200);
        lcd.clear();
        Statecurrent = INCUB;
      } // Cambiar al siguiente estado
      break;
    case INCUB:
      incub();
      countdays();
      if (digitalRead(SAVE_BUTTON) == LOW) {
        delay(200);
        lcd.clear();
        Statecurrent = SERV;
      }  // Cambiar al siguiente estado
      break;
    case SERV:
      servmo();
      if (digitalRead(SAVE_BUTTON) == LOW) {
        delay(200);
        lcd.clear();
        Statecurrent = TEMP;
      } // Volver al primer estado
      break;
  }
}

void countdays() {
  unsigned long currentTimeMillis = millis();
  if (currentTimeMillis - lastExecutionTime >= 1000) {  // Determinar si es hora de actualizar
    lastExecutionTime = currentTimeMillis;  // Actualizar el tiempo de la última ejecución

    DateTime currentTime = rtc.now(); // Obtener la hora actual del RTC
    TimeSpan elapsedTime = currentTime - startTime; // Calcular el tiempo transcurrido
    // Calcular días, horas y minutos
    int days = elapsedTime.days();
    int hours = elapsedTime.hours() % 24;  // Asegurarse de que las horas estén en un formato de 0-23
    int minutes = elapsedTime.minutes() % 60;  // Asegurarse de que los minutos estén en un formato de 0-59

    if (days >= daysCount) { // 'diasIncubacion' es la duración total de la incubación en días
      incubationComplete = true;
    }

    if (!incubationComplete) {
      lcd.setCursor(2, 1); // Mostrar el tiempo transcurrido en la LCD
      lcd.print("D:");
      lcd.print(days);
      lcd.print(" H:");
      lcd.print(hours);
      lcd.print(" M:");
      lcd.print(minutes);
    }
  }
}
