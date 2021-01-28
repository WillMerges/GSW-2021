import at.mukprojects.console.*;
import controlP5.*;

ControlP5 cp5;

Console console;
Chart imu_accel;
Textlabel imu_lab;
Chart altitude;
Textlabel alt_lab;

// charge deployed
boolean c1;
boolean c2;
boolean c3;
boolean c4;
// continuity
boolean cc1;
boolean cc2;
boolean cc3;
boolean cc4;

Textlabel deployed_lab;
Textlabel cont_lab;

Toggle[] charge_toggles;
Toggle[] cont_toggles;

String GSW_HOME = "";

void setup() {
  //if(args != null && args.length == 1) {
  //  GSW_HOME = args[0];
  //} else {
  //  println("Must run as UI [GSW_HOME location]");
  //  exit();
  //} 
  
  size(720, 480);
  surface.setTitle("GSW Blackout");
  surface.setResizable(true);
  
  cp5 = new ControlP5(this);
  
  console = new Console(this);
  console.start();
  
  println("Starting GSW UI");
  //println("GSW_HOME set to " + GSW_HOME);

  // charges
  c1 = false;
  c2 = false;
  c3 = false;
  c4 = false;
  cc1 = true;
  cc2 = true;
  cc3 = true;
  cc4 = true;
  
  deployed_lab = cp5.addTextlabel("labelcharge")
               .setText("Deployed")
               .setPosition(width-810, height-470)
               .setColorValue(color(0,0,0))
               .setFont(createFont("Ubuntu", 20))
               ;
  
  cont_lab = cp5.addTextlabel("labelcont")
               .setText("Continuity")
               .setPosition(width-810, height-400)
               .setColorValue(color(0,0,0))
               .setFont(createFont("Ubuntu", 20))
               ;
  
  charge_toggles = new Toggle[4];
  cont_toggles = new Toggle[4];
  
  int j = 0;
  charge_toggles[j] = cp5.addToggle("c1").setPosition(width-700+(170*j), height-470)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  cont_toggles[j] = cp5.addToggle("cc1").setPosition(width-700+(170*j), height-400)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  j++;
  charge_toggles[j] = cp5.addToggle("c2").setPosition(width-700+(170*j), height-470)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  cont_toggles[j] = cp5.addToggle("cc2").setPosition(width-700+(170*j), height-400)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  j++;
  charge_toggles[j] = cp5.addToggle("c3").setPosition(width-700+(170*j), height-470)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  cont_toggles[j] = cp5.addToggle("cc3").setPosition(width-700+(170*j), height-400)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  j++;
  charge_toggles[j] = cp5.addToggle("c4").setPosition(width-700+(170*j), height-470)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  cont_toggles[j] = cp5.addToggle("cc4").setPosition(width-700+(170*j), height-400)
                         .setSize(150,50)
                         .setColorBackground(color(216,54,46))
                         .setColorActive(color(49,173,33))
                         .setColorCaptionLabel(color(200,200,200))
                         ;
  

  // imu
  imu_lab = cp5.addTextlabel("label")
               .setText("IMU Acceleration (X,Y,Z)")
               .setPosition(15, 27)
               .setColorValue(color(0,0,0))
               .setFont(createFont("Ubuntu", 20))
               ;

  imu_accel = cp5.addChart("imu accel")
                  .setPosition(10,52)
                  .setSize(500,300)
                  .setView(Chart.LINE)
                  .setStrokeWeight(15)
                  .setColorBackground(color(150,150,150))
                  .setColorCaptionLabel(color(200,200,200)) // make sure this is the same as the background color to hide the label
                  .setRange(-20,20)
                  ;
                  
   imu_accel.addDataSet("x");
   imu_accel.setData("x", new float[100]);
   imu_accel.setColors("x", color(255,0,0));
   
   imu_accel.addDataSet("y");
   imu_accel.setData("y", new float[100]);
   imu_accel.setColors("y", color(0,255,0));
   
   imu_accel.addDataSet("z");
   imu_accel.setData("z", new float[100]);
   imu_accel.setColors("z", color(0,0,255));
   
   imu_accel.addDataSet("zero_ref");
   float[] zero_accel = new float[100];
   for(int i = 0; i < 100; i++) {
     zero_accel[i] = 0;
   }
   imu_accel.setData("zero_ref", zero_accel);
   imu_accel.setColors("zero_ref", color(0,0,0));

  
   // alt
   alt_lab = cp5.addTextlabel("label2")
                 .setText("Relative Altitude (ft)")
                 .setPosition(15, 380)
                 .setColorValue(color(0,0,0))
                 .setFont(createFont("Ubuntu", 20))
                 ;
  
   altitude = cp5.addChart("altitude")
                  .setPosition(10,405)
                  .setSize(500,300)
                  .setView(Chart.LINE)
                  .setStrokeWeight(15)
                  .setColorCaptionLabel(color(200,200,200)) // make sure this is the same as the background color to hide the labe
                  .setRange(-1,120)
                  .setColorBackground(color(150,150,150))
                  ;
                  
   altitude.addDataSet("altitude");
   altitude.setData("altitude", new float[100]);
   altitude.setColors("altitude", color(0,0,255));
   
   altitude.addDataSet("max_altitude");
   float[] zero_alt = new float[100];
   for(int i = 0; i < 100; i++) {
     zero_alt[i] = 0;
   }
   altitude.setData("max_altitude", zero_alt);
   altitude.setColors("max_altitude", color(0,0,0));
}

float alt = 0;
float max_alt = 0;
float hard_max_alt = 100; // for simulation only
float x = 0;

int charge_count = 0;
int time_to_deploy = 20;
float main_deploy_alt = 20;

boolean apogee = false;

void draw() {
  background(color(200,200,200));
  
  // console
  console.draw(0, height - 320, width, height, 18, 18, 4, 4, color(220), color(0), color(255, 255, 0));
  
  // imu
  imu_lab.draw(this);
  
  imu_accel.push("x", random(-1,1));
  imu_accel.push("y", random(-1,1));
  imu_accel.push("z", 9.8 + random(-1,1));
  
  // altitude
  alt_lab.draw(this);
  
  alt = -((x-10.0)*(x-10.0)) + 100 + random(-1, 1);
  x += 0.035;
  if(alt > max_alt) {
    max_alt = alt;
  }
  altitude.push("altitude", alt);
  
  float[] max = new float[100];
  for(int i = 0; i < 100; i++) {
    max[i] = max_alt;
  }
  altitude.setData("max_altitude", max);

  if(alt >= hard_max_alt-1 && !apogee) {
    print("Apogee detected at: ");
    print(alt);
    print(" ft\n");
    apogee = true;
  }

  if(alt < 0) {
    max_alt = 0;
    x = 0;
  }
  
  // charges
  if(apogee && !c1 && !c3) {
    if(charge_count++ > time_to_deploy) { 
      println("Deploying charge [1]");
      println("Deploying charge [3]");
      println("Lost continuity on line [1]");
      println("Lost continuity on line [3]");
      c1 = true;
      c3 = true;
      cc1 = false;
      cc3 = false;
    }
  }
  
  if(apogee && !c2 && !c4) {
    if(alt < main_deploy_alt) {
      println("Deploying charge [2]");
      println("Deploying charge [4]");
      println("Lost continuity on line [2]");
      println("Lost continuity on line [4]");
      c2 = true;
      c4 = true;
      cc2 = false;
      cc4 = false;
    }
  }
  
  int j = 0;
  charge_toggles[j].setValue(c1);
  cont_toggles[j++].setValue(cc1);
  charge_toggles[j].setValue(c2);
  cont_toggles[j++].setValue(cc2);
  charge_toggles[j].setValue(c3);
  cont_toggles[j++].setValue(cc3);
  charge_toggles[j].setValue(c4);
  cont_toggles[j++].setValue(cc4);
  
  deployed_lab.draw(this);
  cont_lab.draw(this);
  
}
