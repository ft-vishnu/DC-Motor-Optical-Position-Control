// ============================================================
// OPQC - Optical Position Control  (state-machine version)
//
// ESP32-S3 + TB6612FNG + single-channel optical encoder
// Web UI, PD position control, marker-based homing, virtual dial
//
// States:
//   READY -> MOVING -> BRAKING -> SETTLING -> (verify) -> READY
//   READY -> HOME_CLEAR -> HOME_SEARCH -> BRAKING -> SETTLING -> READY (homed)
//   any   -> FAULT (stall / timeout)    STOP -> BRAKING -> SETTLING -> STOPPED
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include "driver/gpio.h"

// ============================================================
// Wi-Fi
// ============================================================
const char* AP_SSID = "DC-Motor-Optical-Position-Control";
const char* AP_PASS = "12345678";

WebServer server(80);

// ============================================================
// Pins
// ============================================================
const int ENCODER_PIN = 4;
const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

// ============================================================
// Encoder
// ============================================================
const int   COUNTS_PER_REV    = 20;
const float DEGREES_PER_COUNT = 360.0f / COUNTS_PER_REV;   // 18 deg

// Ignore edges closer than this (electrical noise / bounce)
const uint32_t EDGE_DEBOUNCE_US = 100;

volatile long     encoderCount   = 0;
volatile int      motorDirection = 1;      // single channel: direction = commanded direction
volatile bool     lastLevel      = false;
volatile uint32_t lastEdgeUs     = 0;
volatile uint32_t riseTimeUs     = 0;

// Ring buffer of completed HIGH pulse widths (filled by ISR, read by homing)
const uint8_t WIDTH_BUF = 16;
volatile uint32_t widthBuf[WIDTH_BUF];
volatile uint8_t  widthHead = 0;
volatile uint8_t  widthTail = 0;

// ============================================================
// PD controller  (equation unchanged from your working code)
// ============================================================
const int MAX_PWM = 255;
const int MIN_PWM = 0;        // set ~40-60 if the motor will not start on a 1-count error

float Kp = 20;
float Kd = 10.0;

float         previousError = 0;
unsigned long previousTime  = 0;          // micros
const unsigned long CONTROL_INTERVAL_US = 1000;

// ============================================================
// Motion / safety limits
// ============================================================
const unsigned long BRAKE_MS            = 120;
const unsigned long SETTLE_MS           = 300;    // let the rotor stop before judging position
const unsigned long STALL_MS            = 1500;   // no encoder activity while driving
const unsigned long MOVE_BASE_TIMEOUT   = 3000;
const unsigned long MOVE_PER_COUNT_MS   = 1000;
const int           MAX_RETRIES         = 3;      // correction passes after settling

// ============================================================
// Homing
// ============================================================
const int           HOME_PWM            = 60;
const int           HOME_KICK_PWM       = 150;    // short kick to beat stiction
const unsigned long HOME_KICK_MS        = 40;
const unsigned long HOME_TIMEOUT_MS     = 20000;

// Marker = a HIGH pulse much wider than the normal ones.
// It is judged against the running average, so it does not depend on motor speed.
const float         HOME_MARKER_RATIO   = 2.0f;
const uint32_t      HOME_MIN_MARKER_US  = 2000;   // absolute floor
const uint8_t       HOME_BASELINE_PULSES = 3;     // normal pulses needed before judging

float   homeAvgHigh       = 0;
uint8_t homeBaselineCount = 0;
bool    homeKickDone      = false;

// ============================================================
// State machine
// ============================================================
enum State   : uint8_t { S_READY, S_MOVING, S_HOME_CLEAR, S_HOME_SEARCH,
                         S_BRAKING, S_SETTLING, S_STOPPED, S_FAULT };
enum Pending : uint8_t { P_NONE, P_MOVE, P_HOME, P_STOP };

State   state   = S_READY;
Pending pending = P_NONE;

long          targetPosition   = 0;
long          prevErrorLong    = 0;
int           moveRetries      = 0;
bool          homed            = false;
String        message          = "Ready";

unsigned long stateStartMs     = 0;
unsigned long opStartMs        = 0;
unsigned long moveTimeoutMs    = 0;
unsigned long lastActivityMs   = 0;
long          lastCountSeen    = 0;
bool          lastLevelSeen    = false;

bool isBusy()
{
    return state == S_MOVING || state == S_HOME_CLEAR || state == S_HOME_SEARCH ||
           state == S_BRAKING || state == S_SETTLING;
}

const char* statusText()
{
    switch (state)
    {
        case S_READY:       return (homed && getEncoderCountFast() == 0) ? "HOME" : "READY";
        case S_MOVING:      return "MOVING";
        case S_HOME_CLEAR:
        case S_HOME_SEARCH: return "HOMING";
        case S_BRAKING:
        case S_SETTLING:    return pending == P_HOME ? "HOMING" :
                                   pending == P_MOVE ? "MOVING" : "STOPPING";
        case S_STOPPED:     return "STOPPED";
        case S_FAULT:       return "FAULT";
    }
    return "UNKNOWN";
}

// ============================================================
// Encoder interrupt  (CHANGE: counts on rising, times HIGH pulses)
// ============================================================
void IRAM_ATTR encoderISR()
{
    uint32_t now = micros();

    if ((uint32_t)(now - lastEdgeUs) < EDGE_DEBOUNCE_US) return;

    bool level = gpio_get_level((gpio_num_t)ENCODER_PIN);
    if (level == lastLevel) return;            // not a real transition

    lastLevel  = level;
    lastEdgeUs = now;

    if (level)
    {
        encoderCount += motorDirection;
        riseTimeUs = now;
    }
    else
    {
        widthBuf[widthHead] = now - riseTimeUs;
        widthHead = (widthHead + 1) % WIDTH_BUF;
    }
}

long getEncoderCount()
{
    noInterrupts();
    long c = encoderCount;
    interrupts();
    return c;
}

long getEncoderCountFast() { return encoderCount; }   // 32-bit read is atomic

void resetEncoderCount()
{
    noInterrupts();
    encoderCount = 0;
    interrupts();
}

bool popWidth(uint32_t &w)
{
    if (widthTail == widthHead) return false;
    w = widthBuf[widthTail];
    widthTail = (widthTail + 1) % WIDTH_BUF;
    return true;
}

void flushWidths()
{
    noInterrupts();
    widthTail = widthHead;
    interrupts();
}

// ============================================================
// Motor
// ============================================================
void setMotor(int direction, int pwm)
{
    motorDirection = direction;       // set BEFORE the motor moves

    digitalWrite(AIN1, direction > 0 ? HIGH : LOW);
    digitalWrite(AIN2, direction > 0 ? LOW  : HIGH);

    analogWrite(PWMA, constrain(pwm, 0, MAX_PWM));
}

void stopMotor()
{
    analogWrite(PWMA, 0);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
}

void applyBrake()                      // non-blocking short brake
{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, 255);
}

void beginBrake(Pending p)
{
    pending      = p;
    applyBrake();
    state        = S_BRAKING;
    stateStartMs = millis();
}

void fault(const char* why)
{
    stopMotor();
    pending = P_NONE;
    state   = S_FAULT;
    message = why;
    Serial.printf("FAULT: %s\n", why);
}

// ============================================================
// Position move
// ============================================================
void beginPD()
{
    previousError  = 0;                // same start condition as your working code
    previousTime   = micros();
    prevErrorLong  = targetPosition - getEncoderCount();
    lastCountSeen  = getEncoderCount();
    lastActivityMs = millis();
    state          = S_MOVING;
    pending        = P_NONE;
    message        = "Moving";
}

// returns nullptr on success, otherwise the reason it was rejected
const char* startMove(float angle)
{
    if (isBusy()) return "Busy";

    long counts = lround(angle / DEGREES_PER_COUNT);

    if (counts == 0 || fabsf(angle - counts * DEGREES_PER_COUNT) > 0.01f)
        return "Angle must be a non-zero multiple of 18";

    long start     = getEncoderCount();
    targetPosition = start + counts;
    moveRetries    = 0;
    opStartMs      = millis();
    moveTimeoutMs  = MOVE_BASE_TIMEOUT + MOVE_PER_COUNT_MS * (unsigned long)labs(counts);

    beginPD();

    Serial.printf("\nMOVE %.1f deg | start=%ld need=%ld target=%ld\n",
                  angle, start, counts, targetPosition);
    return nullptr;
}

void updateMoving()
{
    uint32_t nowUs = micros();
    if (nowUs - previousTime < CONTROL_INTERVAL_US) return;

    long pos   = getEncoderCount();
    long error = targetPosition - pos;
    unsigned long ms = millis();

    // ---- watchdogs -------------------------------------------------
    if (pos != lastCountSeen) { lastCountSeen = pos; lastActivityMs = ms; }

    if (ms - opStartMs > moveTimeoutMs) { fault("MOVE TIMEOUT"); return; }

    if (ms - lastActivityMs > STALL_MS)
    {
        if (labs(error) <= 1) beginBrake(P_MOVE);   // stuck on the last count: finish & verify
        else                  fault("STALL");
        return;
    }

    // ---- arrived (exactly, or crossed the target) ------------------
    if (error == 0 || ((error > 0) != (prevErrorLong > 0)))
    {
        beginBrake(P_MOVE);
        return;
    }
    prevErrorLong = error;

    // ---- PD (unchanged) ---------------------------------------------
    float dt = (nowUs - previousTime) / 1000000.0f;
    previousTime = nowUs;

    float derivative = ((float)error - previousError) / dt;
    float output     = Kp * error + Kd * derivative;
    previousError    = error;

    int direction = output > 0 ? 1 : -1;
    int pwm = constrain(abs((int)output), 0, MAX_PWM);
    if (pwm > 0 && pwm < MIN_PWM) pwm = MIN_PWM;

    setMotor(direction, pwm);
}

// ============================================================
// Homing
// ============================================================
void enterHomeSearch()
{
    flushWidths();                     // discard any partial pulse
    homeBaselineCount = 0;
    homeAvgHigh       = 0;
    state             = S_HOME_SEARCH;
    Serial.println("HOMING: SEARCHING");
}

bool startHoming()
{
    if (isBusy()) return false;

    stopMotor();

    homed          = false;
    homeKickDone   = false;
    opStartMs      = millis();
    lastActivityMs = opStartMs;
    lastCountSeen  = getEncoderCount();
    lastLevelSeen  = digitalRead(ENCODER_PIN);
    pending        = P_NONE;
    message        = "Homing";

    if (lastLevelSeen == HIGH)
    {
        state = S_HOME_CLEAR;          // we may be inside the marker: leave it first
        Serial.println("HOMING: CLEARING REGION");
    }
    else
    {
        enterHomeSearch();
    }

    setMotor(1, HOME_KICK_PWM);
    return true;
}

void updateHoming()
{
    unsigned long ms = millis();

    if (!homeKickDone && ms - opStartMs >= HOME_KICK_MS)
    {
        setMotor(1, HOME_PWM);
        homeKickDone = true;
    }

    if (ms - opStartMs > HOME_TIMEOUT_MS) { fault("HOME TIMEOUT"); return; }

    bool lvl = digitalRead(ENCODER_PIN);
    long c   = getEncoderCount();
    if (c != lastCountSeen || lvl != lastLevelSeen)
    {
        lastCountSeen  = c;
        lastLevelSeen  = lvl;
        lastActivityMs = ms;
    }
    if (ms - lastActivityMs > STALL_MS) { fault("HOME STALL (raise HOME_PWM?)"); return; }

    // ---- 1. get out of any HIGH region we started in ----------------
    if (state == S_HOME_CLEAR)
    {
        if (lvl == LOW) enterHomeSearch();
        return;
    }

    // ---- 2. look at every completed HIGH pulse ------------------------
    uint32_t w;
    while (popWidth(w))
    {
        bool isMarker = homeBaselineCount >= HOME_BASELINE_PULSES &&
                        w >= HOME_MIN_MARKER_US &&
                        (float)w > homeAvgHigh * HOME_MARKER_RATIO;

        if (isMarker)
        {
            Serial.printf("HOME MARKER: %lu us (normal avg %.0f us)\n",
                          (unsigned long)w, homeAvgHigh);
            beginBrake(P_HOME);        // zero is set after the rotor has settled
            return;
        }

        homeAvgHigh = (homeBaselineCount == 0) ? (float)w
                                               : 0.7f * homeAvgHigh + 0.3f * (float)w;
        if (homeBaselineCount < 255) homeBaselineCount++;
    }
}

// ============================================================
// After brake + settle
// ============================================================
void finishAfterSettle()
{
    switch (pending)
    {
        case P_STOP:
            state   = S_STOPPED;
            pending = P_NONE;
            message = "Stopped";
            Serial.println("STOPPED");
            break;

        case P_HOME:
            resetEncoderCount();       // rest position after the marker = zero
            targetPosition = 0;
            homed   = true;
            state   = S_READY;
            pending = P_NONE;
            message = "Homed";
            Serial.println("HOME OK: encoder = 0");
            break;

        case P_MOVE:
        {
            long err = targetPosition - getEncoderCount();

            if (err == 0)
            {
                state   = S_READY;
                pending = P_NONE;
                message = "Target reached";
                Serial.printf("Target reached, count=%ld\n", getEncoderCount());
            }
            else if (moveRetries < MAX_RETRIES)
            {
                moveRetries++;
                moveTimeoutMs += 3000;
                Serial.printf("Correction %d, error=%ld\n", moveRetries, err);
                beginPD();
            }
            else
            {
                state   = S_READY;
                pending = P_NONE;
                message = "Off target by " + String(err) + " count(s)";
                Serial.println(message);
            }
            break;
        }

        default:
            state = S_READY;
            break;
    }
}

void runStateMachine()
{
    switch (state)
    {
        case S_MOVING:      updateMoving();  break;
        case S_HOME_CLEAR:
        case S_HOME_SEARCH: updateHoming();  break;

        case S_BRAKING:
            if (millis() - stateStartMs >= BRAKE_MS)
            {
                stopMotor();
                state        = S_SETTLING;
                stateStartMs = millis();
            }
            break;

        case S_SETTLING:
            if (millis() - stateStartMs >= SETTLE_MS) finishAfterSettle();
            break;

        default: break;
    }
}

// ============================================================
// Web page
// ============================================================
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DC-Motor-Optical-Position-Control</title>
<style>
*{box-sizing:border-box}
body{margin:0;font-family:Arial,Helvetica,sans-serif;background:#0b1120;color:#e5e7eb}
.container{max-width:900px;margin:auto;padding:24px}
.header h1{margin:0;font-size:30px;font-weight:600;letter-spacing:1px}
.header p{margin:6px 0 24px;color:#94a3b8;font-size:14px}
.dashboard{display:grid;grid-template-columns:1fr 1fr;gap:20px}
.card{background:#111827;border:1px solid #1e293b;border-radius:16px;padding:22px}
.position-card{text-align:center}
.label{color:#94a3b8;font-size:13px;text-transform:uppercase;letter-spacing:1px}
.angle-value{margin-top:8px;font-size:42px;font-weight:600}
.count-value{margin-top:6px;color:#94a3b8;font-size:16px}

/* ---------- dial ---------- */
.dial-container{display:flex;justify-content:center;margin-top:20px}
.dial{position:relative;width:260px;height:260px;border-radius:50%;
 background:radial-gradient(circle,#1e293b 0%,#111827 62%,#0f172a 63%,#0f172a 100%);
 border:8px solid #334155;box-shadow:inset 0 0 30px rgba(0,0,0,.5),0 10px 35px rgba(0,0,0,.35)}
.tick,.hand{position:absolute;left:50%;top:50%;width:0;height:0}
.tick::after{content:"";position:absolute;left:-1px;bottom:104px;width:2px;height:8px;background:#475569}
.tick.major::after{height:14px;width:3px;left:-1.5px;background:#94a3b8}
.hand{transition:transform .12s linear}
.hand::after{content:"";position:absolute;left:-2px;bottom:0;width:4px;height:92px;background:#38bdf8;
 border-radius:4px;box-shadow:0 0 12px rgba(56,189,248,.7)}
.degree{position:absolute;font-size:12px;color:#94a3b8}
.d0{top:22px;left:50%;transform:translateX(-50%)}
.d90{right:22px;top:50%;transform:translateY(-50%)}
.d180{bottom:22px;left:50%;transform:translateX(-50%)}
.d270{left:18px;top:50%;transform:translateY(-50%)}
.dial-center{position:absolute;width:28px;height:28px;left:50%;top:50%;transform:translate(-50%,-50%);
 background:#e2e8f0;border-radius:50%;border:5px solid #0f172a;z-index:5}

/* ---------- controls ---------- */
.command-card h2{margin-top:0;font-size:18px;font-weight:500}
.input-row{display:flex;gap:10px;margin-top:15px}
input{flex:1;min-width:0;padding:13px;border-radius:8px;border:1px solid #334155;background:#0f172a;
 color:#f8fafc;font-size:17px;text-align:center;outline:none}
input:focus{border-color:#38bdf8}
button{border:none;border-radius:8px;padding:12px 15px;font-size:14px;font-weight:500;cursor:pointer;
 transition:transform .1s,opacity .1s}
button:active{transform:scale(.96)}
button:disabled{opacity:.35;cursor:not-allowed}
.primary{background:#0284c7;color:#fff}
.quick-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:16px}
.quick{background:#1e293b;color:#e2e8f0}
.quick:hover:not(:disabled){background:#334155}
.stop{width:100%;margin-top:18px;background:#dc2626;color:#fff}
.home{width:100%;margin-top:10px;background:#0f766e;color:#fff}
.err{min-height:18px;margin-top:10px;color:#f87171;font-size:13px}

/* ---------- status ---------- */
.status-card{grid-column:1/-1;display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap}
.status-name{color:#94a3b8;font-size:13px;text-transform:uppercase;letter-spacing:1px}
.status-value{font-size:15px;font-weight:600;color:#22c55e}
.status-value.moving,.status-value.homing,.status-value.stopping{color:#f59e0b}
.status-value.stopped{color:#94a3b8}
.status-value.fault,.status-value.offline{color:#ef4444}
.small{color:#94a3b8;font-size:13px}

@media(max-width:700px){
 .dashboard{grid-template-columns:1fr}
 .status-card{grid-column:auto}
 .quick-grid{grid-template-columns:repeat(2,1fr)}
}
</style>
</head>
<body>
<div class="container">
 <div class="header"><h1>OPQC Motor Control</h1><p>Optical Position Control</p></div>

 <div class="dashboard">

  <div class="card position-card">
   <div class="label">Current Position</div>
   <div class="angle-value"><span id="angle">0.0</span>&deg;</div>
   <div class="count-value">Encoder Count: <span id="count">0</span></div>

   <div class="dial-container">
    <div class="dial" id="dial">
     <div class="degree d0">0&deg;</div>
     <div class="degree d90">90&deg;</div>
     <div class="degree d180">180&deg;</div>
     <div class="degree d270">270&deg;</div>
     <div id="ticks"></div>
     <div class="hand" id="hand"></div>
     <div class="dial-center"></div>
    </div>
   </div>
  </div>

  <div class="card command-card">
   <h2>Position Command</h2>
   <div class="input-row">
    <input type="number" id="angleInput" step="18" placeholder="Angle">
    <button class="primary cmd" id="rotateButton" onclick="rotate()">ROTATE</button>
   </div>

   <h2 style="margin-top:28px">Quick Angles</h2>
   <div class="quick-grid" id="quickGrid"></div>

   <button class="stop" onclick="send('/stop')">STOP</button>
   <button class="home cmd" onclick="send('/home')">HOME</button>
   <div class="err" id="err"></div>
  </div>

  <div class="card status-card">
   <div><div class="status-name">System Status</div><div class="status-value" id="status">...</div></div>
   <div><div class="status-name">Homed</div><div class="small" id="homed">-</div></div>
   <div><div class="status-name">Target Count</div><div class="small" id="target">-</div></div>
   <div><div class="status-name">Message</div><div class="small" id="msg">-</div></div>
  </div>

 </div>
</div>

<script>
const $ = id => document.getElementById(id);

// If the shaft turns anticlockwise for positive counts, set this to -1
const DIAL_SIGN = 1;

// ---------- build ticks + quick buttons ----------
(function build(){
  const t = $('ticks');
  for (let i = 0; i < 20; i++) {
    const d = document.createElement('div');
    d.className = 'tick' + (i % 5 === 0 ? ' major' : '');
    d.style.transform = 'rotate(' + (i * 18) + 'deg)';
    t.appendChild(d);
  }
  const g = $('quickGrid');
  [-180,-90,-54,-36,-18,18,36,54,90,180].forEach(a => {
    const b = document.createElement('button');
    b.className = 'quick cmd';
    b.innerHTML = (a > 0 ? '+' : '') + a + '&deg;';
    b.onclick = () => send('/move?angle=' + a);
    g.appendChild(b);
  });
})();

// ---------- commands ----------
let errTimer = null;
function showErr(t) {
  $('err').textContent = t;
  clearTimeout(errTimer);
  errTimer = setTimeout(() => $('err').textContent = '', 3000);
}

async function send(url) {
  try {
    const r = await fetch(url, {cache: 'no-store'});
    if (!r.ok) showErr(await r.text());
  } catch (e) { showErr('Request failed'); }
}

function rotate() {
  const a = Number($('angleInput').value);
  if (!Number.isFinite(a) || a === 0) return;
  if (a % 18 !== 0) { showErr('Enter an angle in 18 degree steps.'); return; }
  send('/move?angle=' + a);
}

// ---------- dial: continuous angle, always takes the SHORT way round ----------
let lastAngle = null, dialAngle = 0;
function updateDial(a) {
  if (lastAngle === null) {
    dialAngle = a;
  } else {
    let d = a - lastAngle;
    d = (((d % 360) + 540) % 360) - 180;     // shortest signed difference
    dialAngle += d;
  }
  lastAngle = a;
  $('hand').style.transform = 'rotate(' + dialAngle + 'deg)';
}

// ---------- status polling (next poll only after the previous finished) ----------
function render(d) {
  $('count').textContent  = d.count;
  $('angle').textContent  = d.angle.toFixed(1);
  $('target').textContent = d.target;
  $('homed').textContent  = d.homed ? 'YES' : 'NO';
  $('msg').textContent    = d.msg;

  const s = $('status');
  s.textContent = d.status;
  s.className = 'status-value ' + d.status.toLowerCase();

  updateDial(DIAL_SIGN * d.angle);
  document.querySelectorAll('.cmd').forEach(b => b.disabled = d.busy);
}

async function poll() {
  try {
    const r = await fetch('/status', {cache: 'no-store'});
    render(await r.json());
  } catch (e) {
    $('status').textContent = 'OFFLINE';
    $('status').className = 'status-value offline';
  }
  setTimeout(poll, 100);
}
poll();
</script>
</body>
</html>
)rawliteral";

// ============================================================
// Web handlers
// ============================================================
void handleRoot()
{
    server.send(200, "text/html", MAIN_PAGE);
}

void handleStatus()
{
    long  count = getEncoderCount();
    float angle = count * DEGREES_PER_COUNT;

    String json = "{";
    json += "\"count\":"  + String(count);
    json += ",\"angle\":" + String(angle, 1);
    json += ",\"target\":" + String(targetPosition);
    json += ",\"status\":\"" + String(statusText()) + "\"";
    json += ",\"busy\":"  + String(isBusy() ? "true" : "false");
    json += ",\"homed\":" + String(homed ? "true" : "false");
    json += ",\"msg\":\"" + message + "\"";
    json += "}";

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
}

void handleMove()
{
    if (!server.hasArg("angle")) { server.send(400, "text/plain", "Missing angle"); return; }

    const char* why = startMove(server.arg("angle").toFloat());

    if (why) { server.send(409, "text/plain", why); return; }
    server.send(200, "text/plain", "Move started");
}

void handleStop()
{
    Serial.println("STOP command received");

    if (state == S_BRAKING || state == S_SETTLING)
    {
        pending = P_STOP;                      // already braking: just change the outcome
    }
    else if (isBusy())
    {
        beginBrake(P_STOP);
    }
    else
    {
        stopMotor();
        state   = S_STOPPED;
        message = "Stopped";
    }

    server.send(200, "text/plain", "Stopping");
}

void handleHome()
{
    if (!startHoming()) { server.send(409, "text/plain", "Busy"); return; }
    server.send(200, "text/plain", "Homing started");
}

// ============================================================
// Setup
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);
    stopMotor();

    pinMode(ENCODER_PIN, INPUT_PULLUP);
    lastLevel = digitalRead(ENCODER_PIN);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderISR, CHANGE);

    Serial.println("\n==============================");
    Serial.println("OPQC - WEB CONTROL");
    Serial.println("==============================");
    Serial.printf("Kp=%.2f Kd=%.2f | %d counts/rev | %.1f deg/count\n",
                  Kp, Kd, COUNTS_PER_REV, DEGREES_PER_COUNT);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    Serial.print("Wi-Fi SSID: ");   Serial.println(AP_SSID);
    Serial.print("IP Address: ");   Serial.println(WiFi.softAPIP());

    server.on("/",       handleRoot);
    server.on("/status", handleStatus);
    server.on("/move",   handleMove);
    server.on("/stop",   handleStop);
    server.on("/home",   handleHome);
    server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
    server.begin();

    Serial.println("Web server started. System READY.");
}

// ============================================================
// Loop
// ============================================================
void loop()
{
    server.handleClient();
    runStateMachine();
}
