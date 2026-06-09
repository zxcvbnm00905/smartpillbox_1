const STATE_NORMAL = 0;
const STATE_ALARM = 1;
const KEY_NONE = 0;
const KEY2_DOWN = 2;

const medicines = [
  { hour: 7, minute: 30, enabled: 1, taken: 0, name: "Amoxicillin" },
];

let state = STATE_NORMAL;
let triggeredIdx = 0xff;
let lastAlarm = { idx: 0xff, day: 0xff, hour: 0xff, minute: 0xff };
let now = { day: 6, hour: 7, minute: 29, second: 50 };
let ms = 0;
let alarmStartMs = 0;
let lastAlarmGuardState = STATE_NORMAL;
let key2RawPressed = true;
let key2State = 0;
let beepOnCount = 0;
let ledOnCount = 0;
let beep = 0;
let led = 0;
let alarmElapsedSec = 0;
let alarmLastSecond = 0xff;
let snoozeIdx = 0xff;
let snoozeDueMinute = 0;
let snoozePressed = false;
let snoozeRetriggered = false;

function nowMinute() {
  return now.day * 1440 + now.hour * 60 + now.minute;
}

function keyScan() {
  if (key2RawPressed) {
    if (key2State === 0) {
      key2State = 1;
      return KEY2_DOWN;
    }
  } else {
    key2State = 0;
  }
  return KEY_NONE;
}

function alarmUpdateGuard() {
  if (state === STATE_ALARM && lastAlarmGuardState !== STATE_ALARM) {
    alarmStartMs = ms;
  }
  lastAlarmGuardState = state;
}

function alarmCanDismiss() {
  return state === STATE_ALARM && (ms - alarmStartMs) >= 2000;
}

function beepOn() {
  beep = 1;
  beepOnCount++;
}

function ledOn() {
  led = 1;
  ledOnCount++;
}

function processPillBox() {
  if (state === STATE_NORMAL) {
    if (snoozeIdx < medicines.length) {
      const med = medicines[snoozeIdx];
      if (!med.enabled || med.taken) {
        snoozeIdx = 0xff;
        snoozeDueMinute = 0;
      } else if (nowMinute() >= snoozeDueMinute) {
        const idx = snoozeIdx;
        snoozeIdx = 0xff;
        snoozeDueMinute = 0;
        startAlarm(idx);
        snoozeRetriggered = true;
        return;
      }
    }

    for (let i = 0; i < medicines.length; i++) {
      const med = medicines[i];
      const alreadyAlarmed =
        lastAlarm.idx === i &&
        lastAlarm.day === now.day &&
        lastAlarm.hour === now.hour &&
        lastAlarm.minute === now.minute;

      if (!med.enabled || med.taken || alreadyAlarmed) continue;

      if (med.hour === now.hour && med.minute === now.minute) {
        startAlarm(i);
        break;
      }
    }
  } else if (state === STATE_ALARM) {
    if (now.second !== alarmLastSecond) {
      alarmLastSecond = now.second;
      alarmElapsedSec++;
    }

    if (alarmElapsedSec >= 10 || now.hour !== lastAlarm.hour || now.minute !== lastAlarm.minute) {
      state = STATE_NORMAL;
      beep = 0;
      led = 0;
      triggeredIdx = 0xff;
      alarmElapsedSec = 0;
      alarmLastSecond = 0xff;
    } else if (now.second % 2 === 0) {
      beepOn();
      ledOn();
    } else {
      beep = 0;
      led = 0;
    }
  }
}

function startAlarm(idx) {
  state = STATE_ALARM;
  triggeredIdx = idx;
  lastAlarm = { idx, day: now.day, hour: now.hour, minute: now.minute };
  alarmElapsedSec = 0;
  alarmLastSecond = now.second;
  beepOn();
  ledOn();
}

function snooze() {
  if (state === STATE_ALARM && triggeredIdx < medicines.length) {
    snoozeIdx = triggeredIdx;
    snoozeDueMinute = nowMinute() + 5;
    snoozePressed = true;
  }
  state = STATE_NORMAL;
  triggeredIdx = 0xff;
  alarmElapsedSec = 0;
  alarmLastSecond = 0xff;
  beep = 0;
  led = 0;
}

function tickSecond() {
  now.second++;
  if (now.second >= 60) {
    now.second = 0;
    now.minute++;
  }
  if (now.minute >= 60) {
    now.minute = 0;
    now.hour++;
  }
  ms += 1000;
}

for (let i = 0; i < 380; i++) {
  processPillBox();
  alarmUpdateGuard();

  let key = KEY_NONE;
  if (state === STATE_ALARM && !snoozePressed && now.hour === 7 && now.minute === 30 && now.second === 3) {
    key = KEY2_DOWN;
    snooze();
  }

  if (
    (now.hour === 7 && now.minute === 30 && now.second <= 5) ||
    (now.hour === 7 && now.minute === 35 && now.second <= 12)
  ) {
    console.log(
      `${now.hour}:${String(now.minute).padStart(2, "0")}:${String(now.second).padStart(2, "0")} ` +
        `state=${state} beep=${beep} led=${led} key=${key} snoozeIdx=${snoozeIdx}`
    );
  }

  tickSecond();
}

if (!snoozePressed || !snoozeRetriggered || state !== STATE_NORMAL || beep !== 0 || led !== 0) {
  console.error("FAIL: snooze/retrigger/auto-stop behavior is wrong");
  process.exit(1);
}

console.log(`PASS: K2 snooze retriggered after 5 minutes and auto-stopped, beepOnCount=${beepOnCount}, ledOnCount=${ledOnCount}`);
