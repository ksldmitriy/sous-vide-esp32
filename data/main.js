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

function on_ws_open(event) {}

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
  } catch (error) {
    console.error("Error parsing JSON:", error);
  }
}

function on_ws_error(event) {}

const data = [
  { time: 1, value: 10 },
  { time: 2, value: 10 },
  { time: 3, value: 10 },
  { time: 4, value: 10 },
  { time: 5, value: 10 },
  { time: 6, value: 15 },
  { time: 7, value: 23 },
  { time: 8, value: 15 },
  { time: 9, value: 10 },
];

let graph = {
  margin: { left: 48, right: 32, top: 32, bottom: 32 },
  svg: null,
  path: null,
  line: null,
  x_axis: null,
  y_axis: null,
};

function create_chart() {
  const svg = d3.select("#graph");
  const width = parseInt(svg.style("width"));
  const height = parseInt(svg.style("height"));
  const margin = graph.margin;
  // X axis
  const x = d3
    .scaleLinear()
    .domain([d3.max(data, (d) => 9 - d.time), d3.min(data, (d) => 9 - d.time)])
    .range([margin.left, width - margin.right]);

  graph.x_axis = svg
    .append("g")
    .attr("transform", `translate(0, ${height - margin.bottom})`)
    .call(d3.axisBottom(x).ticks(3));

  graph.x_axis.selectAll(".tick text").attr("class", "axis");

  // Y axis
  const y = d3
    .scaleLinear()
    .domain([
      d3.max(data, (d) => d.value) + 5,
      d3.min(data, (d) => d.value) - 5,
    ])
    .range([margin.top, height - margin.bottom]);

  graph.y_axis = svg
    .append("g")
    .attr("transform", `translate(${margin.left}, 0)`)
    .call(d3.axisLeft(y).ticks(10))
    .call((g) =>
      g
        .selectAll(".tick line")
        .clone()
        .attr("x2", width - margin.left - margin.right)
        .attr("stroke-opacity", 0.2),
    );

  graph.y_axis.selectAll(".tick text").attr("class", "axis");

  // data
  graph.line = d3
    .line()
    .x((d) => x(9 - d.time))
    .y((d) => y(d.value));

  graph.path = svg
    .datum([])
    .append("path")
    .attr("fill", "none")
    .attr("stroke", "black")
    .attr("stroke-width", 1.5)
    .attr("d", graph.line);

  update_chart();
}

function update_chart() {
  graph.path.datum(data).transition().attr("d", graph.line);
}

function on_load(event) {
  create_chart();
  //window.addEventListener("resize", update_chart);

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
