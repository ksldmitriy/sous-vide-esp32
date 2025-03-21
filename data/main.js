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
  console.log("Connection opened");
}

function on_ws_close(event) {
  console.log("Connection closed");
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
  } catch (error) {
    console.error("Error parsing JSON:", error);
  }
}

function on_ws_error(event) {
  console.log("Socket error:", event);
}

function create_test_graph() {
  const xValues = [50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150];
  const yValues = [7, 8, 8, 9, 9, 9, 10, 11, 14, 14, 15];

  new Chart("graph-canvas", {
    type: "line",
    data: {
      labels: xValues,
      datasets: [
        {
          fill: false,
          lineTension: 0,
          backgroundColor: "rgba(0,0,255,1.0)",
          borderColor: "rgba(0,0,255,0.1)",
          data: yValues,
        },
      ],
    },
    options: {
      legend: { display: false },
      scales: {
        yAxes: [{ ticks: { min: 6, max: 16, fontSize: 32 } }],
        xAxes: [{ ticks: { fontSize: 32 } }],
      },

      plugins: {
        legend: {
          font: {
            size: 64,
          },
        },
      },
    },
  });
}

function on_load(event) {
  create_test_graph();

  init_socket();

  document
    .getElementById("target-temperature")
    .addEventListener("input", on_target_temperature_input);
  document
    .getElementById("target-temperature")
    .addEventListener("keypress", (event) => {
      if (event.key === "Enter") {
        should_apply_changes = true;
        event.target.blur();
        on_target_temperature_confirm(event.target.value);
      }
    });
  document
    .getElementById("target-temperature")
    .addEventListener("blur", on_target_temperature_blur);
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

  console.log("value:", float_value);

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
