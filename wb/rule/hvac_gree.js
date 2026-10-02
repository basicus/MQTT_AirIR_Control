//-----------------------------------------------------------------------------
// hvac_gree.js — управление кондиционером Gree через ИК-передатчик
// (прошивка https://github.com/basicus/MQTT_AirIR_Control, не изменяется).
//
// ВАЖНО про совместимость:
// Прошивка ESP8266 подписана НАПРЯМУЮ на "сырые" топики значений:
//   /devices/hvac_gree/controls/enabled
//   /devices/hvac_gree/controls/mode
//   /devices/hvac_gree/controls/fanspeed
//   /devices/hvac_gree/controls/temperature
// и реагирует на любую публикацию в них (а не на команды через ".../on").
// Сама она ничего не публикует и /meta не понимает.
//
// Актуальная конвенция WB (https://github.com/wirenboard/conventions)
// не меняет эти топики значений — просто описывает /meta одним JSON.
// Поэтому ниже виртуальное устройство объявлено с ТЕМ ЖЕ id "hvac_gree"
// и с теми же именами ячеек: движок правил сам публикует текущее
// значение в /devices/hvac_gree/controls/<cell> при каждом изменении —
// неважно, инициировано оно правилом, веб-интерфейсом или записью в
// .../on — то есть прошивка продолжит получать команды как раньше.
//-----------------------------------------------------------------------------

defineVirtualDevice("hvac_gree", {
  title: {
    en: "Gree air conditioner",
    ru: "Кондиционер Gree"
  },

  cells: {
    enabled: {
      type: "switch",
      value: true,
      order: 1,
      title: { en: "Power", ru: "Питание" }
    },

    // ВАЖНО: тип "value" (даже с enum) в автоматическом виджете устройства
    // WB часто рендерится как read-only индикатор — редактируемость на
    // дашборде нужно включать отдельно вручную. Тип "range" гарантированно
    // даёт интерактивный слайдер (это подтверждено ячейкой temperature),
    // поэтому mode/fanspeed возвращены к "range", как в оригинальном
    // устройстве. Человекочитаемое название режима/скорости показывается
    // в ячейке status ниже.
    mode: {
      type: "range",
      value: 4,
      min: 0,
      max: 4,
      order: 2,
      title: { en: "Mode", ru: "Режим (0-Авто 1-Охлаждение 2-Осушение 3-Вентиляция 4-Обогрев)" }
    },

    fanspeed: {
      type: "range",
      value: 0,
      min: 0,
      max: 3,
      order: 3,
      title: { en: "Fan speed", ru: "Скорость вентилятора (0-Авто 1-Низкая 2-Средняя 3-Высокая)" }
    },

    temperature: {
      type: "range",
      value: 25,
      min: 16,
      max: 30,
      order: 4,
      title: { en: "Setpoint", ru: "Температура" },
      units: "deg C"
    },

    // Доп. read-only ячейка для наглядной сводки состояния —
    // аналог подписи на старом виджете
    // ("On   T=25°C  Mode: Heat  FanSpeed: Auto").
    status: {
      type: "text",
      value: "",
      readonly: true,
      order: 5,
      title: { en: "Status", ru: "Статус" }
    }
  }
});

//-----------------------------------------------------------------------------
// Формирование текстовой сводки в status при изменении любой из ячеек
//-----------------------------------------------------------------------------

var MODE_NAMES_RU = {
  0: "Авто",
  1: "Охлаждение",
  2: "Осушение",
  3: "Вентиляция",
  4: "Обогрев"
};

var FAN_NAMES_RU = {
  0: "Авто",
  1: "Низкая",
  2: "Средняя",
  3: "Высокая"
};

function updateGreeStatus() {
  var d = getDevice("hvac_gree");
  var isOn = d.getControl("enabled").getValue();
  var mode = d.getControl("mode").getValue();
  var fan = d.getControl("fanspeed").getValue();
  var temp = d.getControl("temperature").getValue();

  var text = isOn
    ? "Вкл, " + temp + "°C, режим: " + (MODE_NAMES_RU[mode] || mode) +
      ", вентилятор: " + (FAN_NAMES_RU[fan] || fan)
    : "Выкл";

  d.getControl("status").setValue(text);
}

defineRule("hvac_gree_status_update", {
  whenChanged: [
    "hvac_gree/enabled",
    "hvac_gree/mode",
    "hvac_gree/fanspeed",
    "hvac_gree/temperature"
  ],
  then: function () {
    updateGreeStatus();
  }
});

// Сформировать сводку сразу при запуске скрипта (без ожидания первого
// изменения ячейки)
updateGreeStatus();
