var gateway = `ws://${window.location.hostname}/ws`;
var socket;
window.addEventListener("load", on_load);

var last_timestamp = Number.MIN_SAFE_INTEGER;
var target_temperature = 10;
var should_apply_changes = false;

function init_socket() {
  socket = new WebSocket(gateway);
  socket.onopen = on_ws_open;
  socket.onclose = on_ws_close;
  socket.onmessage = on_ws_message;
  socket.onerror = on_ws_error;
}

function on_ws_open(event) {
  console.log("socket connected");
}

function on_ws_close(event) {
  setTimeout(init_socket, 2000);
}

function on_ws_message(event) {
  try {
    const data = JSON.parse(event.data);

    if (data.hasOwnProperty("current_temperature")) {
      const element = document.getElementById("current-temperature");
      element.textContent = data.current_temperature.toFixed(1) + "°C";
    }
    if (data.hasOwnProperty("target_temperature")) {
      const element = document.getElementById("target-temperature");
      target_temperature = data.target_temperature;
      if (document.activeElement !== element) {
        element.value = target_temperature.toFixed(1);
      }
    }
    if(data.hasOwnProperty("heater_state")){
      const element = document.getElementById("heater-state");
      element.checked = data.heater_state;
    }
  } catch (error) {
    console.error("Error parsing JSON:", error);
  }
}

function on_ws_error(event) {}

function on_load(event) {
  init_socket();

  // on/off switch
  document
    .getElementById("heater-state")
    .addEventListener("click", on_checkbox_click);

  // target temperature
  const target_temperature = document.getElementById("target-temperature");
  target_temperature.addEventListener("input", on_target_temperature_input);
  target_temperature.addEventListener("keypress", (event) => {
    if (event.key === "Enter") {
      should_apply_changes = true;
      event.target.blur();
      on_target_temperature_confirm(event.target.value);
    }
  });
  target_temperature.addEventListener("blur", on_target_temperature_blur);
}

function on_checkbox_click(event) {
  event.preventDefault();

  const data = {
    heater_state: this.checked,
  };

  const json_string = JSON.stringify(data);
  console.log(json_string)
  socket.send(json_string);
}

function on_target_temperature_input() {
  let value = this.value;
  if (value === "") {
    this.value = 0;
    return;
  }

  value = value.replace(/[^0-9.]/g, "");

  if ((value.match(/\./g) || []).length > 1) {
    value = value.slice(0, -1);
  }

  let number = parseFloat(value);

  if (!isNaN(number)) {
    if (number < 0) number = 0;
    if (number > 95) number = 95;
    value = number;
  }

  this.value = value;
}

function on_target_temperature_confirm(value) {
  let float_value = parseFloat(value);

  const data = {
    target_temperature: float_value,
  };

  const json_string = JSON.stringify(data);
  socket.send(json_string);
}

function on_target_temperature_blur() {
  if (should_apply_changes) {
    should_apply_changes = false;
  } else {
    if (target_temperature == null) {
      this.value = "";
    } else {
      this.value = target_temperature.toFixed(1);
    }
  }
}
