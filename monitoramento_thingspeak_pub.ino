//Biblioteca nativa SoftwareSerial
#include <SoftwareSerial.h>
// Biblioteca do DHT
#include <DHT.h> //#include "DHT.h"
// biblioteca Narcoleptic para economizar bateria
#include <Narcoleptic.h>
// biblioteca para economia de bateria
//#include "LowPower.h"
// Definindo pins SoftwareSerial
// RX arduino pin=2 => TX SIM900 | TX arduino pin=3 => RX SIM900
SoftwareSerial sim900 = SoftwareSerial(2,3);
// variavel DHT
DHT dht;
// meu apiwrite thingspeak
String apiKey = "5MEP0V1O8ESKG5XH";
// configs cartao SIM vivo
String pwn = "zap.vivo.com.br";
String user = "vivo";
String password = "vivo";
boolean DEBUG=true;

//showResponse====================================================

void showResponse(int waitTime){
  long t=millis();
  char c;
  while(t+waitTime>millis()){
    if(sim900.available()){
      c = sim900.read();
      if(DEBUG) Serial.print(c);
    }
  }
}

// funcao que prepara a pilha TCP/IP =====================================
void prepara_tcp(){

  String cmd = "AT+CIPSTART=\"TCP\",\"";
         cmd += "api.thingspeak.com";   //"84.106.153.149";
         cmd += "\",\"80\"";
  sim900.println(cmd);
  delay(3000);
  if(DEBUG) Serial.println(cmd);
}

// funcao que faz o sim900 dormir
// recebe tempo em segundos
//int dorme_sim900(int segundos){
//  digitalWrite(7, HIGH);
//  sim900.println("AT+CSCLK=1");
//  showResponse(1000);
//  Serial.print("Sim900 Dormindo...");
//  for (int i = segundos; i > 0; i--){
//  delay(8000);
//  //Serial.println(i);
//  }
//  return true;
//}

//void acorda_sim900(){
//  // pino 7 DTR do sim900
//  // desliga o modo sleep programado como LOW para desativar modo Sleep
//  digitalWrite(7, LOW);
//  sim900.println("AT+CSCLK=0");
//  delay(1000);
//  showResponse(1000);
//  Serial.println("Sim900 Acordando...");
//}

// funcao que inicializa as configs do sim900====================================
void adquire_ip(){
    /* comandos AT devem ser sincronizados de modo que na sequencia dados
    comandos, um comando execute após o outro
    para maiores detalhes deverá ser consultado o manual de comandos AT do sim900
    V 1.3

    */
    // tempo necessario para sim900 encontrar rede gsm
    //colocar 5000
    Narcoleptic.delay(8000);
    // full funcionality 1
    sim900.println("AT+CFUN=1");
    delay(1000);
    showResponse(1000);
    // limpando configuracoes anteriores
    sim900.println("AT+CIPSHUT");
    delay(1000); // colocar 1000
    showResponse(1000);
    // verifica se o  cartao SIM foi inserido
    sim900.println("AT+CPIN?");
    showResponse(1000);
    // signal quality reporter
    sim900.println("AT+CSQ");
    showResponse(1000);
    // Verifica se o SIM foi registrado na rede e se nao esta bloqueado
    sim900.println("AT+CREG=1");
    showResponse(1000);
    // checa GRPS esta configurado
    sim900.println("AT+CGATT=1");
    delay(1000);
    showResponse(3000);

    if(sim900.find("ERROR") || sim900.find("+CREG: 0")){
      if(DEBUG) Serial.println("AT+CGATT GOT ERROR");
      adquire_ip();
    }
    // status inicial
    sim900.println("AT+CIPSTATUS");
    delay(1000);
    showResponse(1000);
    // 0 para conexao simples, 1 para conexao multipla
    sim900.println("AT+CIPMUX=0");
    showResponse(1000);
    // seta APN, username e o password do SIM vivo. Devera ser consultado no caso de outras //operadoras
    sim900.println("AT+CSTT=\""+pwn+"\",\""+user+"\",\""+password+"\"");  //password
    delay(1000);
    showResponse(1000);
    // possibilita conexao wireless, caso n obtenha resposta nao obtera IP
    // podera dar um delay maior para obter IP
    sim900.println("AT+CIICR");
    delay(2000);
    showResponse(2000);

    if(sim900.find("ERROR") || sim900.find("+PDP: DEACT")){
      if(DEBUG) Serial.println("AT+CIICR ERROR");
      // se nao retornar "OK", tenta adquirir IP novamente
      adquire_ip();
    }
    else{
      // Retorna o IP se conectado. depende do CIICR
      sim900.println("AT+CIFSR");
      delay(2000);
      showResponse(2000);

      if(sim900.find("ERROR")){
        if(DEBUG) Serial.println("AT+CIFSR ERROR");
        // caso nao retorne o IP, a funcao adquire_ip devera ser chamada novamente
        adquire_ip();
      }
    }
  }

/*

A funcao dormir executa o loop principal, desligando o sim900 varias vezes conforme
o i, pois o sim900 apos ser desligado liga-se novamente sozinho nao sei o porque ainda
porem procurei adequar o loop interno que faz 8 Narcoleptic.delay de 8s, totalizando
64 segundos, o que dá uma margem de 4 segundos pra quando o sim900 voltar a ligar e
ganhar rede
o maximo suportado por narcoleptic é 8s
o lowPower comportou0se de maneira adequada para o arduino, porem perde a sincronia
com o sim900A
nao foi testado com outros modulos GSM.
O comando "AT" foi executado so para verif se o sim900 acordou
*/
// funcao dormir ===============================================================
void dormir(){
  // laço principal que configura a quant de minutos
  for (int i=0; i<10; i++){

    sim900.println("AT+CPOWD=0");
    delay(1000);
    showResponse(1000);

    for (int i=0; i<8; i++){
      Narcoleptic.delay(8000);
      //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }

    sim900.println("AT");
    delay(1000);
    showResponse(1000);
  }
}
/*
recebe as duas leituras passadas por parametros e apos ter conseguido IP no Loop()
executa prepara_tcp() com dados da api do thingspeak
se o "AT+CIPSTART" nao retornar "OK" retorna falso e volta a executar o Loop(),
senao, faz o GET concatenando as strings.
a string "\r\n\r\n" possibilita fechar o commandline do comando AT+CIPSEND
para que possa enviar o GET.
a documentacao diz que pode fezer CTRL+Z, #026 ..., porem no sim900A nao deu certo
*/
//thingspeak======================================================
boolean thingSpeakWrite(float value1, float value2){
  // abre a conexao passando o protocolo, API e porta
  prepara_tcp();
  if(sim900.find("ERROR") || isnan(sim900)){
    if(DEBUG) Serial.println("AT+CIPSTART ERROR");
      return false;
  }
  else{
  // prepara o metodo GET
  String getStr = "GET http://api.thingspeak.com/update?api_key=";
         getStr +=  apiKey; // api Write do ThingSpeak
         getStr += "&field1=";
         getStr +=  String(value1); // temperatura
         getStr += "&field2=";
         getStr += String(value2); // umidade
         getStr += "\r\n\r\n";

   // send data length precisa passar o comprimento da String no AT+CIPSEND
   String cmd = "AT+CIPSEND=";                // comando AT que possibilita o envio do GET
          cmd += String(getStr.length());     // comprimento da String precisa ser passado
   sim900.println(cmd);

   if(DEBUG) Serial.println(cmd);
   delay(1000);

   if(sim900.find(">")){
     sim900.println(getStr);             //  envia através do GET
     if(DEBUG) Serial.println(getStr);
       showResponse(8000);
   }
   else{
     sim900.println("AT+CIPCLOSE");     // se der problema tirar
     if(DEBUG) Serial.println("AT+CIPCLOSE");
       delay(3000);
       return false;
   }
}
// se os dados forem emviados coloca o arduino
// e o sim900 pra dormir.
dormir();

return true;
}

// setup========================================================================
void setup() {
    // pino dtr do sim900A
    DEBUG=true;
    Serial.begin(9600);
    while(!Serial){
      ;                 // espera pela conexao serial
    }
    // baud rate;
    sim900.begin(9600);
    // configura o pino do DHT22
    dht.setup(5);

    if(DEBUG) Serial.println("Setup DHT e Serial Completa");
}

void loop(){
  // aguarda tempo minimo para sincronizar DHT22
  delay(dht.getMinimumSamplingPeriod());
  // get das medicoes
  float t = dht.getTemperature();
  float h = dht.getHumidity();

  // if DHT indisponivel
  if (isnan(t) || isnan(h)) {
     if (DEBUG) Serial.println("Falha ao realizar a leitura do DHT");
  }
  else {

        if (DEBUG) Serial.println(" \t\t\tTemperatura  => "+String(t)+" Celsius");
        if (DEBUG) Serial.println(" \t\t\tUmidade      => "+String(h)+" %");

        sim900.begin(9600); // baud rate;
        // utiliza os comandos AT para ganhar IP
        adquire_ip();

        thingSpeakWrite(t,h);
      }

}
