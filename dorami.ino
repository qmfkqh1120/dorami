#include <EEPROM.h>
#include <MsTimer2.h>
#define Node_ID 7
#define Server_ID 235
#define Port_Num 7778
const int MOTOR_A_PINS[3]= {42, 44, 4};
const int MOTOR_B_PINS[3] = {46, 48, 5};
const int BUZZER = 40;
 
uint8_t ID = 0;
uint32_t timer_check = 0;
//RX_flag=0일 때는 데이터를 수신할 수 없음. 1일 때는 데이터를 수신할 수 있음. 2는 오류
//TX_flag는 데이터를 송신한 ID를 저장(0x00: 서버, 그 외: 클라이언트)
uint8_t RX_flag = 0, TX_flag = 0, Timer_flag = 0;
//EEPROM에는 항상 0xAA와 ID 값을 저장해야 한다.
uint8_t EEPROM_buf[2] = {0xAA, 0};
//Wi-Fi shield packet info에서 정의된 패킷의 길이는 항상 17byte
char RX_buf[17];
//Wi-Fi shield packet info에서 header는 항상 0xA0, 0x0A이며 tail은 항상 0x0A, 0xA0이다.
uint8_t TX_buf[17] = {0xA0, 0x0A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0xA0};
//RX_count가 낮으면 Wi-Fi 모듈 설정중, 높으면 데이터를 보낼 수 있는 상태이다.
int RX_count = 0;
void setup() {
  static int RX_count = 0;
  static char Check_buf[3] = {0, 0, 0};
    int i;
 for (i = 0; i < 3; ++i) {
 pinMode(MOTOR_A_PINS[i], OUTPUT);
 pinMode(MOTOR_B_PINS[i], OUTPUT);
 }
 pinMode(BUZZER, OUTPUT);
  
  Serial.begin(115200);
  Serial1.begin(9600);
  
  delay(1000);
  
  timer_check = millis();
  
  Serial.println("Start Wifi Setting");
  
  Serial1.print("AT\r\n"); //OK response
  delay(100);
  
  Serial1.print("AT\r\n"); //OK response
  delay(100);
  
  Serial1.print("AT+WAUTO=0,jy931120\r\n");  //authentication mode none
  delay(10);
  
  Serial1.print("AT+NDHCP=0\r\n"); //DHCP is disabled
  delay(10);
  Serial1.print("AT+NAUTO=0,1,192.168.43."); //set network parameters to be used for Auto connect. <type>,<protocol>,<destination ip>,<destination port>. when type is 0 for client and type 1 is for server, protocol 1 is for udp and 1 for tcp
  Serial1.print(Server_ID);
  Serial1.print(",");
  Serial1.print(Port_Num);
  Serial1.print("\r\n");
  delay(10);
  
  Serial1.print("AT+NSET=192.168.43."); //Static network parameters. <src address>,<net-mask>,<gateway>
  Serial1.print(Node_ID);
  Serial1.print(",255.255.255.0,192.168.43.1\r\n");
  delay(10);
  
  Serial1.print("AT&W0\r\n"); //save profile spceified by 0
  delay(10);
  
  Serial1.print("ATC0\r\n"); //auto connect is disabled on next reboot or AT
  delay(10);
  
  Serial1.print("ATA\r\n"); //start auto connect, including association
  delay(10);
  
  Serial.println("Wifi Setting Finish");
  
  attachInterrupt(0, Motion_ISR, FALLING);
  
  //timer2 period: 200ms, ISR: TIMER_ISR 설정
  MsTimer2::set(200, TIMER_ISR);
  
 //timer2 start
  MsTimer2::start();
}
void loop() {
  uint16_t i, tmp = 0;
  //pinmode와 serial 초기화를 설정한 시간으로부터 4초 이하면서 RX_falg가 0일 경우
  if(((timer_check+4000) < millis()) && (RX_flag == 0)) {
    if(RX_count < 1) { //RX_count가 0이면 초기상태
      if(EEPROM.read(0) == 0xAA) { //EEPROM의 첫 번째 번지가 0xAA라면 사용자가 한번도 EEPROM을 수정하지 않았다.
        ID = EEPROM.read(1); //EEPROM에 저장된 id를 읽어온다.
        RX_flag = 1; //데이터를 수신할 수 있는 상태가 된다.
        
        Serial.print("\n\rID : ");
        Serial.println(ID);
        
        TX_buf[3] = ID; //TX_buf의 4번째 인덱스에 id를 저장한다.
      } else {
        RX_flag = 2; //오류
        
        Serial.println("\n\rWifi Connected Error");
        Serial.print("\n\rPlease reset the ADK-2560"); 
      }
    }
  }
  
  if(Timer_flag && TX_flag) { //Timer_flag 발생, 내 id가 서버가 아니라면(서버라면 0x00->당연히 if문 통과 안함)
    //센서값 읽음
  
    
    Serial.println("\n\r TX Packet data");
    
    for(i=0; i<17; i++) {
      Serial.write(' ');
      Serial.print(TX_buf[i],HEX);
      Serial1.write(TX_buf[i]);
      //TX_buf에 쓰인 정보를 serial(모니터)과 serial1(와이파이 모듈)에 출력
    }
    
    Timer_flag = 0;
  }
}
//serial1에서 읽어올 데이터가 존재하면 자동으로 실행
void serialEvent1(void) {
  static char Check_buf[4] = {0, 0, 0, };
  uint8_t i,check_sum = 0, RX_cnt = 0;
  
  if(RX_flag == 0) { //데이터를 수신할 수 없을 때(아직 setup()함수 작동 중)
    char da = Serial1.read(); //데이터를 읽어옴
    Serial.write(da); //이 데이터를 serial로 보냄
    
    Check_buf[0] = Check_buf[1];
    Check_buf[1] = Check_buf[2];
    Check_buf[2] = da;
    //Serial1에서 읽어온 데이터가 'ATA'이며, RX 버퍼에서 읽어온 횟수가 0회 일 때
    //ATA는 start auto connect, including association : Wi-Fi 모듈의 IP, Subnet, Gateway 정보를 출력하고 : 문자로 구분한다.
    if((Check_buf[0] == 'A') && (Check_buf[1] == 'T') && (Check_buf[2] == 'A') && (RX_count == 0)) {
      RX_count = 1; 
    } else if(RX_count == 4) { //RX_count가 4일 때 serial1에서 읽어온 데이터에 :가 있으면 구체적인 오류가 있을 때임
      if(Check_buf[2] != ':') {
        ID = ID*10 + (Check_buf[2]-'0');
      } else { //오류가 없으면 RX_counter를 증가시켜 아무 행동도 안함
        RX_count++; 
      }
    } else if(RX_count == 5) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'K')) { //Check_buf의 값이 OK] 일 때
          RX_flag = 1; //RX 가져올 값이 있음
          delay(1000);
          
          RX_cnt = Serial1.available(); //Serial1에서 읽어올 데이터 크기를 RX_cnt에 저장
          Serial.println(RX_cnt); //Serial1에서 읽어올 데이터 크기를 사용자에게 알려줌
          
          while(1) {
            Serial.write(Serial1.read()); //Serial1에서 데이터를 읽어와 Serial에 쓴다.
            RX_cnt--; //RX_cnt 감소
            
            if(RX_cnt == 0) //더이상 읽어올 데이터가 없으면 while문 탈출
              break;
          }
    
          Serial.print("\n\rID : ");
          Serial.print(ID);
          
          EEPROM_buf[1] = ID;
          
          if(EEPROM.read(1) != ID) { //EEPROM 두 번째 번지의 값이 id와 다르면(EEPROM을 수정한 흔적이 있으면)
            for(i=0; i<2; i++) {
              EEPROM.write(i,EEPROM_buf[i]); //EEPROM 첫 번째 번지는 0xAA, 두 번째 번지는 id 기록
            }
          }
          
          TX_buf[3] = ID; //TX_buf 배열 3에 장치 id 저장
        } else { //Check_buf의 값이 OK]가 아니면(문제 발생)
          RX_flag = 2; //오류
          
          Serial.print("\n\rWifi Connected Error");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    } else if((Check_buf[2] == '.') && ((RX_count == 1) || (RX_count == 2) || (RX_count == 3))) { //Check_buf[2]가 . 일 경우는 ip 혹은 subnet 혹은 gatewaqy 주소 출력 중임
      RX_count++;
    } else if(RX_count == 1) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'R')) { //오류인 경우
          RX_flag = 2;
          
          Serial.print("\n\rWifi Connected fail");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    }
    
  } else if(RX_flag == 1) { //데이터를 수신할 수 있을 때(loop() 함수 작동 중)
    if(Serial1.available() > 16) { //Serial1에서 읽어올 데이터 크기가 16바이트를 넘으면
      Serial1.readBytes(RX_buf, 17); //앞의 17바이트만 읽어와 RX_buf에 저장
      
      if(((uint8_t)RX_buf[0] == 0xA0) && ((uint8_t)RX_buf[1] == 0x0A)) { //데이터가 0xA0 0x0A로 시작하면 수신한 데이터의 맨 처음이다(헤더)
        for(i=2; i<14; i++) {
          check_sum += (uint8_t)RX_buf[i]; //Checksum을 위해 수신한 데이터를 모두 더함
        }
           if((uint8_t)RX_buf[4]==0x01){
        while((uint8_t)RX_buf[5]==0x00){
           controlBuzzer(0x01);
           if((uint8_t)RX_buf[12]==0x01) setForwardRight();
           else if((uint8_t)RX_buf[12]==0x02) setBackwardRight();
           else if((uint8_t)RX_buf[12]==0x03) setForwardLeft();
           else if((uint8_t)RX_buf[12]==0x04) setBackwardLeft();
           else setBackwardRight();
           runMotor(200);
           delay(90);
        }
      }
      else{
        controlBuzzer(0x00);
        stopMotor();
      }
        
        if(check_sum == (uint8_t)RX_buf[14]) { //Checksum 일치
          Serial.println("\n\r RX Packet data");
          
          for(i=0; i<17; i++) {
            Serial.write(' ');
            Serial.print((uint8_t)RX_buf[i],HEX); //16진수로 수신한 데이터 출력
          }
          
          TX_flag = RX_buf[4]; //데이터를 송신한 id를 TX_flag에 저장
          
          if(!TX_flag) { //데이터를 보낸 id가 0x00(서버)라면
            for(i=4; i<14; i++) {
              TX_buf[i] = 0; //TX_buf의 데이터 영역 초기화
            }
            
            TX_buf[14] = TX_buf[2]; //checksum 준비
            
            for(i=3; i<14; i++) {
              TX_buf[14] += TX_buf[i]; //checksum을 위해 데이터 더함
            }
            
            Serial.println("\n\r TX Packet data");
            
            for(i=0; i<17; i++) {
              Serial.write(' ');
              Serial.print(TX_buf[i],HEX);
              Serial1.write(TX_buf[i]); //데이터를 Serial과 Serial1에 모두 출력
            }
          }
        }
      }
    }
  }
}
void serialEvent(void) {
  Serial1.write(Serial.read());
}
 
void TIMER_ISR(void) {
  Timer_flag = 1;
}
void stopMotor(){
  digitalWrite(MOTOR_A_PINS[0], 0);
  digitalWrite(MOTOR_A_PINS[1], 0);
  digitalWrite(MOTOR_B_PINS[0], 0);
  digitalWrite(MOTOR_B_PINS[1], 0);
}
void setForwardRight() {
 digitalWrite(MOTOR_A_PINS[0], 1);
 digitalWrite(MOTOR_B_PINS[0], 0);
 digitalWrite(MOTOR_A_PINS[1], 0);
 digitalWrite(MOTOR_B_PINS[1], 0);
}
void setBackwardRight() {
 digitalWrite(MOTOR_A_PINS[0], 0);
 digitalWrite(MOTOR_B_PINS[0], 0);
 digitalWrite(MOTOR_A_PINS[1], 1);
 digitalWrite(MOTOR_B_PINS[1], 0);
}
void setForwardLeft() {
 digitalWrite(MOTOR_A_PINS[0], 0);
 digitalWrite(MOTOR_B_PINS[0], 1);
 digitalWrite(MOTOR_A_PINS[1], 0);
 digitalWrite(MOTOR_B_PINS[1], 0);
}
void setBackwardLeft() {
 digitalWrite(MOTOR_A_PINS[0], 0);
 digitalWrite(MOTOR_B_PINS[0], 0);
 digitalWrite(MOTOR_A_PINS[1], 0);
 digitalWrite(MOTOR_B_PINS[1], 1);
}
void runMotor(unsigned char speed) { // [0, 255]
 analogWrite(MOTOR_A_PINS[2], speed);
 analogWrite(MOTOR_B_PINS[2], speed);
} 
void controlBuzzer(uint8_t onOff){
  if(onOff) digitalWrite(BUZZER, HIGH);
  else digitalWrite(BUZZER, LOW);
}
