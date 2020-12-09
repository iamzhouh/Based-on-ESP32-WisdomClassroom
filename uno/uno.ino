#define LED1 13
#define LED2 12
int val0;
int val1;
void setup()
{
  Serial.begin(9600); // open serial port, set the baud rate to 9600 bps
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
}

void loop()
{
  val0=analogRead(A0); 
  if (500>val0>0)  
  {
    digitalWrite(LED1,HIGH);
    //Serial.println(val0);
  }
  else
  {
    digitalWrite(LED1,LOW);
    //Serial.println(val0);
  }       



  val1=analogRead(A1); 
  if (500>val1>0)  
  {
    digitalWrite(LED2,HIGH);
    //Serial.println(val1);
  }
  else
  {
    digitalWrite(LED2,LOW);
    //Serial.println(val1);
  }       
  
}
