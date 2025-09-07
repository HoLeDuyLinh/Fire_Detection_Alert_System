# ==============================================================
# 0. IMPORTS & CAU HÌNH CHUNG
# ==============================================================

from ultralytics import YOLO
import torch, torch.nn as nn
import cv2, numpy as np, json, ssl, os, csv, time, threading
from datetime import datetime
from pathlib import Path
import paho.mqtt.client as mqtt
import pyrebase
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.mime.base import MIMEBase
from email import encoders

from flask import Flask, render_template
from flask_socketio import SocketIO
import base64
import threading
import time
import queue
import serial


ser = serial.Serial('/dev/serial0', baudrate=115200, timeout=1)

app = Flask(__name__)
socketio = SocketIO(app, async_mode="threading")




# ---------- Fire-spread mapping ----------
LABEL_MAP   = ['Incipient', 'Developed', 'Critical']
LABEL_COLOR = {'Incipient':(255,100,0),'Developed':(0,255,255),'Critical':(0,0,255)}
SEQ_LEN   = 5    # So frame liên tiep đưa vào LSTM
COOLDOWN  = 20   # Khoang cách toi thieu (giây) giữa 2 email canh báo liên tiep 
# ---------- Model / path ----------
YOLO_PATH   = '/home/pi/fire_alert/fire_det_best.pt'
LSTM_PATH   = '/home/pi/fire_alert/fire_lstm.pt'
CAPTURE_DIR = '/home/pi/fire_alert/captures'
CSV_PATH    = '/home/pi/fire_alert/captures/fire_log.csv'

# ---------- Email ----------
SMTP = {"host":"smtp.gmail.com","port":465,
        "sender":"linhsama2212@gmail.com","password":"bghn qgud dddp iveu"}
RECIPIENT = "holeduylinh2212@gmail.com"


# ---------- Firebase ----------
FB_CFG = { 
    "apiKey":"AIzaSyB4wjvk7v8AVVJscn0BM_HoA1hcufWK5C4", 
    "authDomain":"firedetect-f1fb2.firebaseapp.com", 
    "databaseURL":"https://firedetect-f1fb2-default-rtdb.asia-southeast1.firebasedatabase.app/", 
    "storageBucket":"firedetect-f1fb2.appspot.com" 
    }
firebase = pyrebase.initialize_app(FB_CFG)
db       = firebase.database()

# ---------- MQTT ----------
BROKER = "bd92efa291a44e2497fa3e60484e25c3.s1.eu.hivemq.cloud"
PORT   = 8883
USER   = "linh_mqtt"
PASS   = "Linhsama22122003"
TOPIC_SENSOR      = "esp32/sensors"
TOPIC_LIMITVALUE  = "esp32/limit_value"
TOPIC_CAM_RESULT  = "raspberrypi/camera"
CLIENT_ID         = "RaspberryPi_Client"

# ==============================================================
# 1. HÀM TIỆN ÍCH – EMAIL + CSV
# ==============================================================

def send_email(level,img_path):
    now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    subj = f"[ALERT] Fire - {level.upper()} @ {now}"
    body = f"<h2>Fire Severity: {level}</h2><p>Timestamp: {now}</p>"

    msg = MIMEMultipart();  msg['From']=SMTP["sender"]; msg['To']=RECIPIENT; msg['Subject']=subj
    msg.attach(MIMEText(body,'html'))
    with open(img_path,'rb') as f:
        part = MIMEBase('application','octet-stream'); part.set_payload(f.read()); encoders.encode_base64(part)
        part.add_header('Content-Disposition',f'attachment; filename="{os.path.basename(img_path)}"')
        msg.attach(part)

    with smtplib.SMTP_SSL(SMTP["host"],SMTP["port"]) as s:
        s.login(SMTP["sender"],SMTP["password"]); s.send_message(msg)
    print("Email sent.")

def log_csv(row):
    Path(CAPTURE_DIR).mkdir(exist_ok=True,parents=True)
    new = not Path(CSV_PATH).exists()
    with open(CSV_PATH,'a',newline='') as f:
        w=csv.writer(f)
        if new:
            w.writerow(['ts','level','conf','area','cx','cy','ar','x1','y1','x2','y2','img'])
        w.writerow(row)

# ==============================================================
# 2. LUONG MQTT  –  TIEP NHẬN CAM BIEN & LƯU FIREBASE
# ==============================================================

def on_mqtt_connect(cl,u,f,rc):
    if rc==0:
        print("MQTT connected.");  cl.subscribe([(TOPIC_SENSOR,0),(TOPIC_LIMITVALUE,0)])
    else: print("MQTT connect fail",rc)

def on_message(cl,u,msg):
    try:
        data=json.loads(msg.payload.decode()); ts=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        if msg.topic==TOPIC_SENSOR:
            db.child("Status").push({**data,"timestamp":ts})
            if data.get("flame")==0:
                print("Flame sensor triggered!")
        elif msg.topic==TOPIC_LIMITVALUE:
            db.child("Threshold").push({**data,"timestamp":ts})
    except Exception as e: print("MQTT msg err:",e)

def start_mqtt():
    cl = mqtt.Client(client_id=CLIENT_ID, protocol=mqtt.MQTTv311)
    cl.username_pw_set(USER, PASS)
    cl.tls_set(tls_version=ssl.PROTOCOL_TLS)
    cl.tls_insecure_set(True)

    cl.on_connect = on_mqtt_connect      #  ← gán đúng
    cl.on_message = on_message      #  ← gán đúng

    while True:
        try:
            cl.connect(BROKER, PORT)
            print("MQTT connected …")
            break
        except Exception as e:
            print("Retry MQTT:", e)
            time.sleep(5)

    cl.loop_start()
    return cl


# ==============================================================
# 3. MODEL LSTM + YOLO
# ==============================================================

class SpreadLSTM(nn.Module):
    def __init__(self, d_in=4, d_hid=64, n_cls=3):
        super().__init__()
        self.lstm = nn.LSTM(d_in, d_hid, num_layers=2,
                            dropout=0.2, batch_first=True)
        self.fc   = nn.Linear(d_hid, n_cls)

    def forward(self, x):
        #Dam bao tham so da duoc flatten moi lan goi
        self.lstm.flatten_parameters()
        _, (h, _) = self.lstm(x)
        return self.fc(h[-1])

yolo = YOLO(YOLO_PATH)  #Khoi tao model YOLO

lstm = SpreadLSTM().to('cpu')
lstm.load_state_dict(torch.load(LSTM_PATH, map_location='cpu'))
lstm.eval()

def extract(frame):
    res = yolo.predict(frame, conf=0.25, imgsz=320, verbose=False)[0]
    h, w = res.orig_shape

    # lay các box class-id = 0 (fire)
    fire = [(b, conf) for b, cls, conf
            in zip(res.boxes.xyxy.cpu(), res.boxes.cls.cpu(), res.boxes.conf.cpu())
            if cls == 0]

    if fire:
        (b, conf) = max(fire,
                        key=lambda s: (s[0][2]-s[0][0]) * (s[0][3]-s[0][1]))
        x1, y1, x2, y2 = map(float, b)           # <-- ép sang float
        conf          = float(conf)
        area          = (x2 - x1) * (y2 - y1)
        cx,  cy       = (x1 + x2) / 2, (y1 + y2) / 2
        ar            = (x2 - x1) / (y2 - y1 + 1e-6)
    else:
        x1 = y1 = x2 = y2 = area = cx = cy = ar = conf = 0.0

    feats = [round(area/(w*h), 5),
             round(cx/w,       5),
             round(cy/h,       5),
             round(ar,         5)]
    return feats, res, conf, (x1, y1, x2, y2)


# ==============================================================
# 4. LUONG CAMERA + XU LY
# ==============================================================

def camera_loop():
    global cap
    seq = []
    last_alert_t = 0
    cap = cv2.VideoCapture(0)
    Path(CAPTURE_DIR).mkdir(parents=True, exist_ok=True)
    if not cap.isOpened():
        raise RuntimeError("Camera not found")

    while True:
        ret, frm = cap.read()
        
        if not ret:
            time.sleep(0.3)
            continue

        try:
            feats, res, conf, bbox = extract(cv2.resize(frm, (320, 320)))
        except Exception as e:
            print("EXTRACT ERR:", e)
            with open("run_err.log", "a") as f:
                f.write(f"{datetime.now()} | {e}\n")
            continue        # bo frame loi, xu lý frame ke tiep
        # -----------------------------
        fire_present = conf > 0.25
        seq.append(feats)
        seq = seq[-SEQ_LEN:]

  
        label = "Waiting..."
        if len(seq) == SEQ_LEN:
            x = torch.tensor([seq], dtype=torch.float32)
            label = LABEL_MAP[lstm(x).argmax(1).item()]

        # Publish MQTT
        payload = {
            "fire_detected": fire_present,
            "sread_level": label,
            "confidence": float(conf),
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }
        mqtt_cl.publish(TOPIC_CAM_RESULT, json.dumps(payload))

        
        if label == "Critical" and time.time() - last_alert_t > COOLDOWN:
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            img_path = f"{CAPTURE_DIR}/critical_{ts}.jpg"
            cv2.imwrite(img_path, frm)
            try:
                send_email(label, img_path)
            except Exception as e:
                print("Email err:", e)
            log_csv([datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                     label, round(conf, 3), *feats,
                     *[round(v, 2) for v in bbox], img_path])
            last_alert_t = time.time()
            
        if label != "Incipient" and label != "Waiting...":
          message = label+ '\n'
          ser.write(message.encode())
        view = np.ascontiguousarray(res.plot())
        cv2.putText(view, f"Fire Spread Level: {label}", (0, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, LABEL_COLOR.get(label, (0, 0, 255)), 2)
        
        video = cv2.resize(view, (320, 240))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
        _, buffer = cv2.imencode('.jpg', video, encode_param)
        encoded = base64.b64encode(buffer).decode('utf-8')

        try:
            if not frame_queue.full():
                frame_queue.put_nowait(encoded)
        except queue.Full:
            pass

#        cv2.imshow("Fire Detection Realtime", cv2.resize(view, None, fx=1.3, fy=1.3))
#        if cv2.waitKey(1) & 0xFF == ord('q'):
#            break

    cap.release()
    cv2.destroyAllWindows()


def video_emitter():
    while True:
        try:
            encoded = frame_queue.get(timeout=1)  # chờ frame
            socketio.emit('video_frame', {'data': encoded})
            socketio.sleep(0.01)
        except queue.Empty:
            continue



@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def on_connect():
    print("Client connected")



# ==============================================================
# 5. MAIN
# ==============================================================
frame_queue = queue.Queue(maxsize=10) 

def main():
    
    global mqtt_cl
    

    mqtt_cl=start_mqtt()
    time.sleep(2)          # đam bao đã ket noi

    threading.Thread(target=camera_loop).start()
    threading.Thread(target=video_emitter).start()
    socketio.run(app, host='0.0.0.0', port=5000)

if __name__=="__main__":
    main()
