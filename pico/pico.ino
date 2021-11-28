#include "Adafruit_TinyUSB.h"

/***********************
* PIN CONFIGURATION
***********************/
#define ADC_LIGHT A0   //light sensor channel
#define PIN_CLICK D22  //mouse sensor pin
/***********************
* SENSOR CONFIGURATION
***********************/
const uint32_t samplerate = 50;         //sensor sample rate in us
const uint32_t streamrate = 10;         //stream rate in ms
const uint32_t click_timeout = 250000;  //click timeout in us
uint32_t light_sensitivity = 2000;      //default light sensitivity
uint32_t autofire_delay = 500;          //defaultdelay between clicks in ms when auto firing
uint32_t autofire_hold = 50;            //default time to hold a click in ms when auto firing

/***********************
** DO NOT EDIT BELOW  **
***********************/
#define REPORT_LENGTH 15
byte hid_report[REPORT_LENGTH];
const uint32_t MAGIC_PACKET = 0x66726565;

enum{
  OFFSET_MODE = 0,
  OFFSET_TIMESTAMP = 1,
  OFFSET_LIGHT_SENSOR = 5,
  OFFSET_IS_CLICK = 9,
  OFFSET_KEY = 10,
  OFFSET_VALUE = 11,
};

enum{
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

//globals
uint8_t mode = 0;                 //0: stream + e2e; 1: e2e; 2: input latency
bool use_hid = true;              //use hid or pin_click
uint32_t autofire_remaining = 0;  //how many times to auto fire mouse button
volatile int light_sensor = 0;    //current light value
volatile uint32_t event = 0;      //event time in us
bool do_report = false;           //indicates a report should happen
volatile bool is_click = false;   //current click state (modified by hid or pin_click)
volatile bool timeout = false;    //click timed out
uint8_t event_state = 0;          //0: wait for click; 1: wait for light / timeout; 2: wait for report, low light_sensor, low cs

volatile uint32_t click_received = 0;
uint32_t autofire_delay_rnd = autofire_delay * 1000;
uint32_t autofire_hold_rnd = autofire_hold * 1000;
uint32_t stream_timestamp = 0;

enum {
  RID_KEYBOARD = 1,
  RID_MOUSE,
  RID_GENERIC,  //generic io
};
//hid descriptor
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE)),
  TUD_HID_REPORT_DESC_GENERIC_INOUT(REPORT_LENGTH, HID_REPORT_ID(RID_GENERIC))
};
Adafruit_USBD_HID usb_hid;

void setup() {
  memset(hid_report, 0, REPORT_LENGTH);
  //TinyUSB_Device_Init(0);
  usb_hid.setPollInterval(1);
  //usb_hid.setStringDescriptor("Freeflex HID Composite");
  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_NONE);
  usb_hid.enableOutEndpoint(true);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setReportCallback(get_report_callback, set_report_callback);
  usb_hid.begin();
  Serial.begin(115200);

  // wait until device mounted
  while (!TinyUSBDevice.mounted()) delay(1);

  if (TinyUSBDevice.suspended()) {
    TinyUSBDevice.remoteWakeup();
  }
  pinMode(PIN_CLICK, INPUT_PULLUP);
  Serial.println("fReEFLEX pico");

  stream_timestamp = millis();
}

// currently not used
uint16_t get_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  process_hid_input(buffer);
}

void process_hid_input(uint8_t const* input) {
  memset(&hid_report[OFFSET_VALUE], 0, 4);
  bool set = input[OFFSET_MODE+1];
  bool respond = true;
  switch (input[OFFSET_KEY+1]) {
    case KEY_MAGIC_PACKET:
      hid_report[OFFSET_KEY] = KEY_MAGIC_PACKET;
      memcpy(&hid_report[OFFSET_VALUE], &MAGIC_PACKET, 4);
      break;
    case KEY_MODE:
      if (set) {
        memcpy(&mode, &input[OFFSET_VALUE+1], 1);
      }
      hid_report[OFFSET_KEY] = KEY_MODE;
      memcpy(&hid_report[OFFSET_VALUE], &mode, 1);
      break;
    case KEY_LIGHT_SENSITIVITY:
      if (set) {
        memcpy(&light_sensitivity, &input[OFFSET_VALUE+1], 4);
      }
      hid_report[OFFSET_KEY] = KEY_LIGHT_SENSITIVITY;
      memcpy(&hid_report[OFFSET_VALUE], &light_sensitivity, 4);
      break;
    case KEY_AUTOFIRE_DELAY:
      if (set) {
        memcpy(&autofire_delay, &input[OFFSET_VALUE+1], 4);
        autofire_delay_rnd = autofire_delay * 1000;
      }
      hid_report[OFFSET_KEY] = KEY_AUTOFIRE_DELAY;
      memcpy(&hid_report[OFFSET_VALUE], &autofire_delay, 4);
      break;
    case KEY_AUTOFIRE_HOLD:
      if (set) {
        memcpy(&autofire_hold, &input[OFFSET_VALUE+1], 4);
        autofire_hold_rnd = autofire_hold * 1000;
      }
      hid_report[OFFSET_KEY] = KEY_AUTOFIRE_HOLD;
      memcpy(&hid_report[OFFSET_VALUE], &autofire_hold, 4);
      break;
    case KEY_AUTOFIRE_REMAINING:
      if (set) {
        click_received = 0;
        memcpy(&autofire_remaining, &input[OFFSET_VALUE+1], 4);
      }
      hid_report[OFFSET_KEY] = KEY_AUTOFIRE_REMAINING;
      memcpy(&hid_report[OFFSET_VALUE], &autofire_remaining, 4);
      break;
    case KEY_USE_HID_MOUSE:
      if (set) {
        use_hid = input[OFFSET_VALUE+1] > 0;
      }
      hid_report[OFFSET_KEY] = KEY_USE_HID_MOUSE;
      memcpy(&hid_report[OFFSET_VALUE], &use_hid, 1);
      break;
    case KEY_CLICK_RECEIVED:
      click_received = micros();
      respond = false;
      break;
    default:
      break;
  }
  if (respond)
    sendHidReport();
}

void loop() {
  stream();
  autofire();
  report();
  reportEvent();
}

void stream() {
  if (millis() - stream_timestamp >= streamrate) {
    stream_timestamp = stream_timestamp + streamrate;
    do_report = mode == 0;
  }
}

uint32_t autofire_timestamp = micros();
bool autofire_firing = false;
void autofire() {
  if (autofire_remaining < 1) {
    return;
  }
  if (event_state == 0 && micros() - autofire_timestamp >= autofire_delay_rnd + autofire_hold_rnd) {
    hidWaitReady();
    autofire_timestamp = micros();
    if (use_hid) {
      hidLeftDown();
    } else {
      pinMode(PIN_CLICK, OUTPUT);
      digitalWrite(PIN_CLICK, LOW);
    }
    autofire_firing = true;
  } else if (autofire_firing && micros() - autofire_timestamp >= autofire_hold_rnd) {
    if (use_hid) {
      hidLeftUp();
    } else {
      pinMode(PIN_CLICK, INPUT_PULLUP);
    }
    autofire_firing = false;
    autofire_remaining = autofire_remaining - 1;

    autofire_delay_rnd = autofire_delay * 1000 + random(0, autofire_delay * 100);
    autofire_hold_rnd = autofire_hold * 1000 + random(0, autofire_hold * 200);

    hid_report[OFFSET_KEY] = KEY_AUTOFIRE_REMAINING;
    memcpy(&hid_report[OFFSET_VALUE], &autofire_remaining, 4);
    sendHidReport();
  }
}

void reportEvent() {
  if (event > 0) {
    uint32_t ev = event;
    hid_report[OFFSET_KEY] = KEY_EVENT;
    memcpy(&hid_report[OFFSET_VALUE], &ev, 4);
    event = 0;
    sendHidReport();
  } else if (timeout) {
    hid_report[OFFSET_KEY] = KEY_TIMEOUT;
    memcpy(&hid_report[OFFSET_VALUE], &click_timeout, 4);
    timeout = false;
    sendHidReport();
  }
}

void report() {
  if (!do_report) {
    return;
  }
  //send report
  memset(&hid_report[OFFSET_VALUE], 0, 4);
  memcpy(&hid_report[OFFSET_TIMESTAMP], &stream_timestamp, 4);
  hid_report[OFFSET_KEY] = 0;

  if (mode == 0) {
    int ls = light_sensor;
    memcpy(&hid_report[OFFSET_LIGHT_SENSOR], &ls, 4);
    hid_report[OFFSET_IS_CLICK] = is_click;
  } else {
    memset(&hid_report[OFFSET_LIGHT_SENSOR], 0, 4);
    hid_report[OFFSET_IS_CLICK] = 0;
  }
  do_report = false;
  sendHidReport();
}

void hidWaitReady() {
  if (TinyUSBDevice.suspended()) {
    TinyUSBDevice.remoteWakeup();
  }
  while (!usb_hid.ready()) {
    delayMicroseconds(100);
  }
}
void sendHidReport() {
    hidWaitReady();
    usb_hid.sendReport(RID_GENERIC, hid_report, REPORT_LENGTH);
}

void hidLeftDown() {
  hidWaitReady();
  is_click = true;
  usb_hid.mouseButtonPress(RID_MOUSE, MOUSE_BUTTON_LEFT);
}

void hidLeftUp() {
  hidWaitReady();
  usb_hid.mouseButtonRelease(RID_MOUSE);
  is_click = false;
}

/****************
** SECOND CORE **
****************/
uint32_t sample_timestamp;
void setup1() {
  delay(2000);
  sample_timestamp = micros();
}

void loop1() {
  sample();
}

bool sampled_click = false;
void sampleClick() {
  if (sampled_click || use_hid) {
    return;
  }
  sampled_click = true;
  if (digitalRead(PIN_CLICK) == LOW) {
    is_click = true;
  } else {
    is_click = false;
  }
}

bool sampled_light = false;
void sampleLight() {
  if (sampled_light) {
    return;
  }
  sampled_light = true;
  if (mode == 2) {
    if (click_received > 0) {
      light_sensor = light_sensitivity;
      click_received = 0;
    } else {
      light_sensor = 0;
    }
  } else {
    light_sensor = analogRead(ADC_LIGHT);
  }
}

bool isBright() {
  return (light_sensor >= light_sensitivity);
}

uint32_t last_click = 0;
bool isTimeout() {
  return (micros() - last_click >= min(click_timeout, autofire_delay_rnd));
}

uint32_t sample_counter = 0;
void sample() {
  if (micros() - sample_timestamp >= samplerate) {
    sample_timestamp = sample_timestamp + samplerate;
    sampled_click = false;
    sampled_light = false;
    sample_counter++;
    debugEvent();
    switch (event_state) {
      case 0:
        sampleClick();
        if (is_click) {
          event_state = 1;
          last_click = sample_timestamp;
        }
        break;
      case 1:
        sampleLight();
        if (isBright()) {
          event = sample_timestamp - last_click;
          event_state = 2;
        } else if (isTimeout()) {
          event_state = 2;
          timeout = true;
        }
        break;
      case 2:
        sampleClick();
        sampleLight();
        if (event == 0 && !timeout && !is_click && !isBright()) {
          event_state = 0;
        } else {
          //debugEvent();
        }
        break;
      default:
        break;
    }
    if (mode == 0) {
      sampleClick();
      sampleLight();
    }
  }
}

uint32_t debugTimer = millis();
void debugEvent() {
  if (millis() - debugTimer >= 1000) {
    debugTimer = millis();
    Serial.println(sample_counter);
    Serial.println(light_sensor);
    sample_counter = 0;
    //Serial.println("micros: " + String(event) + " | timeout: " + timeout + " | is_click: " + is_click + " | isBright: " + isBright() + " | do_report: " + do_report);
  }
}
