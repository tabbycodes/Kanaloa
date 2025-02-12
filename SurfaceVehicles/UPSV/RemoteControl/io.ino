// Read kill state inputs 
void readKill() {

  // Read kill inputs
  ch7Pulse = pulseIn(ch7Pin,HIGH);              // ch 7 (remote kill switch channel)
  int physKillRaw1 = digitalRead(physKillPin1);   // physical kill switch channel
  int physKillRaw2 = digitalRead(physKillPin2); 
  
  // Determine remote kill swtich state (ch7)
  if (ch7Pulse > ch7PulseMax - pulseTolerance && ch7Pulse < ch7PulseMax + pulseTolerance) {
    remKillStatus = 0;      // unkill
  }
  else if (ch7Pulse > ch7PulseMin - pulseTolerance && ch7Pulse < ch7PulseMin + pulseTolerance) {
    remKillStatus = 1;      // kill
  }
  else { 
    Serial.println("Error reading remote kill swtich signal (ch7) from receiver...killing");
    remKillStatus = 2;      // error
  }

  // Determine physical kill swtich state (from physical kill buttons
  if (physKillRaw1 == 1 || physKillRaw2 == 1) {
    Serial.println(physKillRaw1);
    Serial.println(physKillRaw2);
    physKillStatus = 0;     // unkill
  }
  else if (physKillRaw1 == 0 && physKillRaw2 == 0) {
    Serial.println(physKillRaw1);
    Serial.println(physKillRaw2);
    physKillStatus = 1;     // kill
  }
  else {
    Serial.println("Error reading remote physical kill state...this state should be impossible to trip");
    Serial.println(physKillRaw1);
    Serial.println(physKillRaw2);
    physKillStatus = 2;     // error
  }

  // Determine kill status
  if (remKillStatus == 0 && physKillStatus == 0) {
    killStatus = 0;     // unkill
  }
  else {
    killStatus = 1;     // kill
  }
  
}

// Read joystick inputs
void joy2Setpoint() {
  
  // Read joystick inputs
  ch1Pulse = pulseIn(ch1Pin,HIGH);    // ch 1 (surge joystick channel)
  // ch2Pulse = pulseIn(ch2Pin,HIGH);    // ch 2 (sway joystick channel)
  ch3Pulse = pulseIn(ch3Pin,HIGH);    // ch 3 (yaw joystick channel)

  // Map joystick inputs from -1000 to 1000
  int ch1Map = map(ch1Pulse,ch1PulseMin,ch1PulseMax,-1000,1000);
  // int ch2Map = map(ch2Pulse,ch2PulseMin,ch2PulseMax,-1000,1000);
  int ch3Map = map(ch3Pulse,ch3PulseMin,ch3PulseMax,-1000,1000);

  // Remove the deadzone
  if (ch1Map < pulseDeadzone && ch1Map > -pulseDeadzone) {
    ch1Map = 0;
  }
 // if (ch2Map < pulseDeadzone && ch2Map > -pulseDeadzone) {
 //   ch2Map = 0;
 // }
  if (ch3Map < pulseDeadzone && ch3Map > -pulseDeadzone) {
    ch3Map = 0;
  }

  // Calculate surge sway and yaw components
  int surgePort = ch1Map;   // surge (forward positive)
  int surgeStar = ch1Map;
  // int surgeQ3 = ch1Map;
  // int surgeQ4 = ch1Map;
  int swayPort = 0;   // sway (right positive)
  int swayStar = 0;
  //int swayQ3 = -ch2Map;
  //int swayQ4 = ch2Map;
  int yawPort = ch3Map;    // yaw (CLOCKWISE positive)
  int yawStar = -ch3Map;
  //int yawQ3 = ch3Map;
  //int yawQ4 = -ch3Map;

  // Map thruster components from -1000 to 1000
  portSetpoint = constrain(surgePort + swayPort + yawPort,-1000,1000);
  starSetpoint = constrain(surgeStar + swayStar + yawStar,-1000,1000);
  // q3Setpoint = constrain(surgeQ3 + swayQ3 + yawQ3,-1000,1000);
  // q4Setpoint = constrain(surgeQ4 + swayQ4 + yawQ4,-1000,1000);
  
}

// Calculate acceleration-limited outputs as a function of the setpoint and send to thrusters
void setpoint2Output() {

  // Calculate time delta since last loop
  timeNow = millis();
  float timeDelta = (timeNow - timeLast) * 0.001;

  // Calculate maximum thrust delta
  float thrustDelta = accLimit * 10 * timeDelta;  // maximum change in thrust component percentage in this loop [%]

  // Calculate actual thrust delta
  int portDelta = portSetpoint - portLast;
  int starDelta = starSetpoint - starLast;
  // int q3Delta = q3Setpoint - q3Last;
  // int q4Delta = q4Setpoint - q4Last;
  
  // Limit Q1 output thrust if change in setpoint is greater than allowable delta 
  if (abs(portDelta) > thrustDelta) {
    if (portDelta > 0) {
    portOut = portLast + thrustDelta;
    }
    else {
    portOut = portLast - thrustDelta;
    }
  }
  else {
    portOut = portSetpoint;
  }

  // Limit Q2 output thrust if change in setpoint is greater than allowable delta 
  if (abs(starDelta) > thrustDelta) {
    if (starDelta > 0) {
    starOut = starLast + thrustDelta;
    }
    else {
    starOut = starLast - thrustDelta;
    }
  }
  else {
    starOut = starSetpoint;
  }

  // Limit Q3 output thrust if change in setpoint is greater than allowable delta 
 // if (abs(q3Delta) > thrustDelta) {
 //   if (q3Delta > 0) {
 //   q3Out = q3Last + thrustDelta;
 //   }
 //   else {
 //   q3Out = q3Last - thrustDelta;
 //   }
 // }
 // else {
 //   q3Out = q3Setpoint;
 // }

  // Limit Q4 output thrust if change in setpoint is greater than allowable delta 
 // if (abs(q4Delta) > thrustDelta) {
 //   if (q4Delta > 0) {
 //   q4Out = q4Last + thrustDelta;
 //   }
 //   else {
 //   q4Out = q4Last - thrustDelta;
 //   }
 // }
 // else {
 //   q4Out = q4Setpoint;
 // }

  // Save timeNow and thruster output components for calculation in next loop
  timeLast = timeNow;
  portLast = portOut;
  starLast = starOut;
  //q3Last = q3Out;
  //q4Last = q4Out;
  
}

// Scale output to thrusters by a multiplier (typically to limit voltage output to thruster)
void scaleOutput() {

  portOut = portOut*voltMult;
  starOut = starOut*voltMult;
  //q3Out = q3Out*voltMult;
  //q4Out = q4Out*voltMult;

}

// Write output thruster values to motor controllers
void output2Thruster() {
  
  // reverse = 31, neutral = 47, forward = 63  
  int portBit = map(portOut,-1000,1000,111,255);
  Serial.println(portBit);
  int starBit = map(starOut,-1000,1000,111,255);
  // int q3Bit = map(q3Out,-1000,1000,31,63);
  // int q4Bit = map(q4Out,-1000,1000,31,63);

  // Write to motor controller pins
  analogWrite(portPin,portBit);
  analogWrite(starPin,starBit);
  // analogWrite(q3Pin,q3Bit);
  // analogWrite(q4Pin,q4Bit);
  
}


// Changes color of light
void changeLight(int lightColor) {
 
  // light color variable (0 = off, 1 = white, 2 = red, 3 = green, 4 = blue, 5 = yellow, 6 = light blue)

  if (lightColor == 0) {
    digitalWrite(redPin,HIGH);
    //digitalWrite(bluePin,HIGH);
    digitalWrite(greenPin,HIGH);
  }
  else if (lightColor == 1) {
    digitalWrite(redPin,LOW);
    //digitalWrite(bluePin,LOW);
    digitalWrite(greenPin,LOW);
  }
  else if (lightColor == 2) {
    digitalWrite(redPin,LOW);
    //digitalWrite(bluePin,HIGH);
    digitalWrite(greenPin,HIGH);
  }
  else if (lightColor == 3) {
    digitalWrite(redPin,HIGH);
    //digitalWrite(bluePin,HIGH);
    digitalWrite(greenPin,LOW);
  }
  else if (lightColor == 4) {
    digitalWrite(redPin,HIGH);
    //digitalWrite(bluePin,LOW);
    digitalWrite(greenPin,HIGH);
  }  
  else if (lightColor == 5) {
    digitalWrite(redPin,LOW);
    //digitalWrite(bluePin,HIGH);
    digitalWrite(greenPin,LOW);
  }
  else if (lightColor == 6) {
    digitalWrite(redPin,HIGH);
    //digitalWrite(bluePin,LOW);
    digitalWrite(greenPin,LOW);
  }  
}

// Read battery voltage
/* void battRead() {

  battVoltBit = analogRead(batteryPin);
  battVoltArd = battVoltBit/(205.0);
  battVoltTrue = battVoltArd*4.625;
  voltMult = 12/battVoltTrue;
  
  if (voltMult >= 1) {
    voltMult == 1;
  }
  
}

// Perform battery check if user desires it
void batteryCheck() {

  // Read channel 4
  ch4Pulse = pulseIn(ch4Pin,HIGH);

  if (ch4Pulse > ch4PulseMax - pulseTolerance && ch4Pulse < ch4PulseMax + pulseTolerance) {

    int battLevel = 0;
    // 0 = dead (V < 14)
    // 1 = low (14<V<14.66)
    // 2 = medium low (14.66<V<15.33)
    // 3 = medium full (15.33<V<15.99)
    // 4 = full (15.99<V<16.65)
    // 5 = invalid

    Serial.print("checking battery voltage...battery voltage is ");
    Serial.println(battVoltTrue);

    if (battVoltTrue < 14) {
      battLevel = 0;
      batteryLight(battLevel);
    }
    else if (battVoltTrue >= 14 && battVoltTrue < 14.66) {
      battLevel = 1;
      batteryLight(battLevel);
    }
    else if (battVoltTrue >= 14.66 && battVoltTrue < 15.33) {
      battLevel = 2;
      batteryLight(battLevel);
    }
    else if (battVoltTrue >= 15.33 && battVoltTrue < 15.99) {
      battLevel = 3;
      batteryLight(battLevel);
    }
    else if (battVoltTrue >= 15.99 && battVoltTrue < 16.65 + 0.25) {
      battLevel = 4;
      batteryLight(battLevel);
    }
    else {
      battLevel = 5;
      batteryLight(battLevel);
    }
  }
}

void batteryLight(int battLevel) {

  if (battLevel == 0) {
    changeLight(2);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(0);
    delay(200);
  }
  else if (battLevel == 1) {
    changeLight(2);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(2);
    delay(200);
    changeLight(4);
    delay(200);
  }
  else if (battLevel == 2) {
    changeLight(4);
    delay(2000);
  }
  else if (battLevel == 3) {
    changeLight(3);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(3);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(3);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(3);
    delay(200);
    changeLight(4);
    delay(200);
    changeLight(3);
    delay(200);
    changeLight(4);
    delay(200);
  }
  else if (battLevel == 4) {
    changeLight(3);
    delay(2000);
  }
  else {
    changeLight(1);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(1);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(1);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(1);
    delay(200);
    changeLight(0);
    delay(200);
    changeLight(1);
    delay(200);
    changeLight(0);
    delay(200);
  }

}
*/ 
