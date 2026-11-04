#!/usr/bin/env python3
"""Simulateur ELM327 minimal pour le régime moteur, destiné au réseau local."""
import json
import socket
import socketserver
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

OBD_PORT = 35000
WEB_PORT = 8765


class State:
    def __init__(self):
        self.rpm = 900
        self.lock = threading.Lock()

    def get(self):
        with self.lock:
            return self.rpm

    def set(self, rpm):
        with self.lock:
            self.rpm = rpm


state = State()


def answer(command, rpm):
    command = "".join(command.upper().split())
    if command in ("ATZ", "ATWS"):
        return "ELM327 v1.5"
    if command in ("ATE0", "ATL0", "ATS0", "ATH0", "ATSP0", "ATD"):
        return "OK"
    if command == "0100":
        return "41 00 00 10 00 00"
    if command == "010C":
        value = min(max(rpm, 0), 16383) * 4
        return f"41 0C {value >> 8:02X} {value & 255:02X}"
    return "NO DATA" if command.startswith("01") else "?"


class OBDHandler(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.settimeout(5)
        command = bytearray()
        while True:
            try:
                data = self.request.recv(256)
            except (TimeoutError, ConnectionError, OSError):
                return
            if not data:
                return
            for char in data:
                if char in (10, 13):
                    if command:
                        response = answer(command.decode("ascii", errors="replace"), state.get())
                        try:
                            self.request.sendall((response + "\r\r>").encode("ascii"))
                        except OSError:
                            return
                        command.clear()
                elif len(command) < 128:
                    command.append(char)
                else:
                    return


class OBDServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


PAGE = '''<!doctype html><html lang="fr"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Simulateur OBD Luma</title><style>
body{font:18px system-ui;background:#101610;color:#e8f6dc;margin:0;padding:32px}
main{max-width:620px;margin:auto}section{background:#1b291b;padding:24px;border-radius:20px}
h1{font-size:30px}strong{font-size:40px;color:#c9fb77}input{width:100%;accent-color:#c9fb77}
small{color:#b3c1aa;line-height:1.5}code{color:#c9fb77}button{padding:12px;border-radius:10px}
</style><main><h1>Simulateur OBD</h1><section><p>Régime moteur simulé</p>
<strong><span id="value">900</span> tr/min</strong><p><input id="rpm" type="range" min="0" max="12000" step="50" value="900"></p>
<p><small>Dans Luma, ouvre Activités → Compte-tours OBD. Adresse du Mac : <code id="host">chargement…</code> · Port : <code>35000</code></small></p>
<p id="error" role="alert"></p></section><p><small>Garde cette fenêtre Terminal ouverte. Arrête le simulateur avec Ctrl+C.</small></p></main>
<script>const slider=document.querySelector('#rpm'), value=document.querySelector('#value'), error=document.querySelector('#error');
fetch('/info').then(r=>r.json()).then(d=>{document.querySelector('#host').textContent=d.host;slider.value=d.rpm;value.textContent=d.rpm}).catch(()=>error.textContent='Simulateur indisponible');
let target=null,busy=false;async function sync(){if(busy)return;busy=true;while(target!==null){const rpm=target;target=null;try{const r=await fetch('/rpm',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({rpm})});if(!r.ok)throw Error();error.textContent=''}catch{error.textContent='Mise à jour impossible'}}busy=false}
slider.addEventListener('input',()=>{value.textContent=slider.value;target=Number(slider.value);sync()});</script></html>'''.encode("utf-8")


def local_address():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.connect(("192.0.2.1", 9))
            address = sock.getsockname()[0]
            return address if address != "127.0.0.1" else "Vérifie le Wi-Fi du Mac"
    except OSError:
        return "Vérifie le Wi-Fi du Mac"


class WebHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.reply(200, PAGE, "text/html; charset=utf-8")
        elif self.path == "/info":
            self.reply(200, json.dumps({"host": local_address(), "rpm": state.get()}).encode(), "application/json")
        else:
            self.send_error(404)

    def do_POST(self):
        origin = self.headers.get("Origin", "")
        if self.path != "/rpm" or origin not in (f"http://127.0.0.1:{WEB_PORT}", f"http://localhost:{WEB_PORT}"):
            self.send_error(403); return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 1 <= length <= 128 or self.headers.get_content_type() != "application/json":
                raise ValueError("requête invalide")
            rpm = json.loads(self.rfile.read(length)).get("rpm")
            if type(rpm) is not int or not 0 <= rpm <= 12000:
                raise ValueError("régime invalide")
        except (ValueError, TypeError, AttributeError, json.JSONDecodeError):
            self.send_error(400); return
        state.set(rpm)
        self.reply(200, b'{}', "application/json")

    def reply(self, status, data, content_type):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format, *args):
        pass


def main():
    try:
        with OBDServer(("0.0.0.0", OBD_PORT), OBDHandler) as obd, \
             ThreadingHTTPServer(("127.0.0.1", WEB_PORT), WebHandler) as web:
            threading.Thread(target=obd.serve_forever, daemon=True).start()
            print(f"Simulateur OBD : {local_address()}:{OBD_PORT}", flush=True)
            print(f"Réglage du régime : http://127.0.0.1:{WEB_PORT}", flush=True)
            print("Ctrl+C pour arrêter.", flush=True)
            webbrowser.open(f"http://127.0.0.1:{WEB_PORT}")
            try:
                web.serve_forever()
            except KeyboardInterrupt:
                pass
            finally:
                obd.shutdown()
    except OSError as error:
        print(f"Impossible de démarrer le simulateur : {error}", flush=True)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
