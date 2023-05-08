#include <AccelStepper.h>
#include <Servo.h>
#include <ezButton.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h> // Touch Scrren library

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

#define MINPRESSURE 10
#define MAXPRESSURE 1000

//Defining Touch Screen Pins
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 23   // can be a digital pin
#define XP 22   // can be a digital pin

//
//yminus = D1
//Xplus = D0

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//Defining Analong Pins
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).
//   D0 connects to digital pin 22
//   D1 connects to digital pin 23
//   D2 connects to digital pin 24
//   D3 connects to digital pin 25
//   D4 connects to digital pin 26
//   D5 connects to digital pin 27
//   D6 connects to digital pin 28
//   D7 connects to digital pin 29

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GOLD    0xFEA0
#define SILVER    0xC618
#define LIME    0x07E0


Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

//VARIABLE DECLARATIONS
int currentpage = 0; // 0 - Start Screen
//int currentpage = 5; // 0 - Start Screen

int flength = 22, save_flength;
float d_centre = 3, save_d_centre;
float h_object = 5, save_h_object;
float d_object = 0, save_d_object;
float t_smartphone = 0, save_t_smartphone;

int plus_1_x = -10;
int plus_2_x = 60;
int plus_1_y = 111;
int plus_2_y = 160;
int minus_1_x = -10;//same as plus_1_x
int minus_2_x = 60;//same as plus_2_x
int minus_1_y = 60;
int minus_2_y = 99;

int back_1_x = 240;
int back_2_x = 320;
int back_1_y = -10;
int back_2_y = 30;
int conf_1_x = 240;//same as back_1_x
int conf_2_x = 320;//same as back_2_x
int conf_1_y = 190;
int conf_2_y = 230;


#define MAX_POSITION 0x7FFFFFFF // maximum of position we can set (long type)
#define PI 3.1415926535897932384626433832795
#define sensor_width 36 //sensor width for a full frame camera
#define sensor_height 24 //sensor height for a full frame camera

Servo myservo;

ezButton limitSwitch(A5);
ezButton limitSwitch2(A6);
int trig = 8;

#define servoPin 3
#define MS0_1 37
#define MS1_1 36
#define MS2_1 34

#define MS0_2 41
#define MS1_2 40
#define MS2_2 42

#define MS0_3 45
#define MS1_3 46
#define MS2_3 47

#define MS0_4 51
#define MS1_4 50
#define MS2_4 52


//Counters for Steppers
double step_z =0; //vertical gantry steps
double ref_z; //reference position of gantry 
double step_x =0; //horizontal turntable steps
double step_rotate=0; //turntable steps
double pitch = 0.2; //lead screw pitch in cm

//Position Variables
double x_max = 31.5;// max x_position (turntable)
double x_min = 10;
double z_max = 24.5;// max z_position of gantry
double x_position; // height of smartphone gantry
double z_position; // horizontal distance of turntable

//Camera and Object Parameters
double AOV_width; // angle of view for the width of the field of view
double AOV_height; //angle of view for the height of the field of view
double focal_length; //equivalent focal length of smartphone camera

double camera_distance; // distance from center of phone to camera
double object_diameter;
double object_height;
double R = 5.2;
double t;




// Define the stepper motor and the pins that is connected to
AccelStepper stepper1(1,32, 33); // (Typeof driver: with 2 pins, STEP, DIR)
AccelStepper stepper3(1, 35, 44);
AccelStepper stepper4(1, 38, 39);

void image_aquisition(double view_angle, double min_distance)
{

  z_position = object_height/2.0 + (min_distance)*sin(view_angle) - camera_distance*cos(view_angle) + (R+t)*sin(view_angle);
  x_position =  (min_distance)*cos(view_angle) + camera_distance*sin(view_angle) + (R+t)*cos(view_angle);

  if (z_position > z_max){
    z_position = z_max;
    x_position = x_position - 6;
  }

  if (x_position > x_max){
    x_position = x_max;
  }

  if (x_position < x_min){
    x_position = x_min;
  }

  if (view_angle == 0){
     if (x_position < 23){
    x_position = 23;
     }
  }

  step_z = round(3200*(z_position/pitch));
  step_x = round(3200*(x_position-x_max)/pitch);
}

void calc_AOV(){
  AOV_width = 2*atan(sensor_width/(2*focal_length));
  AOV_height = 2*atan(sensor_height/(2*focal_length));
}

float calc_minimum_distance(double view_angle){
  
  double min_distance_width;
  double min_distance_height;
  double minimum_distance;
  double d1,d2;
  double alpha;
  double beta;
  double gamma;
  double length_AC;
  double length_AB;
  double length_AD;
  double length_DC;
  double length_AE;

  alpha = AOV_height/2;
  beta = view_angle;

  //Case 1: FOV Intersects Top Corner of Object
  length_AC = sqrt((object_height/2)*(object_height/2) + (object_diameter/2)*(object_diameter/2));
  gamma = atan(object_height/object_diameter) - (PI/2-beta);
  length_AB = sin(PI/2 + alpha - gamma)/((sin(PI/2-alpha)/length_AC));
  d1 = length_AB/tan(alpha);

  //Case 2: FOV Intersects Lower Corner of Object
  length_AD = sqrt(pow(object_height/2,2) + pow(object_diameter/2,2));
  gamma = (PI/2)-atan(object_height/object_diameter) - beta;
  d2 = (sin(PI/2-gamma+alpha)/((sin(PI/2-alpha))/length_AD))/tan(alpha);

  if (d1>d2){
      min_distance_height = d1;
  }
  else{
      min_distance_height = d2;
  }

  min_distance_width = (object_diameter/2)/(tan(AOV_width/2)*cos(AOV_width/2));

  if (min_distance_height<min_distance_width){
    minimum_distance = min_distance_width;
  }
  else{
    minimum_distance = min_distance_height;
  }

  

  return minimum_distance;
  
}


void setup() {
  Serial.begin(115200);
   tft.reset();

  uint16_t identifier = tft.readID();
  limitSwitch.setDebounceTime(50);
  limitSwitch2.setDebounceTime(50);
  tft.begin(identifier);
  tft.setRotation(3);
  //diameter_of_object();
  start_button();

  pinMode(MS0_4, OUTPUT);
  digitalWrite(MS0_4,HIGH);
  pinMode(MS1_4, OUTPUT);
  digitalWrite(MS1_4,HIGH);
  pinMode(MS2_4, OUTPUT);
  digitalWrite(MS2_4,HIGH);

  pinMode(MS0_1, OUTPUT);
  digitalWrite(MS0_1,LOW);
  pinMode(MS1_1, OUTPUT);
  digitalWrite(MS1_1,LOW);
  pinMode(MS2_1, OUTPUT);
  digitalWrite(MS2_1,HIGH);

  pinMode(MS0_2, OUTPUT);
  digitalWrite(MS0_2,LOW);
  pinMode(MS1_2, OUTPUT);
  digitalWrite(MS1_2,LOW);
  pinMode(MS2_2, OUTPUT);
  digitalWrite(MS2_2,HIGH);

  pinMode(MS0_3, OUTPUT);
  digitalWrite(MS0_3,LOW);
  pinMode(MS1_3, OUTPUT);
  digitalWrite(MS1_3,LOW);
  pinMode(MS2_3, OUTPUT);
  digitalWrite(MS2_3,HIGH);

  myservo.attach(servoPin);
  myservo.write(90);
  stepper1.setMaxSpeed(2500*16); // Set maximum speed value for the stepper
  stepper1.setAcceleration(200*4); // Set acceleration value for the stepper
  stepper1.setCurrentPosition(0);
   // Set the current position to 0 steps

  stepper3.setMaxSpeed(2500*16); // Set maximum speed value for the stepper
  stepper3.setAcceleration(200*4); // Set acceleration value for the stepper
  stepper3.setCurrentPosition(0); // Set the current position to 0 steps

  stepper4.setMaxSpeed(6400);
  stepper4.setAcceleration(1000);
  stepper4.setCurrentPosition(0);

//Check steppers
stepper1.moveTo(3200);
stepper3.moveTo(-3200);

stepper1.runToPosition();
stepper3.runToPosition();

stepper1.setCurrentPosition(0);
stepper3.setCurrentPosition(0);


//Reset Scanner Position
stepper1.moveTo(-MAX_POSITION);
stepper3.moveTo(MAX_POSITION);



while(true){
  limitSwitch2.loop();
  if (limitSwitch2.isPressed()) {
    stepper3.moveTo(-MAX_POSITION);
    stepper3.run();
    stepper3.run();
    break;
  }
  else{
    stepper3.run();
  }
}

while(true){
  limitSwitch.loop();
  if (limitSwitch.isPressed()){
    stepper1.moveTo(MAX_POSITION);
    stepper1.run();
    stepper1.run();
    break;
  }
  else{
    stepper1.run();
  }
}

stepper1.setCurrentPosition(0);
stepper3.setCurrentPosition(0);

}


void loop() {
  //reading touch sensor
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
 
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
 
  p.x = map(p.x, TS_MAXX, TS_MINX, tft.width(), 0);
  p.y = map(p.y, TS_MAXY, TS_MINY, tft.height(), 0);
  if (currentpage == 0){//START SCREEN

    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
/*
      Serial.print("X = "); Serial.print(p.x);
      Serial.print("\tY = "); Serial.print(p.y);
      Serial.print("\tPressure = "); Serial.println(p.z);
*/
      if (p.x > 50 && p.x < 250 && p.y > 20 && p.y < 200){
        tft.fillScreen(BLACK);
        focal_length_screen(); //LOAD FOCAL LENGTH SCREEN
        currentpage = 1; //GO TO FOCAL LENGTH SCREEN
        delay (600);
      }
    }
  }

  if (currentpage == 1){//FOCAL LENGTH SCREEN
    delay(100);
    
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {  
      
      if (p.x > plus_1_x && p.x < plus_2_x && p.y > plus_1_y && p.y < plus_2_y){
        //Serial.println("Plus");
        flength++;
      }  
      if (p.x > minus_1_x && p.x < minus_2_x && p.y > minus_1_y && p.y < minus_2_y){
        //Serial.println("Minus");
        flength--;
      }

      if (flength >= 56){
        flength = 56;  
      }
      if (flength <= 10){
        flength = 10;  
      }
      num_cover();
      tft.setCursor(150, 130);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println(flength);
      mm();
      
      if (p.x > back_1_x && p.x < back_2_x && p.y > back_1_y && p.y < back_2_y){//RETURN TO START SCREEN
  
        tft.fillScreen(BLACK);
        start_button();
        flength = 22;
        currentpage = 0; //BACK TO START SCREEN
      }
      if (p.x > conf_1_x && p.x < conf_2_x && p.y > conf_1_y && p.y < conf_2_y){//GO TO DISTANCE FROM CENTRE SCREEN
        save_flength = flength;//SAVED FOCAL LENGTH VARIABLE
        tft.fillScreen(BLACK);
        p.x = 0;
        p.y = 0;
        distance_from_centre();
        currentpage = 2; //GO TO DISTANCE FROM SCREEN
        delay (600);
      }
    }
  }


  if (currentpage == 2){//DISTANCE FROM CENTRE SCREEN
   
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {  
      if (p.x > plus_1_x && p.x < plus_2_x && p.y > plus_1_y && p.y < plus_2_y){
        d_centre += 0.1;
      }  
      if (p.x > minus_1_x && p.x < minus_2_x && p.y > minus_1_y && p.y < minus_2_y){
        d_centre -= 0.1;
      }
      if (d_centre >= 3){//MAX DISTANCE FROM CENTRE
        d_centre = 3;  
      }
      if (d_centre <= 0){//MIN DISTANCE FROM CENTRE
        d_centre = 0;  
      }
      num_cover();
      tft.setCursor(120, 130);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println(d_centre);
      cm();

      if (p.x > back_1_x && p.x < back_2_x && p.y > back_1_y && p.y < back_2_y){//RETURN TO FOCAL LENGTH SCREEN 
        tft.fillScreen(BLACK);
        focal_length_screen();
        d_centre = 3;
        currentpage = 1; //BACK TO FOCAL LENGTH SCREEN
      }
      if (p.x > conf_1_x && p.x < conf_2_x && p.y > conf_1_y && p.y < conf_2_y){//GO TO SMARTPHONE THICKNESS SCREEN
        save_d_centre = d_centre;//SAVED DISTANCE FROM CENTRE
        p.x = 0;
        p.y = 0;
        smartphone_thickness();
        currentpage = 3; //GO TO SMARTPHONE THICKNESS SCREEN
        delay (600);
      }
    }
  }

if (currentpage == 3){//SMARTPHONE THICKNESS SCREEN
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {  
      if (p.x > plus_1_x && p.x < plus_2_x && p.y > plus_1_y && p.y < plus_2_y){
        t_smartphone += 0.1;
      }  
      if (p.x > minus_1_x && p.x < minus_2_x && p.y > minus_1_y && p.y < minus_2_y){
        t_smartphone -= 0.1;
      }
      if (t_smartphone >= 2){//MAX SMARTPHONE THICKNESS
        t_smartphone = 2;  
      }
      if (t_smartphone <= 0){//MIN SMARTPHONE THICKNESS
        t_smartphone = 0;  
      }
      num_cover();
      tft.setCursor(120, 130);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println(t_smartphone);
      cm();

      if (p.x > back_1_x && p.x < back_2_x && p.y > back_1_y && p.y < back_2_y){//RETURN TO HEIGHT OF OBJECT SCREEN 
        tft.fillScreen(BLACK);
        distance_from_centre();
        t_smartphone = 0;
        currentpage = 2; //BACK TO DISTANCE FROM CENTRE SCREEN
      }
      if (p.x > conf_1_x && p.x < conf_2_x && p.y > conf_1_y && p.y < conf_2_y){//GO TO HEIGHT OF OBJECT SCREEN
        save_t_smartphone = t_smartphone;//SAVED SMARTPHONE THICKNESS
        p.x = 0;
        p.y = 0;
        height_of_object();
        currentpage = 4; //
      }
    }
  } 


  if (currentpage == 4){//HEIGHT OF OBJECT SCREEN
   
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {  
      if (p.x > plus_1_x && p.x < plus_2_x && p.y > plus_1_y && p.y < plus_2_y){
        h_object += 0.1;
      }  
      if (p.x > minus_1_x && p.x < minus_2_x && p.y > minus_1_y && p.y < minus_2_y){
        h_object -= 0.1;
      }
      if (h_object >= 25){//MAX HEIGHT OF OBJECT
        h_object = 25;  
      }
      if (h_object <= 5){//MIN HEIGHT OF OBJECT
        h_object = 5;  
      }
      num_cover();
      tft.setCursor(120, 130);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println(h_object);
      cm();

      if (p.x > back_1_x && p.x < back_2_x && p.y > back_1_y && p.y < back_2_y){//RETURN TO SMARTPHONE THICKNESS SCREEN 
        tft.fillScreen(BLACK);
        smartphone_thickness();
        h_object = 5;
        currentpage = 3; //BACK TO SMARTPHONE THICKNESS SCREEN
      }
      if (p.x > conf_1_x && p.x < conf_2_x && p.y > conf_1_y && p.y < conf_2_y){//GO TO DIAMETER OF OBJECT SCREEN
        save_h_object = h_object;//SAVED HEIGHT OF OBJECT
        p.x = 0;
        p.y = 0;
        diameter_of_object();
        currentpage = 5; //GO TO DIAMETER OF OBJECT
        delay (600);
      }
    }
  }

  if (currentpage == 5){//DIAMETER OF OBJECT SCREEN 
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {  
      if (p.x > plus_1_x && p.x < plus_2_x && p.y > plus_1_y && p.y < plus_2_y){
        d_object += 0.1;
      }  
      if (p.x > minus_1_x && p.x < minus_2_x && p.y > minus_1_y && p.y < minus_2_y){
        d_object -= 0.1;
      }
      if (d_object >= 30){//MAX DIAMETER OF OBJECT
        d_object = 30;  
      }
      if (d_object <= 0){//MIN DIAMETER OF OBJECT
        d_object = 0;  
      }
      num_cover();
      tft.setCursor(120, 130);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println(d_object);
      cm();

      if (p.x > back_1_x && p.x < back_2_x && p.y > back_1_y && p.y < back_2_y){//RETURN TO HEIGHT OF OBJECT SCREEN 
        tft.fillScreen(BLACK);
        height_of_object();
        d_object = 0;
        currentpage = 4; //BACK TO HEIGHT OF OBJECT SCREEN
      }
      if (p.x > conf_1_x && p.x < conf_2_x && p.y > conf_1_y && p.y < conf_2_y){//GO TO SCANNING
        save_d_object =d_object;//
        p.x = 0;
        p.y = 0;
        start_scanning();
        currentpage = 6; //
      }
    }
  }

  if (currentpage == 6){
    int i;
  double min_distance;
  double z_position; //gantry height
  double view_angles[4] = {0,5.0*PI/36.0,PI/4,(11.0/36.0)*PI};

  focal_length = flength;

  if (focal_length<13){
    focal_length =13;
  }
  object_height = h_object;
  object_diameter = d_object;
  t = t_smartphone;
  camera_distance = d_centre;
  
   // focal_length = 28.0;
  // object_height = 14.0;
  // object_diameter = 20.0;
  // camera_distance = 2.70;
  // t = 0.8;`

  // focal_length = 13.0;
  // object_height = 14.0;
  // object_diameter = 20.0;
  // camera_distance = 1.0;
  // t = 0.8;

  //reset_scanner
  myservo.write(90);
  stepper1.moveTo(0);
  stepper3.moveTo(0);
  stepper3.runToPosition();
  stepper1.runToPosition();


  //aligns gantry with centre of object
  if (camera_distance<object_height/2){
    double z_position = object_height/2 - camera_distance;
    ref_z = round((z_position/pitch)*3200);
    stepper1.moveTo(ref_z);
    stepper1.runToPosition();
  }

  calc_AOV();

  for (i =0; i<4; i++ ){
    min_distance = calc_minimum_distance(view_angles[i]);
    image_aquisition(view_angles[i], min_distance);


    //Run Steppers and Set Camera Position

    myservo.write(int(90 - 8 + view_angles[i]*180/PI));
 
    stepper1.moveTo(step_z);
    stepper3.moveTo(step_x);
   
    stepper1.runToPosition();
    stepper3.runToPosition();

    
    for (int j = 1; j<=25;j++){
      step_rotate = step_rotate + 256;
      stepper4.moveTo(step_rotate);
      stepper4.runToPosition();

      digitalWrite(trig,HIGH);
      delay(5);
      digitalWrite(trig,LOW);
      delay(2000);
    }
  }
  Serial.println("Done Scanning");
  Serial.println("Done Scanning");
   //reset_scanner
  myservo.write(90);
  stepper1.moveTo(0);
  stepper3.moveTo(0);
  stepper3.runToPosition();
  stepper1.runToPosition();
  Serial.println("Done Scanning");
  Serial.println("Done Scanning");
  scan_again();
  currentpage = 7;
  }

if (currentpage == 7){//SCAN AGAIN SCREEN
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      if (p.x > 50 && p.x < 250 && p.y > 20 && p.y < 200){
        tft.fillScreen(BLACK);
        height_of_object(); //LOAD FOCAL LENGTH SCREEN
        currentpage = 4; //GO TO FOCAL LENGTH SCREEN
        Serial.println("Done Scanning");
        Serial.println("Done Scanning");
        Serial.println("Done Scanning");
        delay (600);
      }
    }
  }  
}

void please_enter(){
    tft.setCursor(80, 18);
    tft.setTextColor(GOLD);
    tft.setTextSize(2);
    tft.println("Please Enter:");
    return;
}
void back_button(){
    tft.fillRect(0, 0, 46, 46, WHITE);
    tft.fillRect(2, 2, 42, 42, SILVER);
    tft.fillTriangle(8, 22, 16, 14, 16, 30, WHITE);
    tft.fillRect(16, 20, 18, 6, WHITE);
   
}
void plus_minus(){
    tft.fillRect(160, 180, 60, 60, WHITE);
    tft.fillRect(165, 185, 50, 50, RED);
    tft.fillRect(188, 195, 5, 30, WHITE);
    tft.fillRect(175, 207, 30, 5, WHITE);

    tft.fillRect(100, 180, 60, 60, WHITE);
    tft.fillRect(105, 185, 50, 50, BLUE);
    tft.fillRect(115, 207, 30, 5, WHITE);    
    return;
}
void confirm_button(){
    tft.fillRect(274, 0, 46, 46, WHITE);
    tft.fillRect(276, 2, 42, 42, GREEN);
    tft.fillTriangle(284, 26, 287, 23, 294, 34, WHITE);
    tft.fillTriangle(297, 29, 288, 22, 294, 34, WHITE);
    tft.fillTriangle(298, 30, 308, 12, 311, 15, WHITE);
    tft.fillTriangle(298, 30, 308, 12, 295, 27, WHITE);
   
}
void num_cover(){
  tft.fillRect(65, 115, 200, 60, BLACK);
}
void start_button(){
  tft.fillScreen(BLACK);
  tft.fillRoundRect(50, 50, 220, 140, 20, BLUE);
  tft.drawRoundRect(50, 50, 220, 140, 20, WHITE);
 
  tft.setCursor(90, 105);
  tft.setTextColor(GOLD);
  tft.setTextSize(5);
  tft.println("START");
  return;
}

void scan_again(){
  tft.fillScreen(BLACK);
  tft.fillRoundRect(50, 50, 220, 140, 20, BLUE);
  tft.drawRoundRect(50, 50, 220, 140, 20, WHITE);
 
  tft.setCursor(115, 85);
  tft.setTextColor(GOLD);
  tft.setTextSize(4);
  tft.println("SCAN");
  tft.setCursor(105, 125);
  tft.println("AGAIN");
  return;
}

void mm(){
    tft.setCursor(220, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println("mm");
    return;  
}

void cm(){
    tft.setCursor(220, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println("cm");
    return;  
}
void focal_length_screen(){

    tft.fillScreen(BLACK);
    plus_minus();
    please_enter();
    back_button();
    confirm_button();
   
    tft.fillRoundRect(70, 50, 180, 60, 20, SILVER);
    tft.drawRoundRect(70, 50, 180, 60, 20, WHITE);
    tft.setCursor(89, 74);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("FOCAL LENGTH");

    num_cover();
    tft.setCursor(150, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println(flength);
    mm();
    
    return;
}
void distance_from_centre(){

    tft.fillScreen(BLACK);
    plus_minus();
    please_enter();
    back_button();
    confirm_button();

    
    tft.fillRoundRect(6, 50, 308, 60, 16, SILVER);
    tft.drawRoundRect(6, 50, 308, 60, 16, WHITE);
    tft.setCursor(18, 64);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("DISTANCE FROM THE CENTRE");
    tft.setCursor(10, 82);
    tft.println(" OF YOUR PHONE TO CAMERA");

    num_cover();
    tft.setCursor(120, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println(d_centre);
    cm();
    return;
}

void height_of_object(){
    
    tft.fillScreen(BLACK);
    plus_minus();
    please_enter();
    back_button();
    confirm_button();
    
    tft.fillRoundRect(16, 50, 288, 60, 16, SILVER);
    tft.drawRoundRect(16, 50, 288, 60, 16, WHITE);
    tft.setCursor(40, 74);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("HEIGHT OF THE OBJECT");

    num_cover();
    tft.setCursor(120, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println(h_object);
    cm();
    return;
}

void diameter_of_object(){
    
    tft.fillScreen(BLACK);
    plus_minus();
    please_enter();
    back_button();
    confirm_button();
    
    tft.fillRoundRect(6, 50, 308, 60, 16, SILVER);
    tft.drawRoundRect(6, 50, 308, 60, 16, WHITE);
    tft.setCursor(28, 74);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("DIAMETER OF THE OBJECT");

    num_cover();
    tft.setCursor(120, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println(d_object);
    cm();
    return;
}

void smartphone_thickness(){
    
    tft.fillScreen(BLACK);
    plus_minus();
    please_enter();
    back_button();
    confirm_button();
    
    tft.fillRoundRect(6, 50, 308, 60, 16, SILVER);
    tft.drawRoundRect(6, 50, 308, 60, 16, WHITE);
    tft.setCursor(28, 74);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("SMARTPHONE THICKNESS");

    num_cover();
    tft.setCursor(120, 130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.println(t_smartphone);
    cm();
    return;
}

void start_scanning(){
    int i;
    int box = 20;
    
    tft.fillScreen(BLACK);
    tft.setCursor(40, 40);
    tft.setTextColor(WHITE);
    tft.setTextSize(5);
    tft.println("SCANNING");

    // for(i=2; i <= 6; i+=2){
    //   delay(800);
    //   tft.fillRect(70+box*i, 100, box, box, WHITE);

    //   if(i == 6){
    //     delay(800);
    //     i = 0;
    //     tft.fillRect(100, 90, 180, 40, BLACK);
    //   }
    // }
    return;
}
