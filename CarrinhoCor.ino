#include <HCSR04.h>
#include <Wire.h>

// Sensor Ultrassônico (HC-SR04)
#define TRIG_PIN 17    // Pino Trig do sensor ultrassônico
#define ECHO_PIN 18    // Pino Echo do sensor ultrassônico

// Sensores de cor (TCS230)
#define SENSORCOR0 9   // Pino S0 do sensor de cor
#define SENSORCOR1 10   // Pino S1 do sensor de cor
#define SENSORCOR2 12   // Pino S2 do sensor de cor
#define SENSORCOR3 13   // Pino S3 do sensor de cor
#define SENSORCOR_OUT 11 // Pino OUT do sensor de cor

// LED de indicação
#define LED_VM 8  // Pino do LED VM que indica quando o carrinho parar
#define LED_VD 19 // Pino do LED verde (adicione no hardware)

// Sensores Infravermelhos para controle de linha
#define SENSOR_ESQ 14   // Sensor de linha esquerdo
#define SENSOR_MEIO 15  // Sensor de linha central
#define SENSOR_DIR 16   // Sensor de linha direito

// Controle dos motores via Ponte H
#define MOTOR_DIR_FRONTE 2  // IN1 do motor direito (frente)
#define MOTOR_DIR_TRAS 4    // IN2 do motor direito (trás)
#define MOTOR_ESQ_FRONTE 5  // IN3 do motor esquerdo (frente)
#define MOTOR_ESQ_TRAS 7    // IN4 do motor esquerdo (trás)
#define ENA 3  // Habilita o motor direito (PWM)
#define ENB 6 // Habilita o motor esquerdo (PWM)

// Inicializando o Sensor Ultrassônico
HCSR04 ultraS(TRIG_PIN, ECHO_PIN);

// Variáveis para armazenar os valores das cores
int vermelho = 0;
int verde = 0;
int azul = 0;

// Variáveis para controlar os estados dos sensores infravermelhos
int estadoEsq = 0;
int estadoMeio = 0;
int estadoDir = 0;

// Variável para controle de movimento
bool podeAndar = false;

// Variáveis de controle de estado
bool corVermelhaDetectada = false; 
bool corVerdeDetectada = false; // Nova variável para controle da cor verde

// Classe para controle dos motores
class Motor {
  int pin1, pin2, EN, pinEN;

  public:
    void saidaPino(int in1, int in2, int EN) {
      pin1 = in1;
      pin2 = in2;
      pinEN = EN;
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
      pinMode(pinEN, OUTPUT);
    }

    void defineVelocidade(int valor) {
      analogWrite(pinEN, valor);
    }

    void frente() {
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW);
    }

    void tras() {
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
    }

    void para() {
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, HIGH);
    }
};

Motor motor1, motor2; // Criando os dois motores

// Funções para controle dos motores
void direita() {
   motor1.defineVelocidade(150);
   motor2.defineVelocidade(200);
   motor1.tras();
   motor2.frente();
}

void esquerda() {
   motor1.defineVelocidade(200);
   motor2.defineVelocidade(150);
   motor1.frente();
   motor2.tras();
}

void frente() {
  motor1.defineVelocidade(170);
  motor2.defineVelocidade(170);
  motor1.frente();
  motor2.frente();
}

void para() {
  motor1.defineVelocidade(0);
  motor2.defineVelocidade(0);
  motor2.para();
  motor1.para();
}

// Função para detectar cor e controlar o comportamento do carrinho
void estadoSensorCor() {
  // Leitura das cores
  digitalWrite(SENSORCOR2, LOW);
  digitalWrite(SENSORCOR3, LOW);
  vermelho = pulseIn(SENSORCOR_OUT, LOW);  // Leitura do vermelho

  digitalWrite(SENSORCOR2, HIGH);
  digitalWrite(SENSORCOR3, HIGH);
  verde = pulseIn(SENSORCOR_OUT, LOW);  // Leitura do verde

  digitalWrite(SENSORCOR2, LOW);
  digitalWrite(SENSORCOR3, HIGH);
  azul = pulseIn(SENSORCOR_OUT, LOW);  // Leitura do azul

  Serial.println();
  Serial.print("Vermelho: ");
  Serial.print(vermelho);
  Serial.print(" Verde: ");
  Serial.print(verde);
  Serial.print(" Azul: ");
  Serial.print(azul);

  // Detecção da cor vermelha
  if (vermelho >= 50 && vermelho <= 120 && verde >= 210 && verde <= 280 && azul >= 150 && azul <= 210) {  
    Serial.println(" -> Cor detectada: Vermelho");
    para();  // Garante que o carrinho pare
    podeAndar = false;  // Desativa o movimento
    corVermelhaDetectada = true;  // Marca que a cor vermelha foi detectada
    corVerdeDetectada = false;  // Reseta a detecção do verde
    digitalWrite(LED_VM, HIGH); // LED vermelho aceso permanentemente
    digitalWrite(LED_VD, LOW);  // LED verde apagado
  }  
  
  // Detecção da cor verde
  else if (vermelho >= 300 && vermelho <= 380 && verde >= 210 && verde <= 260 && azul >= 190 && azul <= 250) {  // Verde
    Serial.println(" -> Cor detectada: Verde");
    podeAndar = true;  // Permite que o carrinho ande
    corVerdeDetectada = true;  // Marca que a cor verde foi detectada
    corVermelhaDetectada = false; // Reseta a detecção do vermelho
    digitalWrite(LED_VM, LOW); // Apaga o LED vermelho
    digitalWrite(LED_VD, HIGH); // Acende o LED verde
  } 

  // Detecção da cor amarela
  else if (vermelho >= 25 && vermelho <= 70 && verde >= 40 && verde <= 80 && azul >= 80 && azul <= 120) {  // Amarelo
    Serial.println(" -> Cor detectada: Amarelo");
    // Pisca o LED vermelho, mas o carrinho continua em movimento
    if (!corVermelhaDetectada && corVerdeDetectada) {
      digitalWrite(LED_VM, HIGH);
      delay(250);
      digitalWrite(LED_VM, LOW);
      delay(250);
    }
  } 
  else {  // Cor desconhecida
    Serial.println(" -> Cor desconhecida");
    // O LED vermelho deve permanecer aceso se a cor vermelha foi detectada
    if (corVermelhaDetectada) {
      digitalWrite(LED_VM, HIGH);  // Mantém o LED vermelho aceso
    }
    digitalWrite(LED_VD, LOW);  // Desliga o LED verde
  }
}

// Função que lê os sensores infravermelhos e controla o carrinho
void estadoSensorInfra() {
  if (ultraS.dist() <= 6) {
    para();  // Para o carrinho se um objeto estiver próximo
    // Pisca o LED verde
    for (int i = 0; i < 5; i++) { // Pisca 5 vezes
      digitalWrite(LED_VD, HIGH);
      delay(250);
      digitalWrite(LED_VD, LOW);
      delay(250);
    }
  } else if (corVerdeDetectada && podeAndar) {
    // Se a cor verde foi detectada e pode andar, continue se movendo
    estadoEsq = digitalRead(SENSOR_ESQ);
    estadoDir = digitalRead(SENSOR_DIR);
    estadoMeio = digitalRead(SENSOR_MEIO);
    digitalWrite(LED_VD, HIGH); // Mantém o LED verde aceso enquanto o carrinho anda
  
    if(estadoEsq == 0 and estadoMeio == 0 and estadoDir == 0){
      Serial.println("sensor esquerdo = 0, meio = 0 e direito = 0 segue em frente");
      frente();
    }
    else if(estadoEsq == 0 and estadoMeio == 0 and estadoDir == 1){
      Serial.println("sensor esquerdo = 0, meio = 0 e direito = 1 segue em direita frente");
      direita();
    }
    else if(estadoEsq == 0 and estadoMeio == 1 and estadoDir == 0){
      Serial.println("sensor esquerdo = 0, meio = 1 e direito = 0 segue em frente");
      para();  
    }
    else if(estadoEsq == 0 and estadoMeio == 1 and estadoDir == 1){
      Serial.println("sensor esquerdo = 0, meio = 1 e direito = 1 segue direita");
      direita();
    }
    else if(estadoEsq == 1 and estadoMeio == 0 and estadoDir == 0){
      Serial.println("sensor esquerdo = 1, meio = 0 e direito = 0 segue em esquerda frente");
      esquerda();
    }
    else if(estadoEsq == 1 and estadoMeio == 1 and estadoDir == 0){
      Serial.println("sensor esquerdo = 1, meio = 1 e direito = 0 segue em esquerda");
      esquerda();
    }
    else if(estadoEsq == 1 and estadoMeio == 0 and estadoDir == 1){
      Serial.println("sensor esquerdo = 1, meio = 0 e direito = 1 segue frente");
      frente();
    }
    //else if(estadoEsq == 1 and estadoMeio == 1 and estadoDir == 1){
      //Serial.println("sensor esquerdo = 1, meio = 1 e direito = 1 segue tras");
      //tras();
    //}
  } else { // Essa linha foi adicionada para corrigir o erro
    para(); // Para o carrinho se não puder andar
    digitalWrite(LED_VD, LOW); // Desliga o LED verde se o carrinho não puder andar
  }
}

void testeSensorUltra() {
  Serial.println(ultraS.dist());
  delay(500);
}

void testeSensorInfra() { //Função de teste dos sensores
  estadoEsq = digitalRead(SENSOR_ESQ);
  estadoDir = digitalRead(SENSOR_DIR);
  estadoMeio = digitalRead(SENSOR_MEIO);

  Serial.println("Sensor Direito");
  Serial.println(String(estadoDir));
  delay(500);

  Serial.println("Sensor Meio");
  Serial.println(String(estadoMeio));
  delay(500);

  Serial.println("Sensor Esquerdo");
  Serial.println(String(estadoEsq));
  delay(500);
}

void setup() {
  // Configura os pinos dos motores
  motor1.saidaPino(MOTOR_DIR_FRONTE, MOTOR_DIR_TRAS, ENA);
  motor2.saidaPino(MOTOR_ESQ_FRONTE, MOTOR_ESQ_TRAS, ENB);

  // Configura os pinos dos sensores infravermelhos
  pinMode(SENSOR_ESQ, INPUT);
  pinMode(SENSOR_DIR, INPUT);
  pinMode(SENSOR_MEIO, INPUT);

  // Configura os pinos do sensor de cor
  pinMode(SENSORCOR0, OUTPUT);
  pinMode(SENSORCOR1, OUTPUT);
  digitalWrite(SENSORCOR0, HIGH); // S0 HIGH
  digitalWrite(SENSORCOR1, LOW);  // S1 LOW -> Frequência de saída de 20%

  // Configura os pinos de seleção de cor e o pino de saída
  pinMode(SENSORCOR2, OUTPUT);
  pinMode(SENSORCOR3, OUTPUT);
  pinMode(SENSORCOR_OUT, INPUT);
  pinMode(LED_VM, OUTPUT);

  // Configura os LEDS
  pinMode(LED_VM, OUTPUT);
  pinMode(LED_VD, OUTPUT); // Configura o pino do LED verde como saída

  Serial.begin(9600);  // Inicializa a comunicação serial
}

void loop() {
  estadoSensorCor();  // Chama a função para detectar a cor e controlar os LEDs e o movimento
  estadoSensorInfra();  // Chama a função para controlar os sensores infravermelhos
}
