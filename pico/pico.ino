#include "Adafruit_TinyUSB.h"
//#include "pico/multicore.h"


#define VERSION 2

/***********************
* PIN CONFIGURATION ****
***********************/
#define ADC_LIGHT A0              //light sensor channel
#define PIN_CLICK D22             //mouse sensor pin
/***********************
* SAMPLING CONFIG ******
***********************/
#define SAMPLERATE 20             //us
#define EVENT_TIMEOUT 250000      //us
/***********************
* STREAMING CONFIG *****
***********************/
#define STREAMRATE 10             //ms


/***********************
************************
** DO NOT EDIT BELOW  **
************************
***********************/
#define MAGIC_PACKET 0x66726565   //magic packet to identify as fReEFLEX controller
enum{
  KEY_STREAM = 0,
  KEY_MAGIC_PACKET = 1,
  KEY_MODE = 2,
  KEY_EVENT = 3,
  KEY_TIMEOUT = 4,
  KEY_LIGHT_SENSITIVITY = 5,
  KEY_USE_HID_MOUSE = 6,
  KEY_AUTOFIRE_DELAY = 7,
  KEY_AUTOFIRE_HOLD = 8,
  KEY_AUTOFIRE_REMAINING = 9,
  KEY_CLICK_RECEIVED = 10,
  KEY_MESSAGE = 20,
  KEY_ERROR = 21,
};

/***********************
* GLOBAL VARIABLES *****
***********************/

struct Globals {
  private:
    critical_section_t cs1;
    //used on both cores
    uint8_t mode = 0;                       //0: stream + e2e; 1: e2e; 2: input latency; 3: response time (not ready); 4: light frequency
    uint32_t light_sensitivity = 4095;      //default light sensitivity
    uint32_t event = 0;                     //measured e2e latency or response time
    bool use_hid = true;                    //use emulated hid mouse or PIN_CLICK    
    bool is_mouse_down = false;             //mouse down detected (from auto fire or PIN_CLICK)    
    bool is_timeout = false;                //timeout while measuring event

    uint32_t click_received = 0;
    uint32_t sample_count = 0;
    int adc_light_val = 0;

    //used on one core
    uint32_t autofire_delay = 500000;       //defaultdelay between clicks in us when auto firing
    uint32_t autofire_hold = 50000;         //default time to hold a click in us when auto firing

  public:
    void init() {
      critical_section_init(&cs1);
    }
    //used on one core
    uint32_t autofire_delay_rnd = autofire_delay + random(0, 1000);
    uint32_t autofire_hold_rnd = autofire_hold + random(0, 1000);
    uint32_t autofire_remaining = 0;           

    void setMode(uint8_t val) {
      critical_section_enter_blocking(&cs1); 
      mode = val;
      critical_section_exit(&cs1);     
    }

    uint8_t getMode() {
      critical_section_enter_blocking(&cs1); 
      uint8_t val = mode;
      critical_section_exit(&cs1);     
      return val;
    }

    void setLight_sensitivity(uint32_t val) {
      critical_section_enter_blocking(&cs1); 
      light_sensitivity = val;
      critical_section_exit(&cs1);     
    }

    uint32_t getLight_sensitivity() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = light_sensitivity;
      critical_section_exit(&cs1);     
      return val;
    }

    void setEvent(uint32_t val) {
      critical_section_enter_blocking(&cs1); 
      event = val;
      critical_section_exit(&cs1);     
    }

    uint32_t getEvent() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = event;
      critical_section_exit(&cs1);     
      return val;
    }

    void setAutofire_delay(uint32_t val) {
      critical_section_enter_blocking(&cs1); 
      autofire_delay = val;
      critical_section_exit(&cs1);  
      autofire_delay_rnd = autofire_delay + random(0, 1000); 
    }

    uint32_t getAutofire_delay() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = autofire_delay;
      critical_section_exit(&cs1);     
      return val;
    }

    void setAutofire_hold(uint32_t val) {
      critical_section_enter_blocking(&cs1); 
      autofire_hold = val;
      critical_section_exit(&cs1);  
      autofire_hold_rnd = autofire_hold + random(0, 1000); 
    }

    uint32_t getAutofire_hold() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = autofire_hold;
      critical_section_exit(&cs1);     
      return val;
    }

    void setUse_hid(bool val) {
      critical_section_enter_blocking(&cs1); 
      use_hid = val;
      critical_section_exit(&cs1);     
    }

    bool getUse_hid() {
      critical_section_enter_blocking(&cs1); 
      bool val = use_hid;
      critical_section_exit(&cs1);     
      return val;
    }

    void setIs_mouse_down(bool val) {
      critical_section_enter_blocking(&cs1); 
      is_mouse_down = val;
      critical_section_exit(&cs1);     
    }

    bool getIs_mouse_down() {
      critical_section_enter_blocking(&cs1); 
      bool val = is_mouse_down;
      critical_section_exit(&cs1);     
      return val;
    }

    void setIs_timeout(bool val) {
      critical_section_enter_blocking(&cs1); 
      is_timeout = val;
      critical_section_exit(&cs1);     
    }

    bool getIs_timeout() {
      critical_section_enter_blocking(&cs1); 
      bool val = is_timeout;
      critical_section_exit(&cs1);     
      return val;
    }

    void setAdc_light_val(int val) {
      critical_section_enter_blocking(&cs1); 
      adc_light_val = val;
      critical_section_exit(&cs1);     
    }

    void setClick_received(uint32_t val) {
      critical_section_enter_blocking(&cs1); 
      click_received = val;
      critical_section_exit(&cs1);     
    }

    uint32_t getClick_received() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = click_received;
      critical_section_exit(&cs1);  
      return val;
    }

    void incSample_count() {
      critical_section_enter_blocking(&cs1); 
      sample_count++;
      critical_section_exit(&cs1);     
    }

    uint32_t resetSample_count() {
      critical_section_enter_blocking(&cs1); 
      uint32_t val = sample_count;
      sample_count = 0;
      critical_section_exit(&cs1);     
      return val;
    }

    int getAdc_light_val() {
      critical_section_enter_blocking(&cs1); 
      int val = adc_light_val;
      critical_section_exit(&cs1);     
      return val;
    }
};
Globals globals;

/***********************
* HID ******************
***********************/
#define REPORT_LENGTH 14
enum {
  RID_KEYBOARD = 1,
  RID_MOUSE,
  RID_GENERIC,
};
const uint8_t desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE)),
  TUD_HID_REPORT_DESC_GENERIC_INOUT(REPORT_LENGTH, HID_REPORT_ID(RID_GENERIC))
};
Adafruit_USBD_HID usb_hid;

struct HidReport {
  private:
  public:
    uint32_t timestamp;
    uint8_t key = 0;
    bool set = false;
    uint32_t value = 0;
    uint32_t value_2 = 0;

    void fromByteArray(uint8_t const* buffer) {
      memcpy(&timestamp, &buffer[0], 4);
      memcpy(&key, &buffer[4], 4);
      set = buffer[5] > 0;
      memcpy(&value, &buffer[6], 4);
      memcpy(&value_2, &buffer[10], 4);
    };

    void toByteArray(uint8_t* buffer) {
      memcpy(&buffer[0], &timestamp, 4);
      memcpy(&buffer[4], &key, 1);
      buffer[5] = set ? 1 : 0;
      memcpy(&buffer[6], &value, 4);
      memcpy(&buffer[10], &value_2, 4);
    };
};

void hid_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  static HidReport report;
  //static HidReport response_report;  
  
  if (bufsize == REPORT_LENGTH + 1 && buffer[0] == 0x03) {
    buffer = &buffer[1];
    bufsize = REPORT_LENGTH;
  } 
  if (bufsize != REPORT_LENGTH) return;

  report.fromByteArray(buffer);

  //Serial.println("received hid report: " + String(report.timestamp) + "|" + String(report.key) + "|" + String(report.set) + "|" + String(report.value) + "|" + String(report.value_2));
  
  report.timestamp = millis();
  bool response = true;
  switch (report.key) {
    case KEY_MAGIC_PACKET:
      report.value = MAGIC_PACKET;
      report.value_2 = VERSION;
      break;
    case KEY_MODE:
      if (report.set) {
        globals.setMode(min(max(report.value,0),4));
      }
      report.value = globals.getMode();
      break;
    case KEY_LIGHT_SENSITIVITY:
      if (report.set) {
        globals.setLight_sensitivity(min(max(report.value,1),4095));
      }
      report.value = globals.getLight_sensitivity();
      break;
    case KEY_USE_HID_MOUSE:
      if (report.set) {
        globals.setUse_hid(report.value > 0);
      }
      report.value = globals.getUse_hid();
      break;
    case KEY_AUTOFIRE_DELAY:
      if (report.set) {
        globals.setAutofire_delay(min(max(report.value,50),5000) * 1000);
      }
      report.value = globals.getAutofire_delay() / 1000;
      break;
    case KEY_AUTOFIRE_HOLD:
      if (report.set) {
        globals.setAutofire_hold(min(max(report.value,20),2000) * 1000);
      }
      report.value = globals.getAutofire_hold() / 1000;
      break;
    case KEY_AUTOFIRE_REMAINING:
      if (report.set) {
        globals.autofire_remaining = min(max(report.value,0),10000);
      }
      report.value = globals.autofire_remaining;
      break;
    case KEY_CLICK_RECEIVED:
      globals.setClick_received(micros());
      response = false;
      break;
    default:
      break;
  }
  if (response) {
    hidSendReport(report);
  }
}

void hidWaitReady() {
  if (TinyUSBDevice.suspended()) {
    TinyUSBDevice.remoteWakeup();
  }
  while (!usb_hid.ready()) {
    delayMicroseconds(1);
  }
}

void hidSendReport(HidReport &report) {
    hidWaitReady();
    static uint8_t buffer[REPORT_LENGTH];
    report.toByteArray(buffer);
    usb_hid.sendReport(RID_GENERIC, buffer, REPORT_LENGTH);
}

void mouseDown() {
  if (globals.getUse_hid()) {
    hidWaitReady();
    usb_hid.mouseButtonPress(RID_MOUSE, MOUSE_BUTTON_LEFT);
    globals.setIs_mouse_down(true);
  } else {
    pinMode(PIN_CLICK, OUTPUT);
    digitalWrite(PIN_CLICK, LOW);
  }
}

void mouseUp() {
  if (globals.getUse_hid()) {
    hidWaitReady();
    usb_hid.mouseButtonRelease(RID_MOUSE);
    globals.setIs_mouse_down(false);
  } else {
    pinMode(PIN_CLICK, INPUT_PULLUP);
  }
}

/***********************
* SETUP & LOOP *********
***********************/

void setup() {
  //init globals 
  globals.init();

  //init hid
  USBDevice.setManufacturerDescriptor("github.com/fReEFLEX/            ");
  USBDevice.setProductDescriptor     ("fReEFLEX Controller             ");
  usb_hid.setStringDescriptor("fReEFLEX HID");
  usb_hid.setPollInterval(1);
  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_NONE);
  usb_hid.enableOutEndpoint(true);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setReportCallback(NULL, hid_callback);
  usb_hid.begin();
  while (!TinyUSBDevice.mounted()) delay(1);
  if (TinyUSBDevice.suspended()) {
    TinyUSBDevice.remoteWakeup();
  }

  //initialize pins
  pinMode(PIN_CLICK, INPUT_PULLUP);
  analogRead(ADC_LIGHT); 

  //initialize serial port
  Serial.begin(250000);
}

void loop() {
  autofire(); 
  stream();
  reportEvent();
  serialPing();
}

void autofire() {
  static uint32_t autofire_timestamp = micros();
  static bool mouse_down = false;
  static HidReport autofire_report;
  if (globals.autofire_remaining < 1) {
    return;
  }
  if (micros() - autofire_timestamp >= globals.autofire_delay_rnd + globals.autofire_hold_rnd) {
    autofire_timestamp = micros();
    mouseDown();
    mouse_down = true;
  } else if (mouse_down && micros() - autofire_timestamp >= globals.autofire_hold_rnd) {
    mouseUp();
    mouse_down = false;

    //update random delays
    globals.autofire_delay_rnd = globals.getAutofire_delay() + random(0, 1000);
    globals.autofire_hold_rnd = globals.getAutofire_hold() + random(0, 1000);

    //send autofire report
    globals.autofire_remaining--;
    autofire_report.key = KEY_AUTOFIRE_REMAINING;
    autofire_report.value = globals.autofire_remaining;
    hidSendReport(autofire_report);
  }
}

void stream() {
  static uint32_t stream_timestamp;
  static HidReport stream_report;
  if (globals.getMode() != 0) {
    return;
  }
  uint32_t now = millis();
  if (now - stream_timestamp >= STREAMRATE) {
    stream_timestamp = now - ((now - stream_timestamp) % STREAMRATE);
    //send report
    stream_report.timestamp = now;
    stream_report.key = KEY_STREAM;
    stream_report.value = globals.getAdc_light_val();
    stream_report.value_2 = globals.getIs_mouse_down();
    hidSendReport(stream_report);
  }
}

void reportEvent() {
  static HidReport event_report;
  static uint32_t event;
  static bool is_timeout;
  event = globals.getEvent();
  if (event > 0) {
    event_report.key = KEY_EVENT;
    event_report.timestamp = millis();
    event_report.value = event;
    hidSendReport(event_report);
    globals.setEvent(0);
    return;
  } 
  is_timeout = globals.getIs_timeout();
  if (is_timeout) {
    event_report.key = KEY_TIMEOUT;
    event_report.timestamp = millis();
    event_report.value = min(globals.getAutofire_delay() + 1000, EVENT_TIMEOUT);
    hidSendReport(event_report);
    globals.setIs_timeout(false);
  }
}

void serialPing() {
  static uint32_t ping_timestamp = 0;
  uint32_t now = millis();
  if (now - ping_timestamp >= 5000) {
    ping_timestamp = now - ((now - ping_timestamp) % 5000);
    Serial.println("timestamp: " + String(now/1000) + "s | samplerate: " + String((float)globals.resetSample_count()/5000) + "kHz");
  }
  if (globals.getMode() == 2 && Serial.available() > 0) {
    //detect system latency response 
    globals.setClick_received(micros());
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}



/****************
** SECOND CORE **
****************/

void setup1() {
  //wait for core 1 to initialize
  delay(2000);
}

void loop1() {
  sample();
}

void sample() {
  static uint32_t sample_timestamp;
  static uint32_t adc_light_value;
  static uint32_t light_sensitivity;
  static uint32_t click_received;
  static bool     is_mouse_down;
  static bool     use_hid;

  uint32_t now = micros();
  if (now - sample_timestamp >= SAMPLERATE) {
    sample_timestamp = now - ((now - sample_timestamp) % SAMPLERATE);
    
    //read light sensor
    adc_light_value = analogRead(ADC_LIGHT);
    globals.setAdc_light_val(adc_light_value);
    //read mouse sensor
    use_hid = globals.getUse_hid();
    if (use_hid) {
      is_mouse_down = globals.getIs_mouse_down();
    } else {
      is_mouse_down = digitalRead(PIN_CLICK) == LOW;
      globals.setIs_mouse_down(is_mouse_down);
    }

    //mode 0-1
    switch (globals.getMode()) {
      case 0:
      case 1:
        light_sensitivity = globals.getLight_sensitivity();
        detect_latency(now, is_mouse_down, adc_light_value >= light_sensitivity);
        break;
      case 2:
        click_received = globals.getClick_received(); 
        click_received = now - click_received < min(globals.getAutofire_delay() + 1000, EVENT_TIMEOUT) ? click_received : 0;
        detect_latency(click_received > 0 ? click_received : now, is_mouse_down, click_received > 0);
        break;
      case 3:
        detect_response_time(adc_light_value);
        break;
      case 4: 
        detect_light_frequency(now, adc_light_value);
      default:
        break;
    }

    
    //increase sample counter
    globals.incSample_count();
  }
}

void detect_latency(const uint32_t &now, const bool &is_mouse_down, const bool &is_bright) {
  static uint8_t state = 0;
  static uint32_t last_click = 0;
  switch (state) {
    case 0: //wait for mouse down
      if (is_mouse_down) {
        state = 1;
        last_click = now;
      }
      break;
    case 1: //wait for light source 
      if (is_bright) {
        globals.setEvent(now - last_click);
        state = 2;
      } else if (now - last_click >= min(globals.getAutofire_delay() + 1000, EVENT_TIMEOUT)) {
        state = 2;
        globals.setIs_timeout(true);
      }
      break;
    case 2:  //wait for core 1 to report event and both sensor values to reset to default state
      if (globals.getEvent() == 0 && !globals.getIs_timeout() && !is_mouse_down && !is_bright) {
        state = 0;
      }
      break;
    default:
      break;
  }
}

void detect_response_time(const uint32_t &adc_light_value) {
  
}

void detect_light_frequency(const uint32_t &now, const uint32_t &adc_light_value) {
  static uint8_t state;
  static uint32_t low = 9999;
  static uint32_t high = 0;
  static uint32_t count_high = 0;
  static uint32_t start = 0;
  static bool is_high = false;
  switch (state) {
    case 0:
      //calibrate
      if (adc_light_value < low) low = adc_light_value;
      if (adc_light_value > high) high = adc_light_value;
      if (high / low >= 2 && now - start >= 1000000) {
        start = now;
        high -= (high-low)/4;
        low += (high-low)/4;
        is_high = (float)adc_light_value >= high;
        state = 1;
      }
      break;
    case 1:
      if ((float)adc_light_value >= high) {
        if (!is_high) {
          count_high++;
          is_high = true;
        }
      } else if (is_high && (float)adc_light_value <= low) {
        is_high = false;
      }
      if (now - start >= 1000000) {
        globals.setEvent(floor((float)count_high * 1000000 / (now - start)));
        low = 9999;
        high = 0;
        count_high = 0;
        start = now;
        state = 0;
      }
      break;
    default:
      break;
  }
}


