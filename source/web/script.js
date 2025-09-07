const firebaseConfig = {
  apiKey: " YOUR_API_KEY",
  authDomain: "YOUR_AUTH_DOMAIN",
  databaseURL: "YOUR_URL",
  projectId: "YOUR_ID",
  storageBucket: "YOUR_BUCKET",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID",
  measurementId: "YOUR_MEASUREMENT_ID"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// DOM elements
const tableBody = document.getElementById("table-body");
const alertContainer = document.getElementById("alert-container");
const alertSound = document.getElementById("alert-sound");
const downloadCsvBtn = document.getElementById("download-csv");
const expandTableBtn = document.getElementById("expand-table");
const loadingSpinner = document.getElementById("loading-spinner");
// Phone config elements (may be absent in older pages)
const phoneInput = document.getElementById("phone-input");
const savePhoneBtn = document.getElementById("save-phone");
const phoneStatus = document.getElementById("phone-status");
let isEditingPhone = false;
let lastServerPhone = "";
if (phoneInput) {
  phoneInput.addEventListener("focus", () => { isEditingPhone = true; });
  phoneInput.addEventListener("input", () => { isEditingPhone = true; });
  phoneInput.addEventListener("blur", () => { setTimeout(() => { isEditingPhone = false; }, 300); });
}

let isTableExpanded = false;
let cameraAlertTimeout = null;
const CAMERA_ALERT_DURATION = 15000; // 30 seconds

// Initialize Chart.js
const ctx = document.getElementById("sensor-chart").getContext("2d");
const sensorChart = new Chart(ctx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Nhiệt độ (°C)",
        data: [],
        borderColor: "#dc3545",
        fill: false,
        tension: 0.3
      },
      {
        label: "Độ ẩm (%)",
        data: [],
        borderColor: "#007bff",
        fill: false,
        tension: 0.3
      },
      {
        label: "CO",
        data: [],
        borderColor: "#28a745",
        fill: false,
        tension: 0.3
      },
      {
        label: "GAS",
        data: [],
        borderColor: "#ff9800",
        fill: false,
        tension: 0.3
      },
      {
        label: "Nhiệt độ môi trường (°C)",
        data: [],
        borderColor: "#6b7280",
        fill: false,
        tension: 0.3
      },
      {
        label: "Nhiệt độ vật thể (°C)",
        data: [],
        borderColor: "#eab308",
        fill: false,
        tension: 0.3
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      x: {
        title: { display: true, text: "Thời gian" }
      },
      y: {
        title: { display: true, text: "Giá trị" }
      }
    },
    plugins: {
      legend: { position: "top" }
    }
  }
});


// MQTT Configuration
const MQTT_BROKER = "bd92efa291a44e2497fa3e60484e25c3.s1.eu.hivemq.cloud";
const MQTT_PORT = 8884;
const MQTT_USER = "linh_mqtt";
const MQTT_PASSWORD = "Linhsama22122003";
const MQTT_CLIENT_ID = "Web_Client_" + Math.random().toString(16).slice(3);
const MQTT_CAMERA_TOPIC = "raspberrypi/camera";
// Topics for phone number configuration
const TOPIC_SET_PHONE = "esp32/Phone";
const TOPIC_CURRENT_PHONE = "esp32/Current_Phone";

// Initialize MQTT Client
const mqttClient = new Paho.MQTT.Client(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);

// MQTT Callbacks
mqttClient.onConnectionLost = (responseObject) => {
  if (responseObject.errorCode !== 0) {
    console.error("MQTT Connection Lost:", responseObject.errorMessage);
    setTimeout(connectMqtt, 5000); // Reconnect after 5 seconds
  }
};

mqttClient.onMessageArrived = (message) => {
  try {
    // Handle current phone update from ESP32
    if (message.destinationName === TOPIC_CURRENT_PHONE) {
      let num = "";
      try {
        const obj = JSON.parse(message.payloadString);
        num = obj.Phone || "";
      } catch (_) {
        num = message.payloadString;
      }
      lastServerPhone = typeof num === "string" ? num : lastServerPhone;
      if (phoneStatus && lastServerPhone) {
        phoneStatus.textContent = `Đang dùng: ${lastServerPhone}`;
        phoneStatus.style.color = "#16a34a";
      }
      // Chỉ tự động điền khi không gõ hoặc ô đang trống/đã hiển thị số cũ từ server
      if (phoneInput && lastServerPhone && !isEditingPhone) {
        const cur = (phoneInput.value || "").trim();
        if (cur === "" || cur === lastServerPhone) {
          phoneInput.value = lastServerPhone;
        }
      }
      return;
    }

    const payload = JSON.parse(message.payloadString);
    if (payload.fire_detected) {
      // Clear any existing timeout to extend the alert duration
      if (cameraAlertTimeout) {
        clearTimeout(cameraAlertTimeout);
      }
      alertContainer.classList.remove("hidden");
      alertContainer.classList.add("warning");
      alertContainer.textContent = `CẢNH BÁO: Phát hiện lửa bởi camera! `;
      alertSound.play().catch(err => console.error("Error playing sound:", err));
      // Set timeout to keep alert visible for CAMERA_ALERT_DURATION
      cameraAlertTimeout = setTimeout(() => {
        // Check sensor status before hiding alert
        db.ref("Status").limitToLast(1).once("value", snapshot => {
          const data = snapshot.val();
          if (data) {
            const latestEntry = Object.values(data)[0];
            if (latestEntry.flame !== 0) {
              alertContainer.classList.add("hidden");
              alertContainer.classList.remove("warning");
              alertContainer.textContent = "";
            }
          } else {
            alertContainer.classList.add("hidden");
            alertContainer.classList.remove("warning");
            alertContainer.textContent = "";
          }
        });
      }, CAMERA_ALERT_DURATION);
    } else {
      // Only hide camera alert if timeout has expired and no sensor fire
      if (!cameraAlertTimeout) {
        db.ref("Status").limitToLast(1).once("value", snapshot => {
          const data = snapshot.val();
          if (data) {
            const latestEntry = Object.values(data)[0];
            if (latestEntry.flame !== 0) {
              alertContainer.classList.add("hidden");
              alertContainer.classList.remove("warning");
              alertContainer.textContent = "";
            }
          } else {
            alertContainer.classList.add("hidden");
            alertContainer.classList.remove("warning");
            alertContainer.textContent = "";
          }
        });
      }
    }
  } catch (error) {
    console.error("Error parsing MQTT message:", error);
  }
};

// Connect to MQTT Broker
function connectMqtt() {
  mqttClient.connect({
    userName: MQTT_USER,
    password: MQTT_PASSWORD,
    useSSL: true,
    onSuccess: () => {
      console.log("Connected to MQTT Broker");
      mqttClient.subscribe(MQTT_CAMERA_TOPIC);
      console.log(`Subscribed to topic: ${MQTT_CAMERA_TOPIC}`);
      mqttClient.subscribe(TOPIC_CURRENT_PHONE);
      console.log(`Subscribed to topic: ${TOPIC_CURRENT_PHONE}`);
    },
    onFailure: (err) => {
      console.error("MQTT Connection Failed:", err.errorMessage);
      setTimeout(connectMqtt, 5000); // Reconnect after 5 seconds
    }
  });
}

// Start MQTT connection
connectMqtt();

//Camera stream
const camImg = document.getElementById("live-cam");

// Kết nối socket.io – để mặc định cho phép fallback long-polling nếu server không hỗ trợ WebSocket
const socket = io("http://192.168.1.143:5000");

socket.on("connect", () => console.log("Socket.IO connected"));
socket.on("frame", (data) => {
  camImg.src = "data:image/jpeg;base64," + data;
});
socket.on("disconnect", () => console.warn("Socket.IO disconnected"));



// Download CSV function
function downloadCSV(data) {
  const headers = ["Thời gian", "Nhiệt độ (°C)", "Độ ẩm (%)", "CO", "GAS", "Lửa", "Nhiệt độ môi trường (°C)", "Nhiệt độ vật thể (°C)"];
  const rows = data.map(entry => [
    entry.timestamp || "",
    entry.temperature || "?",
    entry.humidity || "?",
    entry.co_level || "?",
    entry.gas_level || "?",
    entry.flame === 0 ? "Có lửa" : "Không có lửa",
    entry.ambient_temp || "?",
    entry.object_temp || "?"
  ]);

  const csvContent = [
    headers.join(","),
    ...rows.map(row => row.map(cell => `"${cell}"`).join(","))
  ].join("\n");

  const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
  const link = document.createElement("a");
  const url = URL.createObjectURL(blob);
  link.setAttribute("href", url);
  link.setAttribute("download", "sensor_data.csv");
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

// Render table function
function renderTable(entries, limit = 10) {
const rows = entries.slice(0, limit).map((entry, index) => `
  <tr>
    <td id="timestamp-${index}">${entry.timestamp || ""}</td>
    <td id="temperature-${index}">${entry.temperature || "?"}</td>
    <td id="humidity-${index}">${entry.humidity || "?"}</td>
    <td id="co_level-${index}">${entry.co_level || "?"}</td>
    <td id="gas_level-${index}">${entry.gas_level || "?"}</td>
    <td id="flame-${index}" data-tooltip="${entry.flame === 0 ? 'Cảnh báo: Có lửa!' : 'An toàn'}" class="${entry.flame === 0 ? 'fire-detected' : ''}">${entry.flame === 0 ? "Có lửa" : "Không có lửa"}</td>
    <td id="ambient_temp-${index}">${entry.ambient_temp || "?"}</td>
    <td id="object_temp-${index}">${entry.object_temp || "?"}</td>
  </tr>
`).join("");
  tableBody.innerHTML = rows;
}

// Firebase data listener
db.ref("Status").on("value", snapshot => {
  loadingSpinner.classList.remove("hidden");
  const data = snapshot.val();
  tableBody.innerHTML = "";

  if (data) {
    const entries = Object.values(data).reverse();

    // Render table with limit or full
    renderTable(entries, isTableExpanded ? entries.length : 10);
    loadingSpinner.classList.add("hidden");

    // Update chart (show last 10 records)
    sensorChart.data.labels = entries.slice(0, 10).reverse().map(entry => entry.timestamp || "");
    sensorChart.data.datasets[0].data = entries.slice(0, 10).reverse().map(entry => entry.temperature || 0);
    sensorChart.data.datasets[1].data = entries.slice(0, 10).reverse().map(entry => entry.humidity || 0);
    sensorChart.data.datasets[2].data = entries.slice(0, 10).reverse().map(entry => entry.co_level || 0);
    sensorChart.data.datasets[3].data = entries.slice(0, 10).reverse().map(entry => entry.gas_level || 0);
    sensorChart.data.datasets[4].data = entries.slice(0, 10).reverse().map(entry => entry.ambient_temp || 0);
    sensorChart.data.datasets[5].data = entries.slice(0, 10).reverse().map(entry => entry.object_temp || 0);
    sensorChart.update();

    // Check for sensor fire alert
    const latestEntry = entries[0];

    db.ref("Threshold").limitToLast(1).once("value", thresholdSnap => {
  const thresholdData = thresholdSnap.val();
  const threshold = thresholdData ? Object.values(thresholdData)[0] : null;

  if (threshold && latestEntry) {
    const fields = [
      { key: "temperature", label: "Nhiệt độ", id: "temperature-0" },
      { key: "humidity", label: "Độ ẩm", id: "humidity-0", inverse: true }, // 👈 humidity kiểm tra nhỏ hơn
      { key: "co_level", label: "CO", id: "co_level-0" },
      { key: "gas_level", label: "Gas", id: "gas_level-0" },
      { key: "ambient_temp", label: "Nhiệt độ môi trường", id: "ambient_temp-0" },
      { key: "object_temp", label: "Nhiệt độ vật thể", id: "object_temp-0" }
    ];

    let alerts = [];

    fields.forEach(field => {
      const current = latestEntry[field.key];
      const limit = threshold[field.key];

      const cell = document.getElementById(field.id);
      if (!cell) return;

      // 👇 Nếu là humidity (inverse), thì so sánh < ngưỡng
      const isExceed = field.inverse
        ? current < limit
        : current > limit;

      if (typeof current === "number" && typeof limit === "number" && isExceed) {
        alerts.push(`${field.label} vượt ngưỡng`);
        cell.classList.add("exceed-limit");
      } else {
        cell.classList.remove("exceed-limit");
      }
    });

    if (alerts.length > 0) {
      alertContainer.classList.remove("hidden");
      alertContainer.classList.add("warning");
      alertContainer.textContent = "⚠️ CẢNH BÁO: " + alerts.join(", ");
      alertSound.play().catch(err => console.error("Error playing sound:", err));
    }
  }
});


    if (latestEntry && latestEntry.flame === 0) {
      alertContainer.classList.remove("hidden");
      alertContainer.classList.add("warning");
      alertContainer.textContent = "CẢNH BÁO: Phát hiện lửa nhận được từ cảm biến!";
      alertSound.play().catch(err => console.error("Error playing sound:", err));
    } else {
      // Only hide sensor alert if no camera fire is active
      if (!cameraAlertTimeout) {
        alertContainer.classList.add("hidden");
        alertContainer.classList.remove("warning");
        alertContainer.textContent = "";
      }
    }

    // Attach CSV download event
    downloadCsvBtn.onclick = () => downloadCSV(entries);

    // Attach table expand event
    expandTableBtn.onclick = () => {
      isTableExpanded = !isTableExpanded;
      renderTable(entries, isTableExpanded ? entries.length : 10);
      expandTableBtn.innerHTML = `<i class="fas fa-chevron-${isTableExpanded ? "up" : "down"}"></i> ${isTableExpanded ? "Thu gọn" : "Xem thêm"}`;
      expandTableBtn.classList.toggle("expanded", isTableExpanded);
    };
  } else {
    tableBody.innerHTML = `<tr><td colspan="7">Không có dữ liệu</td></tr>`;
    alertContainer.classList.add("hidden");
    loadingSpinner.classList.add("hidden");
    sensorChart.data.labels = [];
    sensorChart.data.datasets.forEach(dataset => dataset.data = []);
    sensorChart.update();
  }
}, error => {
  console.error("Firebase error:", error);
  tableBody.innerHTML = `<tr><td colspan="7">Lỗi khi tải dữ liệu: ${error.message}</td></tr>`;
  loadingSpinner.classList.add("hidden");
  alertContainer.classList.add("hidden");
});

// ========================= PHONE NUMBER CONFIG =========================
function onlyDigits(v){return (v||"").replace(/\D/g,"");}
function validPhoneDigits(v){const d=onlyDigits(v);return /^0\d{8,10}$/.test(d)?d:null;}

savePhoneBtn && savePhoneBtn.addEventListener("click", ()=>{
  const digits = validPhoneDigits(phoneInput.value);
  if(!digits){
    if (phoneStatus){ phoneStatus.textContent = "Số điện thoại không hợp lệ. Nhập 9–11 chữ số bắt đầu bằng 0."; phoneStatus.style.color = "#dc2626"; }
    return;
  }
  const msg = new Paho.MQTT.Message(digits);
  msg.destinationName = TOPIC_SET_PHONE;
  msg.qos = 1;
  msg.retained = true; // ensure ESP32 receives after reconnect
  mqttClient.send(msg);
  if (phoneStatus){ phoneStatus.textContent = `Đã gửi yêu cầu cập nhật: ${digits}`; phoneStatus.style.color = "#16a34a"; }
  // Sau khi gửi, coi như không còn sửa nữa để lần cập nhật server có thể đổ vào
  isEditingPhone = false;
});
