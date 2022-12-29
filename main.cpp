/*
 * Escola Técnica Fundação Liberato Salzano Vieira da Cunha
 * Projeto de uma Soft Starter monofásica
 *
 * Disciplina: Eletrônica de Potência e Acionamentos
 * Professor: João Neves
 *
 * Autores: Keoma da Silva e Bianca Alana Lopes
 * 2021/1
 *
 * Soft starter monofásica com controles
 * do tempo de aceleração e desaceleração, controle do angulo alfa inicial,
 * proteções de overload e superaquecimento, implementação de rele de by pass
 * e freio cc como trabalho de fim de semestre.
 *
 *
 */
 
 
#include <LiquidCrystal.h> // include the LCD library
 
  
const int rs = PB11, en = PB10, d4 = PA4, d5 = PA3, d6 = PB0, d7 = PA0; //STM32 Pins to which LCD is connected
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //Inicializa o LCD
#define botao_menu    PB9   //Define o botão MENU na porta digital PB9
#define botao_up      PB4   //Define o botão UP na porta digital PB8
#define botao_down    PB8   //Define o botão DOWN na porta digital PB4
#define botao_start   PA15  //Define o botão START na porta digital PA15
#define buzzer        PB6   //Define o BUZZER na porta digital PB6
#define freio         PB7   //Define o FREIO CC na saída digital PB7
#define opencir       PB14   //LARANJA Define o comando para abrir o circuito do triac na porta PB3 **VERIRICAR SE ESSA PORTA ESTÁ FUNCIONANDO**
#define bypass        PB12  //VERMELHO Define o rele de BYPASS na porta PA12
#define sens_c        PB1   //Entrada do conversor AD de corrente
 
 
//Variáveis Globais=================================================================================
void tela_loading();
void tela_softstarter();
void tela_rampa_acel();
void tela_rampa_desacel();
void tela_limite_corrente();
void tela_alfa_inicial();
void tela_start_finish();
void tela_freio_cc();
void aceleracao();
void desaceleracao();
void overload();
void relay_tester();
 
 
 
char tela = 0;                                    //Variável para selecionar a tela atual.
boolean botao1m, botao2u, botao3d, botao4s;       //Flags para armazenar o estado dos botões.
boolean stop_flag;
 
unsigned short int rampa_acel_seg = 10;
unsigned short int rampa_desacel_seg =15;
unsigned short int percentual =0;
unsigned short int tempo = 0;
int duty = 1800;
float corrente = 1;
float coeficiente_alfa;
float alfa = 180;
float alfa_inicial = 180;
float incrementador;
float coef_duty;
 
int calc_mA(float v_por_a);                      //Função para o cálculo da corrente em miliAmperes
short control = 1;                             //Variável de controle (para saber qual display está ativo)
int mil, cen, dez, uni;                        //Variáveis auxiliares para dados nos displays
int valorCorrente = 0x00;                              //Armazena valor a ser exibido no display
int mAmps = 0x00;                              //Armazena corrente em mA
double volts = 0x00;                            //Armazena tensão em Volts
boolean flagAux = 0x00;                        //Flag para determinar se o sensor está acionado
 
 
 
 
 
  
void setup() {                                    //Inicio da função setup
 
pinMode (botao_menu,   INPUT);                     //Define a porta como ENTRADA
pinMode (botao_up,     INPUT);                     //Define a porta como ENTRADA
pinMode (botao_down,   INPUT);                     //Define a porta como ENTRADA
pinMode (botao_start,  INPUT);                     //Define a porta como ENTRADA
pinMode (sens_c,       INPUT);                    //Define a porta como ENTRADA
pinMode (buzzer,      OUTPUT);                    //Define a porta como SAÍDA
pinMode (freio,       OUTPUT);                    //Define a porta como SAÍDA
pinMode (opencir,     OUTPUT);                    //Define a porta como SAÍDA
pinMode (bypass,      OUTPUT);                    //Define a porta como SAÍDA
pinMode (PA10,           PWM);                    //Define a porta PA10 como SAÍDA do sinal PWM.
 
     
 
 
   HardwareTimer timer(1);
   timer.setPrescaleFactor(1);
   timer.setOverflow(1800);
 
   pwmWrite (PA10, duty);
 
    
 
//Flags...
botao1m = 0;                 //Limpa a flag do botão MENU
botao2u = 0;                 //Limpa a flag do botão UP
botao3d = 0;                 //Limpa a flag do botão DOWN
botao4s = 0;                 //Limpa a flag do botão START
 
  lcd.begin(16,2);              //Declara display LCD 16*2
 
  tela_loading();
 
}//FIM DO VOID SETUP
 
 
  
void loop() {                                               //Inicio do laço de repetição Void Loop
 
      if(percentual ==  0) duty=1800;
 
           
                     
 
 
      
 
      //duty = alfa*10;
      if(digitalRead(botao_menu))     botao1m = 1;          //Botão MENU pressionado? seta flag
      if(!digitalRead(botao_menu) &&  botao1m){             //Botão MENU solto? flag setada?
                                                            //Sim...
         botao1m = 0;                                       //Limpa flag
         lcd.clear();                                       //Limpa LCD
         tela++;                                            //Acrescenta uma unidade à variável tela
         digitalWrite (buzzer, HIGH);
         delay(50);
         digitalWrite (buzzer, LOW);
 
          
     
          
          
 
         if(tela > 7) tela = 0;                             //Cria o loop do menu, se chegar na ultima tela, volta ao início
                                               }            //Fim da instrução de set da flag.
  switch(tela)                                              //Inicio da instrução switch.
      {
        case 0 : tela_softstarter();      break;               //Chama a tela SOFT STARTER
        case 1 : tela_rampa_acel();       break;               //Chama a tela RAMPA DE ACELERAÇÃO
        case 2 : tela_rampa_desacel();    break;               //Chama a tela RAMPA DE DESACELERAÇÃO
        case 3 : tela_limite_corrente();  break;               //Chama a tela CORRENTE MÁXIMA
        case 4 : tela_alfa_inicial();     break;               //Chama a tela ALFA INICIAL
        case 5 : tela_start_finish();     break;               //Chama a tela START/FINISH
        case 6 : tela_freio_cc();         break;
        //case 6 : relay_tester();          break;
 
      }//Fim da instrução switch
 
       
 
   
  
   }//fim do void loop
 
int calc_mA(float v_por_a)
{
    volts = (analogRead(sens_c)/4095.0)*5000; 
    mAmps = (volts - 2500)/v_por_a;
 
    if(mAmps < 0) mAmps = ((volts - 2500)/v_por_a)*(-1);
    return mAmps;
 
} //end calc_mA  
 
//Inicio do void tela de loading...=========================================================
void tela_loading(){
 
lcd.setCursor(0,0);           //Posiciona o cursor para a linha 0 e coluna 0
lcd.print("Iniciando...");    //Escreve no display
delay(2000);                  //Espera 3segundos para entrar no void setup
 
digitalWrite (buzzer, HIGH);
delay(50);
digitalWrite (buzzer, LOW);
delay(50);
digitalWrite (buzzer, HIGH);
delay(50);
digitalWrite (buzzer, LOW);
delay(50);
digitalWrite (buzzer, HIGH);
delay(50);
digitalWrite (buzzer, LOW);
delay(50);
 
}//FIM do void tela de loading...
 
//Inicio do void de tela Soft Starter=========================================
void tela_softstarter(){
   
   
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SoftStarter"); 
  lcd.setCursor(0,1);
  lcd.print("v1.0");
  lcd.print(" 30.06.21");
  delay(100);
  }//Fim do Void tela Soft Starter
 
 //Inicio do void de tela Rampa de Aceleração=========================================
void tela_rampa_acel(){
 
  if(percentual==100) tela=5;
  lcd.clear();                                              //limpa a tela
  lcd.setCursor(0,0);                                       //seta o cursor
  lcd.print("Rampa de Acel:");                              //escreve o nome do menu
  lcd.setCursor(7,1);                                       //setar o cursor na linha 2 e coluna 6 (coluna, linha)
  lcd.print(rampa_acel_seg);                                //exibe o valor contido na variável
  lcd.print("s");
  delay(100);
 
      if(digitalRead(botao_up))   botao2u   = 0x01;         //Botão Up pressionado? Seta flag
      if(digitalRead(botao_down)) botao3d   = 0x01;         //Botão Down pressionado? Seta flag
       
   if(!digitalRead(botao_up) && botao2u)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao2u = 0x00;                                       //Limpa flag
      rampa_acel_seg++;                                     //Incrementa variável segundos de rampa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
       
      if(rampa_acel_seg > 30) rampa_acel_seg = 5;           //Se o valor de rampa for maior que 15, volta a ser 5
    
   } //Fim da instrução do botão UP
    
   if(!digitalRead(botao_down) && botao3d)                  //Botão Down solto e flag setada?
   {                                                        //Sim...
      botao3d = 0x00;                                       //Limpa flag
      rampa_acel_seg--;                                     //Decrementa os valores de rampa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
       
      if(rampa_acel_seg < 5) rampa_acel_seg = 30;           //Se os valores de rampa forem menores de 5, o valor passa a ser 15  
    
   }//Fim da instrução do botão DOWN  
   
  }//Fim do Void Tela Rampa de Aceleração
 
//Inicio do void de tela Rampa de Desaceleração=========================================
void tela_rampa_desacel(){
 
  if(percentual==100) tela=5;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Rampa de Desacel:");
  lcd.setCursor(7,1);
  lcd.print(rampa_desacel_seg);
  lcd.print("s");
  delay(100);
 
      if(digitalRead(botao_up))   botao2u   = 0x01;         //Botão Up pressionado? Seta flag
      if(digitalRead(botao_down)) botao3d   = 0x01;         //Botão Down pressionado? Seta flag
       
   if(!digitalRead(botao_up) && botao2u)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao2u = 0x00;                                       //Limpa flag
      rampa_desacel_seg++;                                  //Incrementa variável segundos de rampa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      if(rampa_desacel_seg > 30) rampa_desacel_seg = 5;     //Se o valor de rampa for maior que 15, volta a ser 5
    
   } //Fim da instrução do botão UP
    
   if(!digitalRead(botao_down) && botao3d)                  //Botão Down solto e flag setada?
   {                                                        //Sim...
      botao3d = 0x00;                                       //Limpa flag
      rampa_desacel_seg--;                                  //Decrementa os valores de rampa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      if(rampa_desacel_seg < 5) rampa_desacel_seg = 30;     //Se os valores de rampa forem menores de 5, o valor passa a ser 15  
    
   }//Fim da instrução do botão DOWN  
 
   
  }//Fim do void tela rampa de desaceleração
 
//Inicio do void limite de corrente=============================================
void tela_limite_corrente(){
 
      if(percentual==100) tela=5;
                        
      lcd.clear();
      lcd.print("Corrente Maxima:");
      lcd.setCursor(6,1);
      
      if(corrente <= 0.950){
        float i;
        i = corrente*1000;
         
        lcd.print(i,0);
        lcd.print("mA");
        delay(100);      
        }       
      else if(corrente >= 0.950){
        lcd.print(corrente);
        lcd.print('A');
        delay(100);
        }
 
 
      if(digitalRead(botao_up))   botao2u   = 0x01;         //Botão Up pressionado? Seta flag
      if(digitalRead(botao_down)) botao3d   = 0x01;         //Botão Down pressionado? Seta flag
       
   if(!digitalRead(botao_up) && botao2u)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao2u = 0x00;                                       //Limpa flag
      corrente = corrente + 0.05;                                           //Incrementa variável corrente
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      //if(corrente > 2.00) corrente = 0.35;                   //Se o valor de corrente for maior que 3A, volta a ser 350mA
    
   } //Fim da instrução do botão UP
    
   if(!digitalRead(botao_down) && botao3d)                  //Botão Down solto e flag setada?
   {                                                        //Sim...
      botao3d = 0x00;                                       //Limpa flag
      corrente = corrente - 0.05;                                           //Decrementa os valores de corrente
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      //if(corrente < 0.35) corrente = 2.00;                   //Se os valores de corrente forem menores de 350, o valor passa a ser 3A  
    
   }//Fim da instrução do botão DOWN
 
               
            }//Fim do void tela corrente máxima
 
//Inicio do void tela alfa inicial==============================================
void tela_alfa_inicial(){
   
  if(percentual==100) tela=5;
   
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Alfa Inicial:");
  lcd.setCursor(6,1);
  lcd.print(alfa_inicial,0);
  lcd.print((char)223);
  delay(100);
 
        if(digitalRead(botao_up)){   botao2u   = 0x01;       //Botão Up pressionado? Seta flag
    }
         
        if(digitalRead(botao_down)) botao3d   = 0x01;       //Botão Down pressionado? Seta flag
       
   if(!digitalRead(botao_up) && botao2u)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao2u = 0x00;                                       //Limpa flag
      alfa_inicial=alfa_inicial+5;                                       //Incrementa variável angulo alfa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      if(alfa_inicial > 180) alfa_inicial = 70;               //Se o valor de alfa for maior que 90º, volta a ser 0º
    
   } //Fim da instrução do botão UP
    
   if(!digitalRead(botao_down) && botao3d)                  //Botão Down solto e flag setada?
   {                                                        //Sim...
      botao3d = 0x00;                                       //Limpa flag
      alfa_inicial=alfa_inicial-5;                                       //Decrementa os valores de alfa
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      if(alfa_inicial < 70) alfa_inicial = 180;               //Se os valores de alfa forem menores de 0º, o valor passa a ser 90º
       
   }//Fim da instrução do botão DOWN
   
   
   
  }//Fim do void tela alfa inicial
 
//Inicio do void de tela Start / Finish=========================================
void tela_start_finish(){
 
        
      /* for(int i=10000; i>0; i--){
       valorCorrente = calc_mA(0.185);
       }
 
       int A = valorCorrente/10;
       int B = corrente*1000;
       if(A > B) overload();*/
        
          if(percentual==0){
             
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("START:");
                lcd.setCursor(6,0);
                lcd.print(percentual);
                lcd.print("%");
                lcd.setCursor(0,1);
                lcd.print("ALFA:");
                lcd.setCursor(6,1);
                lcd.print(alfa_inicial,0);
                lcd.print((char)223);
                //lcd.setCursor(10,0);
                //lcd.print(A);
                //lcd.setCursor(10,1);
                //lcd.print(B);
                 
                delay(100);
 
             
      if(digitalRead(botao_start)) botao4s   = 0x01;        //Botão START pressionado? Seta flag
      if(!digitalRead(botao_start) && botao4s)              //Botão START solto e flag setada?
   {                                                        //Sim...
      botao4s = 0x00;                                       //Limpa flag
      alfa = alfa_inicial;
      coeficiente_alfa = alfa/100;
      
      duty = alfa*10;
      
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
      delay(50);
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
      delay(50);
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
      delay(2000);
       
      digitalWrite (opencir,   LOW);
      delay(50);
      digitalWrite (bypass,    LOW);
      delay(50);
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
      delay(500);
             
      for(percentual=0; percentual < 100, alfa > 0.00; ++percentual, alfa = alfa - coeficiente_alfa){    //Inicio do laço FOR para atualização da tela a cada contagem
 
          //sens_current = analogRead(sens_c);
           
          //sens_voltage = (analogRead(sens_c)/1024.0)*5000;
          //sens_current = ((sens_voltage-2500)/66);
           
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("START:");
          lcd.setCursor(6,0);
          lcd.print(percentual);
          lcd.print('%');
          lcd.setCursor(0,1);
          lcd.print("ALFA:");
          lcd.setCursor(6,1);
          lcd.print(alfa,0);
          lcd.print((char)223);
          lcd.setCursor(10,0);
          //lcd.print(valorCorrente);
          duty = alfa*10;
           
          //if(sens_current > corrente) overload();
                  
          if(duty <    0) duty =    0;
          pwmWrite(PA10, duty);
 
      
          delay(tempo);                                         //Recebe a variável TEMPO como instrução de delay em milisegundos.       
          tempo = rampa_acel_seg*10;                            //Multiplica o valor atual da variável por 10 e atribui o resultado à variável TEMPO
           
          }//Fim do laço de repetição FOR
 
                percentual=100;
                alfa =0;
                duty = 0;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("START:");
                lcd.setCursor(6,0);
                lcd.print(percentual);
                lcd.print('%');
                lcd.setCursor(0,1);
                lcd.print("ALFA:");
                lcd.setCursor(6,1);
                lcd.print(alfa,0);
                lcd.print((char)223);
                lcd.setCursor(10,0);
                //lcd.print(valorCorrente);
                delay(2000);
                 
                digitalWrite (bypass, HIGH);
                //delay(50);
                digitalWrite (buzzer, HIGH);
                delay(50);
                digitalWrite (buzzer,  LOW);
                delay(50);
           
              }//Fim do laço BOTAO
                }//Fim do laço IF PERCENTUAL == 0
     
                                   
          if(percentual==100){                              //Pergunta se a variavel percentual está em 100%
  
       
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("START:");
      lcd.setCursor(6,0);
      lcd.print(percentual);
      lcd.print('%');
      lcd.setCursor(0,1);
      lcd.print("ALFA:");
      lcd.setCursor(6,1);
      lcd.print(alfa,0);
      lcd.print((char)223);
      lcd.setCursor(10,0);
      //lcd.print(A);
      lcd.setCursor(10,1);
      //lcd.print(B);
      lcd.setCursor(14,1);
      lcd.print("bp");
      digitalWrite (bypass, HIGH);
      delay(100);
  
             
      if(digitalRead(botao_start)) botao4s   = 0x01;        //Botão START pressionado? Seta flag
      if(!digitalRead(botao_start) && botao4s)              //Botão START solto e flag setada?
   {                                                        //Sim...
      botao4s = 0x00;                                       //Limpa a flag
       
 
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer,  LOW);
      delay(50);
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer,  LOW);
      delay(50);
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer,  LOW);
       
 
      digitalWrite (bypass,  LOW);
      delay(2000);
       
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("START:");
    
      lcd.setCursor(6,0);
      lcd.print(percentual);
      lcd.print('%');
      lcd.setCursor(0,1);
      lcd.print("ALFA:");
      lcd.setCursor(6,1);
      lcd.print(alfa,0);
      lcd.print((char)223);
      lcd.setCursor(10,0);
      //lcd.print(valorCorrente);
       
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer,  LOW);
      delay(50);
 
   
         
      for(percentual=100; percentual > 0, alfa < alfa_inicial; percentual--, alfa = alfa + coeficiente_alfa ){    //Inicio do laço FOR para atualização da tela a cada contagem
       
          //sens_current = analogRead(sens_c);
          //sens_voltage = (5.0 / 1023.0)*analogRead(sens_c);
          //sens_voltage = sens_voltage - 2.5 + 0.007 ;
         // sens_current = sens_voltage/0.66;
          //if(sens_current > corrente) overload();
       
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("START:");
       
      lcd.setCursor(6,0);
      lcd.print(percentual);
      lcd.print('%');
      lcd.setCursor(0,1);
      lcd.print("ALFA:");
      lcd.setCursor(6,1);
      lcd.print(alfa,0);
      lcd.print((char)223);
      lcd.setCursor(10,0);
      //lcd.print(valorCorrente);
 
      duty = alfa*10;
      if(duty > 1800) duty = 1800;
      if(percentual == 0) duty = 1800;
     
      pwmWrite(PA10, duty);
           
      delay(tempo);     
      tempo = rampa_desacel_seg*10;
       
      }//Fim do laço de repetição FOR
          }//Fim da condição Botão 
             }//Fim do laço IF
          
                                   
  }//Fim do void tela Start/Finish
 
void tela_freio_cc(){
 
  if(percentual==0) tela=0;
   
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Freio CC");
      lcd.setCursor(0,1);
      lcd.print("Ativar?");
      delay(50);
 
 
       if(digitalRead(botao_start))  botao4s   = 0x01;      //Botão Down pressionado? Seta flag
       if(!digitalRead(botao_start) && botao4s)             //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao4s = 0x00;                                       //Limpa flag
       
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
 
      digitalWrite (opencir,   HIGH);
       
      digitalWrite (freio,    HIGH);
      percentual = 0;
      stop_flag = 1;
      alfa=alfa_inicial;
      delay(500);
           
      digitalWrite (freio,     LOW);
      digitalWrite (bypass,    LOW);
       
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Freio CC");
      lcd.setCursor(0,1);
      lcd.print("ATIVO!");
      delay(1000);
 
          
   } //Fim da instrução do botão start
    
       }//Fim do void freio cc
        
void overload(){//===========INICIO DO VOID OVERLOAD============================
     
      digitalWrite (buzzer,   HIGH);
      delay(50);
      digitalWrite (buzzer,    LOW);
 
      digitalWrite (opencir,   HIGH);
       
      digitalWrite (freio,    HIGH);
      percentual = 0;
      stop_flag = 1;
      alfa=alfa_inicial;
      delay(500);
           
      digitalWrite (freio,     LOW);
      digitalWrite (bypass,    LOW);
       
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("OVERLOAD");
    delay(1000);
    tela = 0;
         
    }//FIM do void OVERLOAD
 
void relay_tester(){
 
lcd.clear();
lcd.print("menu:freio");
lcd.print(" up:bp");
lcd.setCursor(0,1);
lcd.print("down:opc");
lcd.print(" start:N");
delay(100);
   
   
      if(digitalRead(botao_menu))   botao1m   = 0x01;         //Botão MENU pressionado? Seta flag
      if(digitalRead(botao_start)) botao4s   = 0x01;         //Botão START pressionado? Seta flag
      if(digitalRead(botao_up))   botao2u   = 0x01;         //Botão UP pressionado? Seta flag
      if(digitalRead(botao_down)) botao3d   = 0x01;         //Botão DOWN pressionado? Seta flag
       
   if(!digitalRead(botao_up) && botao2u)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao2u = 0x00;                                       //Limpa flag
 
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
      delay(1000);
      digitalWrite(bypass, HIGH);
      delay(1000);
      digitalWrite(bypass, LOW);
                                            
 
 
      //if(corrente > 2.00) corrente = 0.35;                   //Se o valor de corrente for maior que 3A, volta a ser 350mA
    
   } //Fim da instrução do botão UP
    
   if(!digitalRead(botao_down) && botao3d)                  //Botão Down solto e flag setada?
   {                                                        //Sim...
      botao3d = 0x00;                                       //Limpa flag
 
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
      delay(1000);
      digitalWrite(opencir, HIGH);
      delay(1000);
      digitalWrite(opencir, LOW);
      
 
 
      //if(corrente < 0.35) corrente = 2.00;                   //Se os valores de corrente forem menores de 350, o valor passa a ser 3A  
    
   }//Fim da instrução do botão DOWN
 
   if(!digitalRead(botao_menu) && botao1m)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao1m = 0x00;                                       //Limpa flag
       
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
      delay(1000);
      digitalWrite(freio, HIGH);
      delay(1000);
      digitalWrite(freio, LOW);
                                                  
 
 
      //if(corrente > 2.00) corrente = 0.35;                   //Se o valor de corrente for maior que 3A, volta a ser 350mA
    
   } //Fim da instrução do botão MENU
 
 
   if(!digitalRead(botao_start) && botao4s)                    //Botão Up solto e flag setada?
   {                                                        //Sim...
      botao4s = 0x00;                                       //Limpa flag
                                               
      digitalWrite (buzzer, HIGH);
      delay(50);
      digitalWrite (buzzer, LOW);
 
      //if(corrente > 2.00) corrente = 0.35;                   //Se o valor de corrente for maior que 3A, volta a ser 350mA
    
   } //Fim da instrução do botão UP
   
   
  }//FIM DO VOID TESTE DE RELES
 
[/código]