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
//RX_flag=0�� ���� �����͸� ������ �� ����. 1�� ���� �����͸� ������ �� ����. 2�� ����
//TX_flag�� �����͸� �۽��� ID�� ����(0x00: ����, �� ��: Ŭ���̾�Ʈ)
uint8_t RX_flag = 0, TX_flag = 0, Timer_flag = 0;
//EEPROM���� �׻� 0xAA�� ID ���� �����ؾ� �Ѵ�.
uint8_t EEPROM_buf[2] = {0xAA, 0};
//Wi-Fi shield packet info���� ���ǵ� ��Ŷ�� ���̴� �׻� 17byte
char RX_buf[17];
//Wi-Fi shield packet info���� header�� �׻� 0xA0, 0x0A�̸� tail�� �׻� 0x0A, 0xA0�̴�.
uint8_t TX_buf[17] = {0xA0, 0x0A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0xA0};
//RX_count�� ������ Wi-Fi ��� ������, ������ �����͸� ���� �� �ִ� �����̴�.
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
  
  //timer2 period: 200ms, ISR: TIMER_ISR ����
  MsTimer2::set(200, TIMER_ISR);
  
 //timer2 start
  MsTimer2::start();
}
void loop() {
  uint16_t i, tmp = 0;
  //pinmode�� serial �ʱ�ȭ�� ������ �ð����κ��� 4�� ���ϸ鼭 RX_falg�� 0�� ���
  if(((timer_check+4000) < millis()) && (RX_flag == 0)) {
    if(RX_count < 1) { //RX_count�� 0�̸� �ʱ����
      if(EEPROM.read(0) == 0xAA) { //EEPROM�� ù ��° ������ 0xAA��� ����ڰ� �ѹ��� EEPROM�� �������� �ʾҴ�.
        ID = EEPROM.read(1); //EEPROM�� ����� id�� �о�´�.
        RX_flag = 1; //�����͸� ������ �� �ִ� ���°� �ȴ�.
        
        Serial.print("\n\rID : ");
        Serial.println(ID);
        
        TX_buf[3] = ID; //TX_buf�� 4��° �ε����� id�� �����Ѵ�.
      } else {
        RX_flag = 2; //����
        
        Serial.println("\n\rWifi Connected Error");
        Serial.print("\n\rPlease reset the ADK-2560"); 
      }
    }
  }
  
  if(Timer_flag && TX_flag) { //Timer_flag �߻�, �� id�� ������ �ƴ϶��(������� 0x00->�翬�� if�� ��� ����)
    //������ ����
  
    
    Serial.println("\n\r TX Packet data");
    
    for(i=0; i<17; i++) {
      Serial.write(' ');
      Serial.print(TX_buf[i],HEX);
      Serial1.write(TX_buf[i]);
      //TX_buf�� ���� ������ serial(�����)�� serial1(�������� ���)�� ���
    }
    
    Timer_flag = 0;
  }
}
//serial1���� �о�� �����Ͱ� �����ϸ� �ڵ����� ����
void serialEvent1(void) {
  static char Check_buf[4] = {0, 0, 0, };
  uint8_t i,check_sum = 0, RX_cnt = 0;
  
  if(RX_flag == 0) { //�����͸� ������ �� ���� ��(���� setup()�Լ� �۵� ��)
    char da = Serial1.read(); //�����͸� �о��
    Serial.write(da); //�� �����͸� serial�� ����
    
    Check_buf[0] = Check_buf[1];
    Check_buf[1] = Check_buf[2];
    Check_buf[2] = da;
    //Serial1���� �о�� �����Ͱ� 'ATA'�̸�, RX ���ۿ��� �о�� Ƚ���� 0ȸ �� ��
    //ATA�� start auto connect, including association : Wi-Fi ����� IP, Subnet, Gateway ������ ����ϰ� : ���ڷ� �����Ѵ�.
    if((Check_buf[0] == 'A') && (Check_buf[1] == 'T') && (Check_buf[2] == 'A') && (RX_count == 0)) {
      RX_count = 1; 
    } else if(RX_count == 4) { //RX_count�� 4�� �� serial1���� �о�� �����Ϳ� :�� ������ ��ü���� ������ ���� ����
      if(Check_buf[2] != ':') {
        ID = ID*10 + (Check_buf[2]-'0');
      } else { //������ ������ RX_counter�� �������� �ƹ� �ൿ�� ����
        RX_count++; 
      }
    } else if(RX_count == 5) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'K')) { //Check_buf�� ���� OK] �� ��
          RX_flag = 1; //RX ������ ���� ����
          delay(1000);
          
          RX_cnt = Serial1.available(); //Serial1���� �о�� ������ ũ�⸦ RX_cnt�� ����
          Serial.println(RX_cnt); //Serial1���� �о�� ������ ũ�⸦ ����ڿ��� �˷���
          
          while(1) {
            Serial.write(Serial1.read()); //Serial1���� �����͸� �о�� Serial�� ����.
            RX_cnt--; //RX_cnt ����
            
            if(RX_cnt == 0) //���̻� �о�� �����Ͱ� ������ while�� Ż��
              break;
          }
    
          Serial.print("\n\rID : ");
          Serial.print(ID);
          
          EEPROM_buf[1] = ID;
          
          if(EEPROM.read(1) != ID) { //EEPROM �� ��° ������ ���� id�� �ٸ���(EEPROM�� ������ ������ ������)
            for(i=0; i<2; i++) {
              EEPROM.write(i,EEPROM_buf[i]); //EEPROM ù ��° ������ 0xAA, �� ��° ������ id ���
            }
          }
          
          TX_buf[3] = ID; //TX_buf �迭 3�� ��ġ id ����
        } else { //Check_buf�� ���� OK]�� �ƴϸ�(���� �߻�)
          RX_flag = 2; //����
          
          Serial.print("\n\rWifi Connected Error");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    } else if((Check_buf[2] == '.') && ((RX_count == 1) || (RX_count == 2) || (RX_count == 3))) { //Check_buf[2]�� . �� ���� ip Ȥ�� subnet Ȥ�� gatewaqy �ּ� ��� ����
      RX_count++;
    } else if(RX_count == 1) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'R')) { //������ ���
          RX_flag = 2;
          
          Serial.print("\n\rWifi Connected fail");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    }
    
  } else if(RX_flag == 1) { //�����͸� ������ �� ���� ��(loop() �Լ� �۵� ��)
    if(Serial1.available() > 16) { //Serial1���� �о�� ������ ũ�Ⱑ 16����Ʈ�� ������
      Serial1.readBytes(RX_buf, 17); //���� 17����Ʈ�� �о�� RX_buf�� ����
      
      if(((uint8_t)RX_buf[0] == 0xA0) && ((uint8_t)RX_buf[1] == 0x0A)) { //�����Ͱ� 0xA0 0x0A�� �����ϸ� ������ �������� �� ó���̴�(���)
        for(i=2; i<14; i++) {
          check_sum += (uint8_t)RX_buf[i]; //Checksum�� ���� ������ �����͸� ��� ����
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
        
        if(check_sum == (uint8_t)RX_buf[14]) { //Checksum ��ġ
          Serial.println("\n\r RX Packet data");
          
          for(i=0; i<17; i++) {
            Serial.write(' ');
            Serial.print((uint8_t)RX_buf[i],HEX); //16������ ������ ������ ���
          }
          
          TX_flag = RX_buf[4]; //�����͸� �۽��� id�� TX_flag�� ����
          
          if(!TX_flag) { //�����͸� ���� id�� 0x00(����)���
            for(i=4; i<14; i++) {
              TX_buf[i] = 0; //TX_buf�� ������ ���� �ʱ�ȭ
            }
            
            TX_buf[14] = TX_buf[2]; //checksum �غ�
            
            for(i=3; i<14; i++) {
              TX_buf[14] += TX_buf[i]; //checksum�� ���� ������ ����
            }
            
            Serial.println("\n\r TX Packet data");
            
            for(i=0; i<17; i++) {
              Serial.write(' ');
              Serial.print(TX_buf[i],HEX);
              Serial1.write(TX_buf[i]); //�����͸� Serial�� Serial1�� ��� ���
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
